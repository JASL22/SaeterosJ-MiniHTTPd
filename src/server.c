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
static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
static int add_to_epoll(int epoll_fd, int fd)
{
    struct epoll_event ev;
    ev.events  = EPOLLIN | EPOLLET;   /* Lectura disponible, modo edge-triggered */
    ev.data.fd = fd;
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}
//Manejo de clientes
static int handle_client(int client_fd)
{
    char buffer[RECV_BUFFER_SIZE];
    ssize_t total = 0;
    ssize_t n;
    while (total < (ssize_t)(RECV_BUFFER_SIZE - 1)) {
        n = recv(client_fd, buffer + total, RECV_BUFFER_SIZE - 1 - total, 0);
        if (n > 0) {
            total += n;
            buffer[total] = '\0';
            if (strstr(buffer, "\r\n\r\n") != NULL)
                break;
        } else if (n == 0) {
            return 0;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            perror("recv");
            return 0;
        }
    }
    if (total == 0)
        return 0;
    buffer[total] = '\0';
    //Parsear la petición HTTP
    http_request_t req;
    memset(&req, 0, sizeof(req));
    int parse_result = parse_request(buffer, (size_t)total, &req);
    if (parse_result != 0) {
        send_error(client_fd, (http_status_t)parse_result);
        return 0;
    }
    //Log de la petición
    printf("[REQUEST] %s %s %s\n", req.method, req.uri, req.version);
    //Servir el archivo solicitado
    serve_file(client_fd, req.uri, WWW_ROOT);
    //Decidir si mantener la conexión abierta (keep-alive)
    return req.keep_alive;
}
//Servidor principal
int run_server(int port)
{
    //Crear socket TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return -1;
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return -1;
    }
    //Bind: asociar el socket a la dirección/puerto
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
    //Listen: poner el socket en modo escucha
    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        return -1;
    }
    //El socket del servidor también debe ser no-bloqueante
    if (set_nonblocking(server_fd) == -1) {
        perror("set_nonblocking server");
        close(server_fd);
        return -1;
    }
    //Crear instancia de epoll
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        return -1;
    }
    //Registrar el socket del servidor para detectar nuevas conexiones
    if (add_to_epoll(epoll_fd, server_fd) == -1) {
        perror("add_to_epoll server");
        close(epoll_fd);
        close(server_fd);
        return -1;
    }
    printf("Servidor escuchando en http://localhost:%d\n", port);
    // 5. Bucle de eventos
    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int nready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nready == -1) {
            if (errno == EINTR) continue;   //Interrumpido por señal, reintentar
            perror("epoll_wait");
            break;
        }
        //Procesar cada evento listo
        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;
            if (fd == server_fd) {
                //Nuevo cliente: aceptar todas las conexiones pendientes
                while (1) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd,
                                          (struct sockaddr *)&client_addr,
                                          &client_len);
                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;   //No hay más conexiones pendientes
                        perror("accept");
                        break;
                    }
                    //Configurar cliente como no-bloqueante
                    if (set_nonblocking(client_fd) == -1) {
                        perror("set_nonblocking client");
                        close(client_fd);
                        continue;
                    }
                    //Agregar cliente al epoll
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
                //Cliente existente: datos disponibles para leer
                int keep = handle_client(fd);
                if (!keep) {
                    //Cerrar conexión: quitar de epoll y cerrar fd
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    printf("[CLOSE]   fd=%d cerrado\n", fd);
                }
            }
        }
    }
    //Limpieza al salir
    close(epoll_fd);
    close(server_fd);
    return 0;
}
