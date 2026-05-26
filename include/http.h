#ifndef HTTP_H
#define HTTP_H
#include <stddef.h>
//Límites de tamaño para prevenir ataques de buffer overflow y DoS 
#define MAX_REQUEST_LINE  8192 
#define MAX_HEADER_SIZE   8192 
#define MAX_URI_SIZE      2048 
#define MAX_HEADERS       32  
#define RECV_BUFFER_SIZE  16384 
//Códigos de estado HTTP/1.1 que el servidor soporta
typedef enum {
    HTTP_200_OK                  = 200,
    HTTP_400_BAD_REQUEST         = 400,
    HTTP_403_FORBIDDEN           = 403,
    HTTP_404_NOT_FOUND           = 404,
    HTTP_405_METHOD_NOT_ALLOWED  = 405,
    HTTP_500_INTERNAL_ERROR      = 500
} http_status_t;
//Estructura que representa un encabezado HTTP (nombre: valor)
typedef struct {
    char name[256];
    char value[1024];
} http_header_t;

//Estructura que representa una petición HTTP parseada
typedef struct {
    char          method[16];      
    char          uri[MAX_URI_SIZE];    
    char          version[16];           
    http_header_t headers[MAX_HEADERS];  
    int           header_count;          
    int           keep_alive;
} http_request_t;
int parse_request(const char *buffer, size_t len, http_request_t *req);
void send_response(int client_fd, http_status_t status,
                   const char *content_type, const char *body, size_t body_len);
void send_error(int client_fd, http_status_t status);
const char *get_status_text(http_status_t status);
#endif
