#include <string.h>
#include "mime.h"
//Entrada de la tabla MIME: extensión → tipo MIME
typedef struct {
    const char *extension;
    const char *mime_type;
} mime_entry_t;
static const mime_entry_t mime_table[] = {
    // Texto
    { ".html",  "text/html; charset=utf-8"  },
    { ".htm",   "text/html; charset=utf-8"  },
    { ".css",   "text/css"                  },
    { ".js",    "application/javascript"    },
    { ".txt",   "text/plain; charset=utf-8" },
    { ".json",  "application/json"          },
    { ".xml",   "application/xml"           },
    // Imágenes
    { ".png",   "image/png"                 },
    { ".jpg",   "image/jpeg"                },
    { ".jpeg",  "image/jpeg"                },
    { ".gif",   "image/gif"                 },
    { ".ico",   "image/x-icon"              },
    { ".svg",   "image/svg+xml"             },
    { ".webp",  "image/webp"                },
    //Fuentes
    { ".woff",  "font/woff"                 },
    { ".woff2", "font/woff2"                },
    //Fin de tabla
    { NULL, NULL }
};
const char *get_mime_type(const char *filename)
{
    if (filename == NULL)
        return "application/octet-stream";

    //Encontrar la extensión: última aparición de '.'
    const char *dot = strrchr(filename, '.');
    if (dot == NULL || dot[1] == '\0')
        return "application/octet-stream";   // Sin extensión

    // Buscar en la tabla (case-insensitive con strcasecmp)
    for (int i = 0; mime_table[i].extension != NULL; i++) {
        if (strcasecmp(dot, mime_table[i].extension) == 0)
            return mime_table[i].mime_type;
    }
    // Extensión no reconocida: tipo genérico
    return "application/octet-stream";
}
