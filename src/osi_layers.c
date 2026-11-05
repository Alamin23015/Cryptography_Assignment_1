#include "osi_layers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const layer_names[] = {
    "Unknown", "Physical", "Data Link", "Network", "Transport",
    "Session", "Presentation", "Application"
};

const char *osi_layer_name(unsigned int layer)
{
    if (layer < 1U || layer > 7U) {
        return layer_names[0];
    }
    return layer_names[layer];
}

static void write_u32(unsigned char *destination, uint32_t value)
{
    destination[0] = (unsigned char)(value >> 24);
    destination[1] = (unsigned char)(value >> 16);
    destination[2] = (unsigned char)(value >> 8);
    destination[3] = (unsigned char)value;
}

static uint32_t read_u32(const unsigned char *source)
{
    return ((uint32_t)source[0] << 24) |
           ((uint32_t)source[1] << 16) |
           ((uint32_t)source[2] << 8) |
           (uint32_t)source[3];
}

static uint32_t checksum(const unsigned char *data, size_t length)
{
    uint32_t value = 2166136261U;

    for (size_t index = 0; index < length; index++) {
        value ^= data[index];
        value *= 16777619U;
    }
    return value;
}

int osi_encapsulate(const unsigned char *payload, size_t payload_length,
                    OsiPacket *packet, const char *direction)
{
    size_t offset;
    size_t current_length = payload_length;
    unsigned char *data;

    if (payload == NULL || packet == NULL || payload_length > OSI_MAX_MESSAGE) {
        return -1;
    }

    data = malloc(payload_length + (7U * OSI_HEADER_SIZE));
    if (data == NULL) {
        return -1;
    }

    offset = 7U * OSI_HEADER_SIZE;
    memcpy(data + offset, payload, payload_length);

    for (unsigned int layer = 7U; layer >= 1U; layer--) {
        offset -= OSI_HEADER_SIZE;
        data[offset] = (unsigned char)layer;
        write_u32(data + offset + 1U, (uint32_t)current_length);
        write_u32(data + offset + 5U, checksum(data + offset + OSI_HEADER_SIZE,
                                               current_length));
        current_length += OSI_HEADER_SIZE;
        if (direction != NULL) {
            printf("TRACE %s %u %s\n", direction, layer,
                   osi_layer_name(layer));
        }
    }

    packet->data = data + offset;
    packet->length = current_length;
    return 0;
}

int osi_decapsulate(const unsigned char *frame, size_t frame_length,
                    unsigned char *payload, size_t payload_capacity,
                    size_t *payload_length, const char *direction)
{
    if (frame == NULL || payload == NULL || payload_length == NULL) {
        return -1;
    }

    for (unsigned int expected_layer = 1U; expected_layer <= 7U;
         expected_layer++) {
        uint32_t inner_length;
        uint32_t expected_checksum;

        if (frame_length < OSI_HEADER_SIZE || frame[0] != expected_layer) {
            return -1;
        }

        inner_length = read_u32(frame + 1U);
        expected_checksum = read_u32(frame + 5U);
        if ((size_t)inner_length != frame_length - OSI_HEADER_SIZE ||
            checksum(frame + OSI_HEADER_SIZE, inner_length) != expected_checksum) {
            return -1;
        }

        frame += OSI_HEADER_SIZE;
        frame_length = inner_length;
        if (direction != NULL) {
            printf("TRACE %s %u %s\n", direction, expected_layer,
                   osi_layer_name(expected_layer));
        }
    }

    if (frame_length > payload_capacity) {
        return -1;
    }

    memcpy(payload, frame, frame_length);
    *payload_length = frame_length;
    return 0;
}

void osi_packet_free(OsiPacket *packet)
{
    if (packet != NULL && packet->data != NULL) {
        free(packet->data);
        packet->data = NULL;
        packet->length = 0U;
    }
}