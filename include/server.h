#ifndef SERVER_H
#define SERVER_H

/* Puerto por defecto del servidor */
#define DEFAULT_PORT     8080

/* Máximo de conexiones en la cola de listen() */
#define BACKLOG          128

/* Máximo de eventos que epoll procesa por iteración */
#define MAX_EVENTS       64

/* Directorio raíz desde donde se sirven archivos estáticos */
#define WWW_ROOT         "./www"

/*
 * run_server: inicializa el socket, configura epoll y entra al bucle
 * de eventos principal. Bloquea hasta que el proceso reciba SIGINT.
 *
 * port: número de puerto TCP en el que escuchar (ej. 8080)
 * Retorna 0 al salir limpiamente, -1 en error fatal.
 */
int run_server(int port);

#endif /* SERVER_H */
