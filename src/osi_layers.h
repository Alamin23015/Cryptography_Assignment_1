#ifndef OSI_LAYERS_H
#define OSI_LAYERS_H

#include <stddef.h>
#include <stdint.h>

#define OSI_MAX_MESSAGE 60000U
#define OSI_HEADER_SIZE 9U
#define OSI_FRAME_SIZE (OSI_MAX_MESSAGE + (7U * OSI_HEADER_SIZE))

typedef struct {
    unsigned char *data;
    size_t length;
} OsiPacket;

const char *osi_layer_name(unsigned int layer);
int osi_encapsulate(const unsigned char *payload, size_t payload_length,
                    OsiPacket *packet, const char *direction);
int osi_decapsulate(const unsigned char *frame, size_t frame_length,
                    unsigned char *payload, size_t payload_capacity,
                    size_t *payload_length, const char *direction);
void osi_packet_free(OsiPacket *packet);

#endif