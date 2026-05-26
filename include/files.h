#ifndef FILES_H
#define FILES_H

#include <stddef.h>

/*
 * serve_file: lee el archivo solicitado del sistema de archivos y
 * envía la respuesta HTTP completa al cliente.
 *
 * client_fd : descriptor del socket del cliente
 * uri       : URI recibida en la petición (ej. "/index.html")
 * www_root  : directorio raíz del servidor (ej. "./www")
 *
 * Seguridad:
 *   - Usa realpath() para resolver la ruta absoluta real.
 *   - Verifica que la ruta resuelta esté DENTRO de www_root.
 *   - Si está fuera (directory traversal), responde 403 Forbidden.
 *   - Si el archivo no existe, responde 404 Not Found.
 *   - Si no tiene permisos de lectura, responde 403 Forbidden.
 */
void serve_file(int client_fd, const char *uri, const char *www_root);

#endif /* FILES_H */
