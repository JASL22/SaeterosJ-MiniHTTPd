    # minihttpd

    Servidor HTTP/1.1 básico desarrollado en C utilizando sockets TCP y epoll.

    ---

    # Estructura del proyecto

    ```text
    minihttpd/
    ├── include/
    ├── src/
    ├── obj/
    ├── www/
    ├── Makefile
    └── README.md
    ```

    ---

    # Navegación rápida

    ## Código fuente

    - [main.c](src/main.c)
    - [server.c](src/server.c)
    - [http.c](src/http.c)
    - [files.c](src/files.c)
    - [mime.c](src/mime.c)

    ---

    ## Headers

    - [server.h](include/server.h)
    - [http.h](include/http.h)
    - [files.h](include/files.h)
    - [mime.h](include/mime.h)

    ---

    ## Archivos web

    - [index.html](www/index.html)
    - [style.css](www/style.css)
    - [image.png](www/image.png)

    ---

    # Características

    - HTTP/1.1
    - Método GET
    - epoll
    - Keep-Alive
    - MIME Types
    - Directory Traversal Protection
    - Manejo de errores HTTP

    ---

    # Compilación

    ```bash
    make
    ```

    ---

    # Ejecución

    ```bash
    ./minihttpd
    ```

    o:

    ```bash
    make run
    ```

    Servidor disponible en:

    ```text
    http://localhost:8080
    ```

    ---

    # Pruebas

    ## Request básica

    ```bash
    curl http://localhost:8080
    ```

    ## Ver headers

    ```bash
    curl -v http://localhost:8080
    ```

    ## Benchmark

    ```bash
    ab -n 1000 -c 100 http://localhost:8080/
    ```

    ## Keep-Alive

    ```bash
    ab -k -n 10000 -c 100 http://localhost:8080/
    ```

    ---

    # Seguridad

    ## Directory Traversal

    ```bash
    curl --path-as-is http://localhost:8080/../../../etc/passwd
    ```

    Respuesta esperada:

    ```text
    403 Forbidden
    ```

    ---

    # MIME Types

    | Extensión | MIME |
    |---|---|
    | .html | text/html |
    | .css | text/css |
    | .png | image/png |

    ---

    # Códigos HTTP

    - 200 OK
    - 400 Bad Request
    - 403 Forbidden
    - 404 Not Found
    - 405 Method Not Allowed
    - 500 Internal Server Error

    ---

    # Herramientas utilizadas

    - GCC
    - Linux / WSL
    - epoll
    - sockets TCP
    - curl
    - ApacheBench
    - Valgrind

    ---

    # Autor

    Jhonn Saeteros
