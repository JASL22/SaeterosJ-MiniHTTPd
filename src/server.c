/*
 * server.c – Bucle de eventos con epoll y soporte de conexiones persistentes
 *
 * Arquitectura event-driven (reactor pattern):
 *   - Un solo hilo maneja múltiples clientes sin bloquear.
 *   - epoll notifica qué fds están listos para lectura/escritura.
 *   - Las conexiones son no-bloqueantes (O_NONBLOCK).
 *   - Keep-alive: si el cliente pide Connection: keep-alive, el fd
 *     permanece abierto y monitoreado por epoll.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"
#include "http.h"
#include "files.h"

/* ── Utilidades internas ─────────────────────────────────────────────────── */

/*
 * set_nonblocking: pone un file descriptor en modo no-bloqueante.
 * Es imprescindible con epoll para evitar que recv/send bloqueen el servidor.
 */
static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/*
 * add_to_epoll: registra un fd en la instancia de epoll para monitorear
 * eventos de lectura (EPOLLIN) en modo edge-triggered (EPOLLET).
 *
 * Edge-triggered: epoll notifica UNA vez cuando llegan datos nuevos.
 * El servidor debe leer TODO lo disponible antes de volver al epoll_wait.
 */
static int add_to_epoll(int epoll_fd, int fd)
{
    struct epoll_event ev;
    ev.events  = EPOLLIN | EPOLLET;   /* Lectura disponible, modo edge-triggered */
    ev.data.fd = fd;
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

/* ── Manejo de clientes ──────────────────────────────────────────────────── */

/*
 * handle_client: lee la petición completa de un cliente y la procesa.
 *
 * Con edge-triggered epoll es necesario leer en bucle hasta EAGAIN,
 * porque si no vaciamos el buffer el kernel no vuelve a notificarnos.
 *
 * Retorna:
 *   1  → conexión keep-alive, mantener el fd abierto
 *   0  → cerrar la conexión
 */
static int handle_client(int client_fd)
{
    char buffer[RECV_BUFFER_SIZE];
    ssize_t total = 0;
    ssize_t n;

    /* Leer en bucle hasta vaciar el buffer del socket o error */
    while (total < (ssize_t)(RECV_BUFFER_SIZE - 1)) {
        n = recv(client_fd, buffer + total, RECV_BUFFER_SIZE - 1 - total, 0);

        if (n > 0) {
            total += n;
            /* Si ya tenemos el fin de cabeceras HTTP (\r\n\r\n), paramos */
            buffer[total] = '\0';
            if (strstr(buffer, "\r\n\r\n") != NULL)
                break;
        } else if (n == 0) {
            /* Cliente cerró la conexión */
            return 0;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* No hay más datos por ahora (normal en edge-triggered) */
                break;
            }
            /* Error real de socket */
            perror("recv");
            return 0;
        }
    }

    if (total == 0)
        return 0;   /* No llegaron datos */

    buffer[total] = '\0';

    /* Parsear la petición HTTP */
    http_request_t req;
    memset(&req, 0, sizeof(req));

    int parse_result = parse_request(buffer, (size_t)total, &req);

    if (parse_result != 0) {
        /* Error de parseo: enviar respuesta de error apropiada */
        send_error(client_fd, (http_status_t)parse_result);
        return 0;   /* Cerrar siempre en caso de error */
    }

    /* Log de la petición */
    printf("[REQUEST] %s %s %s\n", req.method, req.uri, req.version);

    /* Servir el archivo solicitado */
    serve_file(client_fd, req.uri, WWW_ROOT);

    /* Decidir si mantener la conexión abierta (keep-alive) */
    return req.keep_alive;
}

/* ── Servidor principal ──────────────────────────────────────────────────── */

int run_server(int port)
{
    /* ── 1. Crear socket TCP ────────────────────────────────────────────── */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return -1;
    }

    /*
     * SO_REUSEADDR: permite reutilizar el puerto inmediatamente después
     * de reiniciar el servidor (evita "Address already in use").
     */
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return -1;
    }

    /* ── 2. Bind: asociar el socket a la dirección/puerto ──────────────── */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;   /* Escuchar en todas las interfaces */
    addr.sin_port        = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    /* ── 3. Listen: poner el socket en modo escucha ─────────────────────── */
    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    /* El socket del servidor también debe ser no-bloqueante */
    if (set_nonblocking(server_fd) == -1) {
        perror("set_nonblocking server");
        close(server_fd);
        return -1;
    }

    /* ── 4. Crear instancia de epoll ────────────────────────────────────── */
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        return -1;
    }

    /* Registrar el socket del servidor para detectar nuevas conexiones */
    if (add_to_epoll(epoll_fd, server_fd) == -1) {
        perror("add_to_epoll server");
        close(epoll_fd);
        close(server_fd);
        return -1;
    }

    printf("Servidor escuchando en http://localhost:%d\n", port);

    /* ── 5. Bucle de eventos ─────────────────────────────────────────────── */
    struct epoll_event events[MAX_EVENTS];

    while (1) {
        /*
         * epoll_wait: bloquea hasta que haya al menos un evento listo.
         * Retorna el número de fds listos.
         * timeout = -1 → esperar indefinidamente.
         */
        int nready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nready == -1) {
            if (errno == EINTR) continue;   /* Interrumpido por señal, reintentar */
            perror("epoll_wait");
            break;
        }

        /* Procesar cada evento listo */
        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                /* ── Nuevo cliente: aceptar todas las conexiones pendientes ── */
                while (1) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);

                    int client_fd = accept(server_fd,
                                          (struct sockaddr *)&client_addr,
                                          &client_len);
                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;   /* No hay más conexiones pendientes */
                        perror("accept");
                        break;
                    }

                    /* Configurar cliente como no-bloqueante */
                    if (set_nonblocking(client_fd) == -1) {
                        perror("set_nonblocking client");
                        close(client_fd);
                        continue;
                    }

                    /* Agregar cliente al epoll */
                    if (add_to_epoll(epoll_fd, client_fd) == -1) {
                        perror("add_to_epoll client");
                        close(client_fd);
                        continue;
                    }

                    printf("[CONNECT] Cliente %s:%d (fd=%d)\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port),
                           client_fd);
                }
            } else {
                /* ── Cliente existente: datos disponibles para leer ──────── */
                int keep = handle_client(fd);

                if (!keep) {
                    /* Cerrar conexión: quitar de epoll y cerrar fd */
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    printf("[CLOSE]   fd=%d cerrado\n", fd);
                }
                /* Si keep==1, el fd queda en epoll para la próxima petición */
            }
        }
    }

    /* Limpieza al salir */
    close(epoll_fd);
    close(server_fd);
    return 0;
}
