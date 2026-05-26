#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include "files.h"
#include "http.h"
#include "mime.h"

#define FILE_BUFFER_SIZE 65536

static int has_traversal(const char *uri)
{
    char lower[2048];
    size_t i;
    for (i = 0; uri[i] && i < sizeof(lower) - 1; i++)
        lower[i] = (uri[i] >= 'A' && uri[i] <= 'Z') ? (char)(uri[i] + 32) : uri[i];
    lower[i] = 0;
    if (strstr(lower, "..") != NULL)    return 1;
    if (strstr(lower, "%2e") != NULL)   return 1;
    if (strstr(lower, "%2f") != NULL)   return 1;
    return 0;
}

void serve_file(int client_fd, const char *uri, const char *www_root)
{
    if (has_traversal(uri)) {
        fprintf(stderr, "[SECURITY] Directory traversal bloqueado: %s\n", uri);
        send_error(client_fd, HTTP_403_FORBIDDEN);
        return;
    }

    char root_real[PATH_MAX];
    if (realpath(www_root, root_real) == NULL) {
        send_error(client_fd, HTTP_500_INTERNAL_ERROR);
        return;
    }

    const char *path_suffix = (strcmp(uri, "/") == 0) ? "/index.html" : uri;
    char candidate[PATH_MAX];
    int n = snprintf(candidate, sizeof(candidate), "%s%s", www_root, path_suffix);
    if (n < 0 || n >= (int)sizeof(candidate)) {
        send_error(client_fd, HTTP_400_BAD_REQUEST);
        return;
    }

    char file_real[PATH_MAX];
    if (realpath(candidate, file_real) == NULL) {
        send_error(client_fd, HTTP_404_NOT_FOUND);
        return;
    }

    size_t root_len = strlen(root_real);
    if (strncmp(file_real, root_real, root_len) != 0 ||
        (file_real[root_len] != 0 && file_real[root_len] != '/')) {
        fprintf(stderr, "[SECURITY] Escape de root bloqueado: %s\n", uri);
        send_error(client_fd, HTTP_403_FORBIDDEN);
        return;
    }

    struct stat st;
    if (stat(file_real, &st) == -1) {
        send_error(client_fd, HTTP_404_NOT_FOUND);
        return;
    }
    if (!S_ISREG(st.st_mode)) {
        send_error(client_fd, HTTP_403_FORBIDDEN);
        return;
    }

    int fd = open(file_real, O_RDONLY);
    if (fd == -1) {
        send_error(client_fd, errno == EACCES ? HTTP_403_FORBIDDEN : HTTP_500_INTERNAL_ERROR);
        return;
    }

    const char *mime = get_mime_type(file_real);
    char date_buf[128];
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);

    char header[1024];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Date: %s\r\n"
        "Server: minihttpd/1.0\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lld\r\n"
        "Connection: keep-alive\r\n"
        "\r\n",
        date_buf, mime, (long long)st.st_size);

    if (hlen < 0 || hlen >= (int)sizeof(header)) {
        close(fd);
        send_error(client_fd, HTTP_500_INTERNAL_ERROR);
        return;
    }

    send(client_fd, header, (size_t)hlen, MSG_NOSIGNAL);

    char buf[FILE_BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t total_sent = 0;
        while (total_sent < bytes_read) {
            ssize_t sent = send(client_fd, buf + total_sent,
                                (size_t)(bytes_read - total_sent), MSG_NOSIGNAL);
            if (sent <= 0) { close(fd); return; }
            total_sent += sent;
        }
    }
    close(fd);
    printf("[SERVE]   %s -> %s (%lld bytes)\n", uri, mime, (long long)st.st_size);
}
