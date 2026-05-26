#ifndef MIME_H
#define MIME_H

/*
 * get_mime_type: dado un nombre de archivo (o su extensión),
 * retorna el tipo MIME correspondiente como string.
 *
 * Si la extensión no se reconoce, retorna "application/octet-stream"
 * (descarga binaria genérica), que es el comportamiento estándar seguro.
 *
 * Ejemplos:
 *   get_mime_type("index.html") → "text/html"
 *   get_mime_type("logo.png")   → "image/png"
 *   get_mime_type("data.xyz")   → "application/octet-stream"
 */
const char *get_mime_type(const char *filename);

#endif /* MIME_H */
