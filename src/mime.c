/*
 * mime.c – Tabla de tipos MIME y función de búsqueda
 *
 * Mapea extensiones de archivo a tipos MIME estándar.
 * Si la extensión no se encuentra, retorna "application/octet-stream"
 * (tipo genérico para descarga binaria), comportamiento estándar y seguro.
 */

#include <string.h>
#include "mime.h"

/* Entrada de la tabla MIME: extensión → tipo MIME */
typedef struct {
    const char *extension;
    const char *mime_type;
} mime_entry_t;

/*
 * Tabla de tipos MIME soportados.
 * Las extensiones deben incluir el punto inicial (ej. ".html").
 */
static const mime_entry_t mime_table[] = {
    /* Texto */
    { ".html",  "text/html; charset=utf-8"  },
    { ".htm",   "text/html; charset=utf-8"  },
    { ".css",   "text/css"                  },
    { ".js",    "application/javascript"    },
    { ".txt",   "text/plain; charset=utf-8" },
    { ".json",  "application/json"          },
    { ".xml",   "application/xml"           },

    /* Imágenes */
    { ".png",   "image/png"                 },
    { ".jpg",   "image/jpeg"                },
    { ".jpeg",  "image/jpeg"                },
    { ".gif",   "image/gif"                 },
    { ".ico",   "image/x-icon"              },
    { ".svg",   "image/svg+xml"             },
    { ".webp",  "image/webp"                },

    /* Fuentes */
    { ".woff",  "font/woff"                 },
    { ".woff2", "font/woff2"                },

    /* Fin de tabla */
    { NULL, NULL }
};

/*
 * get_mime_type: busca el tipo MIME por extensión del archivo.
 *
 * Algoritmo:
 *   1. Encontrar la última aparición de '.' en el nombre de archivo.
 *   2. Buscar esa extensión en la tabla (comparación case-insensitive).
 *   3. Si no se encuentra, retornar tipo genérico binario.
 */
const char *get_mime_type(const char *filename)
{
    if (filename == NULL)
        return "application/octet-stream";

    /* Encontrar la extensión: última aparición de '.' */
    const char *dot = strrchr(filename, '.');
    if (dot == NULL || dot[1] == '\0')
        return "application/octet-stream";   /* Sin extensión */

    /* Buscar en la tabla (case-insensitive con strcasecmp) */
    for (int i = 0; mime_table[i].extension != NULL; i++) {
        if (strcasecmp(dot, mime_table[i].extension) == 0)
            return mime_table[i].mime_type;
    }

    /* Extensión no reconocida: tipo genérico */
    return "application/octet-stream";
}
