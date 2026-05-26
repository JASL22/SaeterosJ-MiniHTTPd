#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;
    //Procesar argumento de puerto opcional
    if (argc == 2) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Error: puerto inválido '%s'. Debe estar entre 1 y 65535.\n",
                    argv[1]);
            return EXIT_FAILURE;
        }
    } else if (argc > 2) {
        fprintf(stderr, "Uso: %s [puerto]\n", argv[0]);
        return EXIT_FAILURE;
    }
    printf("=== minihttpd arrancando en puerto %d ===\n", port);
    printf("Directorio raíz: %s\n", WWW_ROOT);
    printf("Presiona Ctrl+C para detener.\n\n");
    //Iniciar el servidor (bloquea hasta SIGINT)
    if (run_server(port) != 0) {
        fprintf(stderr, "Error fatal al iniciar el servidor.\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
