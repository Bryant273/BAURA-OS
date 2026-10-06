#ifndef BAURA_IPC_V2_H
#define BAURA_IPC_V2_H

#include <stddef.h>
#include <stdint.h>

#include "baura_abi_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t kind;
    uint16_t flags;
    uint32_t opcode;
    uint64_t request_id;
    uint32_t payload_bytes;
    uint16_t attachment_count;
} BauraIpcV2Header;

typedef enum {
    BAURA_IPC_V2_OK = 0,
    BAURA_IPC_V2_ERR_INVALID_ARGUMENT = -1,
    BAURA_IPC_V2_ERR_BUFFER_TOO_SMALL = -2,
    BAURA_IPC_V2_ERR_MALFORMED_FRAME = -3,
    BAURA_IPC_V2_ERR_UNSUPPORTED_VERSION = -4,
    BAURA_IPC_V2_ERR_UNKNOWN_OPCODE = -5,
    BAURA_IPC_V2_ERR_INVALID_PAYLOAD = -6
} BauraIpcV2Result;

/* payload and frame must not overlap; frame_size is written only on success. */
BauraIpcV2Result baura_ipc_v2_encode_frame(
    const BauraIpcV2Header *header,
    const uint8_t *payload,
    size_t payload_size,
    uint8_t *frame,
    size_t frame_capacity,
    size_t *frame_size);

/* On success, payload borrows storage from frame; ignore outputs on failure. */
BauraIpcV2Result baura_ipc_v2_decode_frame(
    const uint8_t *frame,
    size_t frame_size,
    BauraIpcV2Header *header,
    const uint8_t **payload);

#ifdef __cplusplus
}
#endif

#endif
