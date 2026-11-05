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

static int create_listener(const char *port)
{
    struct addrinfo hints;
    struct addrinfo *addresses = NULL;
    struct addrinfo *address;
    int server_fd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, port, &hints, &addresses) != 0) {
        return -1;
    }

    for (address = addresses; address != NULL; address = address->ai_next) {
        int reuse_address = 1;
        server_fd = socket(address->ai_family, address->ai_socktype,
                           address->ai_protocol);
        if (server_fd < 0) {
            continue;
        }
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &reuse_address, sizeof(reuse_address));
        if (bind(server_fd, address->ai_addr, address->ai_addrlen) == 0 &&
            listen(server_fd, 1) == 0) {
            break;
        }
        close(server_fd);
        server_fd = -1;
    }

    freeaddrinfo(addresses);
    return server_fd;
}

static int report_bound_port(int server_fd)
{
    struct sockaddr_storage address;
    socklen_t address_length = sizeof(address);
    unsigned int port;

    if (getsockname(server_fd, (struct sockaddr *)&address, &address_length) != 0) {
        return -1;
    }
    if (address.ss_family == AF_INET) {
        port = ntohs(((struct sockaddr_in *)&address)->sin_port);
    } else {
        port = ntohs(((struct sockaddr_in6 *)&address)->sin6_port);
    }
    printf("READY %u\n", port);
    fflush(stdout);
    return 0;
}

int main(int argc, char **argv)
{
    const char *port = "5000";
    int server_fd;
    int client_fd;
    uint32_t network_length;
    uint32_t frame_length;
    unsigned char frame[OSI_FRAME_SIZE];
    unsigned char message[OSI_MAX_MESSAGE + 1U];
    unsigned char response[OSI_MAX_MESSAGE + 1U];
    size_t message_length = 0U;
    OsiPacket outgoing = {0};
    int status = EXIT_FAILURE;

    if (argc == 3 && strcmp(argv[1], "--port") == 0) {
        port = argv[2];
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [--port PORT]\n", argv[0]);
        return EXIT_FAILURE;
    }

    server_fd = create_listener(port);
    if (server_fd < 0 || report_bound_port(server_fd) != 0) {
        fprintf(stderr, "Could not listen on port %s.\n", port);
        if (server_fd >= 0) {
            close(server_fd);
        }
        return EXIT_FAILURE;
    }

    client_fd = accept(server_fd, NULL, NULL);
    close(server_fd);
    if (client_fd < 0) {
        perror("accept");
        return EXIT_FAILURE;
    }

    if (socket_receive_all(client_fd, &network_length, sizeof(network_length)) != 0) {
        fprintf(stderr, "Could not read the incoming frame length.\n");
        goto cleanup;
    }
    frame_length = ntohl(network_length);
    if (frame_length > sizeof(frame) ||
        socket_receive_all(client_fd, frame, frame_length) != 0) {
        fprintf(stderr, "Received an invalid or incomplete frame.\n");
        goto cleanup;
    }

    if (osi_decapsulate(frame, frame_length, message, OSI_MAX_MESSAGE,
                        &message_length, "RX") != 0) {
        fprintf(stderr, "Could not validate or decapsulate the client message.\n");
        goto cleanup;
    }
    message[message_length] = '\0';

    if (message_length + 5U > OSI_MAX_MESSAGE) {
        fprintf(stderr, "The acknowledgement would exceed the message limit.\n");
        goto cleanup;
    }
    memcpy(response, "ACK: ", 5U);
    memcpy(response + 5U, message, message_length);
    if (osi_encapsulate(response, message_length + 5U, &outgoing, "TX") != 0) {
        fprintf(stderr, "Could not encapsulate the server acknowledgement.\n");
        goto cleanup;
    }

    network_length = htonl((uint32_t)outgoing.length);
    if (socket_send_all(client_fd, &network_length, sizeof(network_length)) != 0 ||
        socket_send_all(client_fd, outgoing.data, outgoing.length) != 0) {
        perror("socket send");
        goto cleanup;
    }

    printf("SERVED %zu bytes\n", message_length);
    status = EXIT_SUCCESS;

cleanup:
    close(client_fd);
    osi_packet_free(&outgoing);
    return status;
}