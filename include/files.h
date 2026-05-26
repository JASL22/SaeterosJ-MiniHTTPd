#ifndef FILES_H
#define FILES_H

#include <stddef.h>
void serve_file(int client_fd, const char *uri, const char *www_root);

#endif /* FILES_H */
