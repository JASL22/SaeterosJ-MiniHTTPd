#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

#include "http.h"

/* ── Texto descriptivo de cada código de estado ─────────────────────────── */

const char *get_status_text(http_status_t status)
{
    switch (status) {
        case HTTP_200_OK:                 return "OK";
        case HTTP_400_BAD_REQUEST:        return "Bad Request";
        case HTTP_403_FORBIDDEN:          return "Forbidden";
        case HTTP_404_NOT_FOUND:          return "Not Found";
        case HTTP_405_METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HTTP_500_INTERNAL_ERROR:     return "Internal Server Error";
        default:                          return "Unknown";
    }
}

/* ── Parsing de la petición HTTP ─────────────────────────────────────────── */

/*
 * parse_request: analiza el buffer de la petición HTTP cruda.
 *
 * Proceso:
 *   1. Trabajar sobre una copia para no modificar el buffer original.
 *   2. Extraer la línea de petición (método, URI, versión).
 *   3. Validar el método → solo GET permitido.
 *   4. Validar longitudes para prevenir buffer overflows.
 *   5. Parsear encabezados línea a línea.
 *   6. Detectar Connection: keep-alive.
 *
 * Retorna 0 en éxito, código HTTP de error en fallo.
 */
int parse_request(const char *buffer, size_t len, http_request_t *req)
{
    /* Copiar el buffer para poder modificarlo con strtok/strsep */
    if (len >= RECV_BUFFER_SIZE)
        return HTTP_400_BAD_REQUEST;

    char copy[RECV_BUFFER_SIZE];
    memcpy(copy, buffer, len);
    copy[len] = '\0';

    /* ── Paso 1: Extraer la línea de petición ───────────────────────────── */
    char *saveptr = NULL;
    char *request_line = strtok_r(copy, "\r\n", &saveptr);
    if (request_line == NULL)
        return HTTP_400_BAD_REQUEST;

    /* Validar longitud total de la línea de petición */
    if (strlen(request_line) >= MAX_REQUEST_LINE)
        return HTTP_400_BAD_REQUEST;

    /* ── Paso 2: Descomponer método, URI y versión ──────────────────────── */
    char method[16]  = {0};
    char uri[MAX_URI_SIZE] = {0};
    char version[16] = {0};

    /*
     * sscanf con límites de ancho de campo: evita overflow.
     * %15s  → máximo 15 caracteres para método
     * %2047s → máximo 2047 caracteres para URI
     * %15s  → máximo 15 caracteres para versión
     */
    int fields = sscanf(request_line, "%15s %2047s %15s", method, uri, version);
    if (fields != 3)
        return HTTP_400_BAD_REQUEST;

    /* ── Paso 3: Validar el método HTTP ─────────────────────────────────── */
    if (strncmp(method, "GET", 3) != 0)
        return HTTP_405_METHOD_NOT_ALLOWED;

    /* ── Paso 4: Validar la versión HTTP ────────────────────────────────── */
    if (strncmp(version, "HTTP/", 5) != 0)
        return HTTP_400_BAD_REQUEST;

    /* ── Paso 5: Copiar campos validados a la estructura ────────────────── */
    snprintf(req->method,  sizeof(req->method),  "%s", method);
    snprintf(req->uri,     sizeof(req->uri),     "%s", uri);
    snprintf(req->version, sizeof(req->version), "%s", version);


    /* ── Paso 6: Parsear encabezados ─────────────────────────────────────── */
    req->header_count = 0;
    req->keep_alive   = 0;  /* Por defecto: cerrar conexión */

    /*
     * HTTP/1.1 define keep-alive como comportamiento por defecto.
     * Lo ponemos a 1 si la versión es 1.1, salvo que el cliente
     * envíe explícitamente "Connection: close".
     */
    if (strncmp(version, "HTTP/1.1", 8) == 0)
        req->keep_alive = 1;

    char *line = strtok_r(NULL, "\r\n", &saveptr);
    while (line != NULL && strlen(line) > 0 && req->header_count < MAX_HEADERS) {

        /* Validar longitud del encabezado completo */
        if (strlen(line) >= MAX_HEADER_SIZE)
            return HTTP_400_BAD_REQUEST;

        /* Encontrar el separador ':' */
        char *colon = strchr(line, ':');
        if (colon == NULL) {
            line = strtok_r(NULL, "\r\n", &saveptr);
            continue;   /* Encabezado mal formado, ignorar */
        }

        /* Separar nombre y valor */
        *colon = '\0';
        char *name  = line;
        char *value = colon + 1;

        /* Saltar espacios al inicio del valor */
        while (*value == ' ' || *value == '\t')
            value++;

        /* Guardar el encabezado de forma segura */
        http_header_t *hdr = &req->headers[req->header_count];
        strncpy(hdr->name,  name,  sizeof(hdr->name)  - 1);
        strncpy(hdr->value, value, sizeof(hdr->value) - 1);
        hdr->name[sizeof(hdr->name)   - 1] = '\0';
        hdr->value[sizeof(hdr->value) - 1] = '\0';
        req->header_count++;

        /* Detectar Connection: close para deshabilitar keep-alive */
        if (strcasecmp(name, "Connection") == 0) {
            if (strcasecmp(value, "close") == 0)
                req->keep_alive = 0;
            else if (strcasecmp(value, "keep-alive") == 0)
                req->keep_alive = 1;
        }

        line = strtok_r(NULL, "\r\n", &saveptr);
    }

    return 0;   /* Éxito */
}

/* ── Generación de respuestas HTTP ──────────────────────────────────────── */

/*
 * send_response: envía una respuesta HTTP completa.
 *
 * Construye la cabecera con snprintf (seguro contra overflow) y
 * luego envía la cabecera + el cuerpo en llamadas separadas.
 */
void send_response(int client_fd, http_status_t status,
                   const char *content_type, const char *body, size_t body_len)
{
    /* Generar fecha RFC 7231 para el encabezado Date */
    char date_buf[128];
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);

    /* Construir cabecera HTTP con snprintf (seguro) */
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Date: %s\r\n"
        "Server: minihttpd/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: keep-alive\r\n"
        "\r\n",
        (int)status,
        get_status_text(status),
        date_buf,
        content_type ? content_type : "application/octet-stream",
        body_len
    );

    if (header_len < 0 || header_len >= (int)sizeof(header)) {
        /* Buffer de cabecera insuficiente (no debería ocurrir) */
        return;
    }

    /* Enviar cabecera */
    send(client_fd, header, (size_t)header_len, MSG_NOSIGNAL);

    /* Enviar cuerpo si existe */
    if (body && body_len > 0)
        send(client_fd, body, body_len, MSG_NOSIGNAL);
}

/*
 * send_error: envía una página HTML de error mínima.
 * Usada para 400, 403, 404, 405, 500.
 */
void send_error(int client_fd, http_status_t status)
{
    char body[512];
    int body_len = snprintf(body, sizeof(body),
        "<!DOCTYPE html>\r\n"
        "<html><head><title>%d %s</title></head>\r\n"
        "<body><h1>%d %s</h1>"
        "<p>minihttpd</p></body></html>\r\n",
        (int)status, get_status_text(status),
        (int)status, get_status_text(status)
    );

    if (body_len < 0) body_len = 0;

    send_response(client_fd, status, "text/html",
                  body, (size_t)body_len);
}
