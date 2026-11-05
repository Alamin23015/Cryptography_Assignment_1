#include "socket_io.h"

#include <errno.h>
#include <sys/socket.h>

int socket_send_all(int socket_fd, const void *buffer, size_t length)
{
    const unsigned char *bytes = buffer;
    size_t sent = 0U;

    while (sent < length) {
        ssize_t result = send(socket_fd, bytes + sent, length - sent, 0);
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result <= 0) {
            return -1;
        }
        sent += (size_t)result;
    }
    return 0;
}

int socket_receive_all(int socket_fd, void *buffer, size_t length)
{
    unsigned char *bytes = buffer;
    size_t received = 0U;

    while (received < length) {
        ssize_t result = recv(socket_fd, bytes + received, length - received, 0);
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result <= 0) {
            return -1;
        }
        received += (size_t)result;
    }
    return 0;
}