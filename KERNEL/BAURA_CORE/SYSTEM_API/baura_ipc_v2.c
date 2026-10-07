#include "baura_ipc_v2.h"

static uint16_t read_u16_le(const uint8_t *bytes) {
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

static uint32_t read_u32_le(const uint8_t *bytes) {
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static uint64_t read_u64_le(const uint8_t *bytes) {
    uint64_t value = 0;
    uint32_t index;
    for (index = 0; index < 8; ++index) value |= (uint64_t)bytes[index] << (index * 8);
    return value;
}

static void write_u16_le(uint8_t *bytes, uint16_t value) {
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static void write_u32_le(uint8_t *bytes, uint32_t value) {
    uint32_t index;
    for (index = 0; index < 4; ++index) bytes[index] = (uint8_t)(value >> (index * 8));
}

static void write_u64_le(uint8_t *bytes, uint64_t value) {
    uint32_t index;
    for (index = 0; index < 8; ++index) bytes[index] = (uint8_t)(value >> (index * 8));
}

static uint8_t is_known_opcode(uint32_t opcode) {
    return opcode == BAURA_ABI_OPCODE_FS_CREATE ||
           opcode == BAURA_ABI_OPCODE_FS_READ ||
           opcode == BAURA_ABI_OPCODE_PROCESS_CREATE ||
           opcode == BAURA_ABI_OPCODE_MEMORY_ALLOCATE ||
           opcode == BAURA_ABI_OPCODE_NETWORK_CONNECT;
}

static uint16_t expected_request_attachments(uint32_t opcode) {
    return opcode == BAURA_ABI_OPCODE_FS_READ ? 1u : 0u;
}

static uint8_t is_non_blocking(uint32_t opcode) {
    return opcode == BAURA_ABI_OPCODE_FS_CREATE ||
           opcode == BAURA_ABI_OPCODE_PROCESS_CREATE ||
           opcode == BAURA_ABI_OPCODE_MEMORY_ALLOCATE;
}

static uint16_t expected_success_attachments(uint32_t opcode) {
    return opcode == BAURA_ABI_OPCODE_MEMORY_ALLOCATE ? 1u : 0u;
}

static uint8_t valid_utf8(const uint8_t *bytes, size_t length) {
    size_t index = 0;
    while (index < length) {
        uint8_t first = bytes[index];
        uint32_t codepoint;
        uint32_t continuation_count;
        uint32_t continuation;

        if (first == 0) return 0;
        if (first <= 0x7F) {
            ++index;
            continue;
        }
        if (first >= 0xC2 && first <= 0xDF) {
            codepoint = first & 0x1Fu;
            continuation_count = 1;
        } else if (first >= 0xE0 && first <= 0xEF) {
            codepoint = first & 0x0Fu;
            continuation_count = 2;
        } else if (first >= 0xF0 && first <= 0xF4) {
            codepoint = first & 0x07u;
            continuation_count = 3;
        } else {
            return 0;
        }

        if (continuation_count > length - index - 1u) return 0;
        for (continuation = 1; continuation <= continuation_count; ++continuation) {
            uint8_t next = bytes[index + continuation];
            if ((next & 0xC0) != 0x80) return 0;
            codepoint = (codepoint << 6) | (next & 0x3Fu);
        }
        if ((continuation_count == 1u && codepoint < 0x80u) ||
            (continuation_count == 2u && codepoint < 0x800u) ||
            (continuation_count == 3u && codepoint < 0x10000u) ||
            (codepoint >= 0xD800u && codepoint <= 0xDFFFu) ||
            codepoint > 0x10FFFFu) {
            return 0;
        }
        index += continuation_count + 1u;
    }
    return 1;
}

static uint8_t valid_path(const uint8_t *path, uint32_t length) {
    uint32_t index;
    uint32_t drive_end;
    if (length < 6u || length > 255u ||
        path[0] != 'P' || path[1] != 'C' || path[2] != '/') return 0;
    drive_end = 3;
    while (drive_end < length && path[drive_end] != ':') {
        uint8_t ch = path[drive_end];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')) return 0;
        ++drive_end;
    }
    if (drive_end == 3u || drive_end + 1u >= length ||
        path[drive_end + 1u] != '/') return 0;
    for (index = 0; index < length; ++index) {
        if (path[index] == 0) return 0;
    }
    return valid_utf8(path, length);
}

static uint8_t read_string(const uint8_t *payload, size_t payload_size,
                           size_t *offset, uint32_t maximum, uint8_t is_path) {
    uint32_t length;
    if (*offset > payload_size || payload_size - *offset < 4u) return 0;
    length = read_u32_le(payload + *offset);
    *offset += 4u;
    if (length > maximum || length > payload_size - *offset) return 0;
    if (is_path) {
        if (!valid_path(payload + *offset, length)) return 0;
    } else if (!valid_utf8(payload + *offset, length)) {
        return 0;
    }
    *offset += length;
    return 1;
}

static BauraIpcV2Result validate_request_payload(
    uint32_t opcode, const uint8_t *payload, size_t payload_size) {
    size_t offset = 0;
    uint64_t flags;

    switch (opcode) {
        case BAURA_ABI_OPCODE_FS_CREATE:
            if (!read_string(payload, payload_size, &offset, 255u, 1) ||
                payload_size - offset != 8u) return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
            flags = read_u64_le(payload + offset);
            if (flags != 0 && flags != BAURA_FS_CREATE_NEW &&
                flags != BAURA_FS_CREATE_ALWAYS) return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
            return BAURA_IPC_V2_OK;
        case BAURA_ABI_OPCODE_FS_READ:
            if (payload_size != 36u) return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
            if (read_u64_le(payload + 8u) == 0u ||
                (read_u64_le(payload + 16u) != UINT64_MAX &&
                 read_u64_le(payload + 16u) > (uint64_t)INT64_MAX))
                return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
            if (read_u16_le(payload + 26u) != 0 ||
                read_u16_le(payload + 24u) != 0 ||
                read_u64_le(payload + 28u) > UINT64_MAX - read_u64_le(payload + 8u))
                return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
            return BAURA_IPC_V2_OK;
        case BAURA_ABI_OPCODE_PROCESS_CREATE:
            if (!read_string(payload, payload_size, &offset, 255u, 1) ||
                !read_string(payload, payload_size, &offset, 4096u, 0) ||
                payload_size - offset != 8u ||
                read_u64_le(payload + offset) != 0u) return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
            return BAURA_IPC_V2_OK;
        case BAURA_ABI_OPCODE_MEMORY_ALLOCATE:
            if (payload_size != 16u || read_u64_le(payload) == 0u ||
                read_u64_le(payload + 8u) != 0u) return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
            return BAURA_IPC_V2_OK;
        case BAURA_ABI_OPCODE_NETWORK_CONNECT:
            if (!read_string(payload, payload_size, &offset, 4096u, 0) ||
                read_u32_le(payload) == 0u || offset > payload_size || payload_size - offset != 8u ||
                read_u64_le(payload + offset) != 0u) return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
            return BAURA_IPC_V2_OK;
        default:
            return BAURA_IPC_V2_ERR_UNKNOWN_OPCODE;
    }
}

static BauraIpcV2Result validate_response_payload(
    const BauraIpcV2Header *header, const uint8_t *payload) {
    uint32_t status;
    uint32_t error;
    size_t expected_result_size;

    if (header->payload_bytes < 8u) return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
    status = read_u32_le(payload);
    error = read_u32_le(payload + 4u);

    if (status == (uint32_t)BAURA_ABI_STATUS_FAILURE) {
        if (error < (uint32_t)BAURA_ABI_ERR_INVALID_ARGUMENT ||
            error > (uint32_t)BAURA_ABI_ERR_INTERNAL ||
            header->payload_bytes != 8u || header->attachment_count != 0)
            return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
        return BAURA_IPC_V2_OK;
    }
    if (error != 0u) return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
    if (status == (uint32_t)BAURA_ABI_STATUS_PENDING) {
        if (header->kind != (uint16_t)BAURA_IPC_KIND_RESPONSE ||
            !is_non_blocking(header->opcode) ||
            header->payload_bytes != 8u || header->attachment_count != 0)
            return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
        return BAURA_IPC_V2_OK;
    }
    if (status != (uint32_t)BAURA_ABI_STATUS_SUCCESS)
        return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;

    switch (header->opcode) {
        case BAURA_ABI_OPCODE_NETWORK_CONNECT:
            expected_result_size = 0u;
            break;
        default:
            expected_result_size = 8u;
            break;
    }
    if (header->payload_bytes != 8u + expected_result_size ||
        header->attachment_count != expected_success_attachments(header->opcode))
        return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
    return BAURA_IPC_V2_OK;
}

static BauraIpcV2Result validate_header_and_payload(
    const BauraIpcV2Header *header, const uint8_t *payload) {
    BauraIpcV2Result result;
    if (header->major != (uint16_t)BAURA_ABI_VERSION_MAJOR ||
        header->minor != (uint16_t)BAURA_ABI_VERSION_MINOR)
        return BAURA_IPC_V2_ERR_UNSUPPORTED_VERSION;
    if (header->flags != BAURA_IPC_FLAGS_VALUE || header->request_id == 0u ||
        header->payload_bytes > BAURA_IPC_MAX_PAYLOAD) return BAURA_IPC_V2_ERR_MALFORMED_FRAME;
    if (!is_known_opcode(header->opcode)) return BAURA_IPC_V2_ERR_UNKNOWN_OPCODE;

    if (header->kind == (uint16_t)BAURA_IPC_KIND_REQUEST) {
        if (header->attachment_count != expected_request_attachments(header->opcode))
            return BAURA_IPC_V2_ERR_INVALID_PAYLOAD;
        return validate_request_payload(header->opcode, payload, header->payload_bytes);
    }
    if (header->kind == (uint16_t)BAURA_IPC_KIND_RESPONSE ||
        header->kind == (uint16_t)BAURA_IPC_KIND_COMPLETION) {
        return validate_response_payload(header, payload);
    }
    result = BAURA_IPC_V2_ERR_MALFORMED_FRAME;
    return result;
}

BauraIpcV2Result baura_ipc_v2_encode_frame(
    const BauraIpcV2Header *header,
    const uint8_t *payload,
    size_t payload_size,
    uint8_t *frame,
    size_t frame_capacity,
    size_t *frame_size) {
    BauraIpcV2Result result;
    size_t required_size;
    uint32_t index;

    if (header == 0 || frame == 0 || frame_size == 0 ||
        (payload == 0 && payload_size != 0u)) return BAURA_IPC_V2_ERR_INVALID_ARGUMENT;
    if (payload_size > BAURA_IPC_MAX_PAYLOAD ||
        header->payload_bytes != payload_size) return BAURA_IPC_V2_ERR_MALFORMED_FRAME;
    result = validate_header_and_payload(header, payload);
    if (result != BAURA_IPC_V2_OK) return result;
    required_size = BAURA_IPC_HEADER_SIZE + payload_size;
    if (frame_capacity < required_size) return BAURA_IPC_V2_ERR_BUFFER_TOO_SMALL;

    write_u32_le(frame + BAURA_IPC_MAGIC_OFFSET, BAURA_IPC_MAGIC);
    write_u16_le(frame + BAURA_IPC_MAJOR_OFFSET, header->major);
    write_u16_le(frame + BAURA_IPC_MINOR_OFFSET, header->minor);
    write_u16_le(frame + BAURA_IPC_KIND_OFFSET, header->kind);
    write_u16_le(frame + BAURA_IPC_FLAGS_OFFSET, header->flags);
    write_u32_le(frame + BAURA_IPC_OPCODE_OFFSET, header->opcode);
    write_u64_le(frame + BAURA_IPC_REQUEST_ID_OFFSET, header->request_id);
    write_u32_le(frame + BAURA_IPC_PAYLOAD_SIZE_OFFSET, header->payload_bytes);
    write_u16_le(frame + BAURA_IPC_ATTACHMENT_COUNT_OFFSET, header->attachment_count);
    write_u16_le(frame + BAURA_IPC_RESERVED_OFFSET, BAURA_IPC_RESERVED_VALUE);
    for (index = 0; index < payload_size; ++index)
        frame[BAURA_IPC_HEADER_SIZE + index] = payload[index];
    *frame_size = required_size;
    return BAURA_IPC_V2_OK;
}

BauraIpcV2Result baura_ipc_v2_decode_frame(
    const uint8_t *frame,
    size_t frame_size,
    BauraIpcV2Header *header,
    const uint8_t **payload) {
    uint32_t payload_size;
    BauraIpcV2Result result;

    if (frame == 0 || header == 0 || payload == 0)
        return BAURA_IPC_V2_ERR_INVALID_ARGUMENT;
    if (frame_size < BAURA_IPC_HEADER_SIZE ||
        read_u32_le(frame + BAURA_IPC_MAGIC_OFFSET) != BAURA_IPC_MAGIC ||
        read_u16_le(frame + BAURA_IPC_RESERVED_OFFSET) != BAURA_IPC_RESERVED_VALUE)
        return BAURA_IPC_V2_ERR_MALFORMED_FRAME;

    header->major = read_u16_le(frame + BAURA_IPC_MAJOR_OFFSET);
    header->minor = read_u16_le(frame + BAURA_IPC_MINOR_OFFSET);
    header->kind = read_u16_le(frame + BAURA_IPC_KIND_OFFSET);
    header->flags = read_u16_le(frame + BAURA_IPC_FLAGS_OFFSET);
    header->opcode = read_u32_le(frame + BAURA_IPC_OPCODE_OFFSET);
    header->request_id = read_u64_le(frame + BAURA_IPC_REQUEST_ID_OFFSET);
    payload_size = read_u32_le(frame + BAURA_IPC_PAYLOAD_SIZE_OFFSET);
    header->payload_bytes = payload_size;
    header->attachment_count = read_u16_le(frame + BAURA_IPC_ATTACHMENT_COUNT_OFFSET);

    if (payload_size > BAURA_IPC_MAX_PAYLOAD ||
        frame_size - BAURA_IPC_HEADER_SIZE != payload_size)
        return BAURA_IPC_V2_ERR_MALFORMED_FRAME;
    *payload = frame + BAURA_IPC_HEADER_SIZE;
    result = validate_header_and_payload(header, *payload);
    return result;
}
