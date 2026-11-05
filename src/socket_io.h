#ifndef SOCKET_IO_H
#define SOCKET_IO_H

#include <stddef.h>

int socket_send_all(int socket_fd, const void *buffer, size_t length);
int socket_receive_all(int socket_fd, void *buffer, size_t length);

#endif