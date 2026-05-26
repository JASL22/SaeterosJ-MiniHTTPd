#ifndef SERVER_H
#define SERVER_H
//Puerto por defecto del servidor
#define DEFAULT_PORT     8080
//Máximo de conexiones en la cola de listen()
#define BACKLOG          128
//Máximo de eventos que epoll procesa por iteración
#define MAX_EVENTS       64
//Directorio raíz desde donde se sirven archivos estáticos
#define WWW_ROOT         "./www"
int run_server(int port);
#endif
