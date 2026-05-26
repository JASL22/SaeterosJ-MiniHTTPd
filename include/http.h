#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

/* Límites de tamaño para prevenir ataques de buffer overflow y DoS */
#define MAX_REQUEST_LINE  8192   /* Tamaño máximo de la línea de petición     */
#define MAX_HEADER_SIZE   8192   /* Tamaño máximo de un encabezado individual */
#define MAX_URI_SIZE      2048   /* Tamaño máximo del URI                     */
#define MAX_HEADERS       32     /* Número máximo de encabezados aceptados    */
#define RECV_BUFFER_SIZE  16384  /* Tamaño del buffer de recepción            */

/* Códigos de estado HTTP/1.1 que el servidor soporta */
typedef enum {
    HTTP_200_OK                  = 200,
    HTTP_400_BAD_REQUEST         = 400,
    HTTP_403_FORBIDDEN           = 403,
    HTTP_404_NOT_FOUND           = 404,
    HTTP_405_METHOD_NOT_ALLOWED  = 405,
    HTTP_500_INTERNAL_ERROR      = 500
} http_status_t;

/* Estructura que representa un encabezado HTTP (nombre: valor) */
typedef struct {
    char name[256];
    char value[1024];
} http_header_t;

/* Estructura que representa una petición HTTP parseada */
typedef struct {
    char          method[16];              /* Método HTTP (solo GET es válido)  */
    char          uri[MAX_URI_SIZE];       /* URI solicitada                    */
    char          version[16];            /* Versión HTTP (HTTP/1.1)           */
    http_header_t headers[MAX_HEADERS];   /* Arreglo de encabezados parseados  */
    int           header_count;           /* Cantidad de encabezados recibidos */
    int           keep_alive;            /* 1 si Connection: keep-alive        */
} http_request_t;

/*
 * parse_request: analiza el buffer recibido y rellena la estructura de petición.
 * Retorna 0 en éxito, o un código HTTP de error (400, 405, etc.) en fallo.
 */
int parse_request(const char *buffer, size_t len, http_request_t *req);

/*
 * send_response: construye y envía la respuesta HTTP completa al cliente.
 * Incluye encabezados y cuerpo (archivo o mensaje de error).
 */
void send_response(int client_fd, http_status_t status,
                   const char *content_type, const char *body, size_t body_len);

/*
 * send_error: envía una respuesta de error con una página HTML básica.
 */
void send_error(int client_fd, http_status_t status);

/*
 * get_status_text: retorna el texto descriptivo del código de estado.
 */
const char *get_status_text(http_status_t status);

#endif /* HTTP_H */
