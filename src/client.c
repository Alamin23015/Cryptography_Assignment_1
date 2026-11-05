#define _POSIX_C_SOURCE 200809L

#include "osi_layers.h"
#include "socket_io.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void usage(const char *program)
{
    fprintf(stderr, "Usage: %s [--host HOST] [--port PORT] MESSAGE\n", program);
}

static int connect_to_server(const char *host, const char *port)
{
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    struct addrinfo *address;
    int socket_fd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port, &hints, &addresses) != 0) {
        return -1;
    }

    for (address = addresses; address != NULL; address = address->ai_next) {
        socket_fd = socket(address->ai_family, address->ai_socktype,
                           address->ai_protocol);
        if (socket_fd >= 0 &&
            connect(socket_fd, address->ai_addr, address->ai_addrlen) == 0) {
            break;
        }
        if (socket_fd >= 0) {
            close(socket_fd);
            socket_fd = -1;
        }
    }

    freeaddrinfo(addresses);
    return socket_fd;
}

int main(int argc, char **argv)
{
    const char *host = "127.0.0.1";
    const char *port = "5000";
    int message_index = 1;
    size_t message_length = 0U;
    char *message;
    OsiPacket outgoing = {0};
    unsigned char frame[OSI_FRAME_SIZE];
    unsigned char response[OSI_MAX_MESSAGE + 1U];
    size_t response_length = 0U;
    uint32_t frame_length;
    uint32_t network_length;
    int socket_fd;
    int status = EXIT_FAILURE;

    while (message_index < argc && argv[message_index][0] == '-') {
        if (strcmp(argv[message_index], "--help") == 0) {
            usage(argv[0]);
            return EXIT_SUCCESS;
        }
        if (message_index + 1 >= argc) {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
        if (strcmp(argv[message_index], "--host") == 0) {
            host = argv[message_index + 1];
        } else if (strcmp(argv[message_index], "--port") == 0) {
            port = argv[message_index + 1];
        } else {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
        message_index += 2;
    }

    if (message_index >= argc) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    for (int index = message_index; index < argc; index++) {
        message_length += strlen(argv[index]) + (index > message_index ? 1U : 0U);
    }
    if (message_length > OSI_MAX_MESSAGE) {
        fprintf(stderr, "Message exceeds the %u-byte limit.\n", OSI_MAX_MESSAGE);
        return EXIT_FAILURE;
    }

    message = malloc(message_length + 1U);
    if (message == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    message[0] = '\0';
    for (int index = message_index; index < argc; index++) {
        if (index > message_index) {
            strcat(message, " ");
        }
        strcat(message, argv[index]);
    }

    if (osi_encapsulate((const unsigned char *)message, message_length,
                        &outgoing, "TX") != 0) {
        fprintf(stderr, "Could not encapsulate the message.\n");
        goto cleanup;
    }

    socket_fd = connect_to_server(host, port);
    if (socket_fd < 0) {
        fprintf(stderr, "Could not connect to %s:%s. Is the server running?\n",
                host, port);
        goto cleanup;
    }

    frame_length = htonl((uint32_t)outgoing.length);
    if (socket_send_all(socket_fd, &frame_length, sizeof(frame_length)) != 0 ||
        socket_send_all(socket_fd, outgoing.data, outgoing.length) != 0 ||
        socket_receive_all(socket_fd, &network_length, sizeof(network_length)) != 0) {
        perror("socket communication");
        close(socket_fd);
        goto cleanup;
    }

    frame_length = ntohl(network_length);
    if (frame_length > sizeof(frame) ||
        socket_receive_all(socket_fd, frame, frame_length) != 0) {
        fprintf(stderr, "Received an invalid or incomplete frame.\n");
        close(socket_fd);
        goto cleanup;
    }
    close(socket_fd);

    if (osi_decapsulate(frame, frame_length, response, OSI_MAX_MESSAGE,
                        &response_length, "RX") != 0) {
        fprintf(stderr, "Could not validate or decapsulate the server response.\n");
        goto cleanup;
    }
    response[response_length] = '\0';
    printf("REPLY: %s\n", response);
    status = EXIT_SUCCESS;

cleanup:
    free(message);
    osi_packet_free(&outgoing);
    return status;
}