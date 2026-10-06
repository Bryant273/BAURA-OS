#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../SYSTEM_API/baura_ipc_v2.h"

#define TEST_FRAME_CAPACITY (BAURA_IPC_HEADER_SIZE + BAURA_IPC_MAX_PAYLOAD)

static void put_u32_le(uint8_t *bytes, uint32_t value) {
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
    bytes[2] = (uint8_t)(value >> 16);
    bytes[3] = (uint8_t)(value >> 24);
}

static void put_u64_le(uint8_t *bytes, uint64_t value) {
    uint32_t index;
    for (index = 0; index < 8; ++index) bytes[index] = (uint8_t)(value >> (index * 8));
}

static uint32_t get_u32_le(const uint8_t *bytes) {
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static BauraIpcV2Header request_header(uint32_t opcode, uint32_t payload_bytes,
                                       uint16_t attachment_count) {
    BauraIpcV2Header header;
    header.major = BAURA_ABI_VERSION_MAJOR;
    header.minor = BAURA_ABI_VERSION_MINOR;
    header.kind = BAURA_IPC_KIND_REQUEST;
    header.flags = 0;
    header.opcode = opcode;
    header.request_id = 42;
    header.payload_bytes = payload_bytes;
    header.attachment_count = attachment_count;
    return header;
}

static void test_network_frame_round_trip(void) {
    static const uint8_t endpoint[] = "host:443";
    uint8_t request[20] = {0};
    uint8_t frame[TEST_FRAME_CAPACITY];
    size_t frame_size = 0;
    const uint8_t *decoded_payload = 0;
    BauraIpcV2Header header = request_header(
        BAURA_ABI_OPCODE_NETWORK_CONNECT, sizeof(request), 0);
    BauraIpcV2Header decoded;

    put_u32_le(request, (uint32_t)(sizeof(endpoint) - 1u));
    memcpy(request + 4, endpoint, sizeof(endpoint) - 1u);
    put_u64_le(request + 12, 0);
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame), &frame_size) == BAURA_IPC_V2_OK);
    assert(frame_size == BAURA_IPC_HEADER_SIZE + sizeof(request));
    assert(frame[0] == 'B' && frame[1] == 'A' && frame[2] == 'U' && frame[3] == 'R');
    assert(frame[BAURA_IPC_MAJOR_OFFSET] == 2);
    assert(frame[BAURA_IPC_MINOR_OFFSET] == 0);
    assert(frame[BAURA_IPC_KIND_OFFSET] == (uint8_t)BAURA_IPC_KIND_REQUEST);
    assert(frame[BAURA_IPC_FLAGS_OFFSET] == 0);
    assert(frame[BAURA_IPC_OPCODE_OFFSET] == 1);
    assert(frame[BAURA_IPC_OPCODE_OFFSET + 1u] == 0);
    assert(frame[BAURA_IPC_REQUEST_ID_OFFSET] == 42);
    assert(frame[BAURA_IPC_PAYLOAD_SIZE_OFFSET] == (uint8_t)sizeof(request));
    assert(frame[BAURA_IPC_RESERVED_OFFSET] == 0);
    assert(baura_ipc_v2_decode_frame(frame, frame_size, &decoded,
                                     &decoded_payload) == BAURA_IPC_V2_OK);
    assert(decoded.kind == (uint16_t)BAURA_IPC_KIND_REQUEST);
    assert(decoded.opcode == BAURA_ABI_OPCODE_NETWORK_CONNECT);
    assert(decoded.request_id == 42u);
    assert(decoded_payload == frame + BAURA_IPC_HEADER_SIZE);
    assert(memcmp(decoded_payload, request, sizeof(request)) == 0);
}

static void test_filesystem_create_payload(void) {
    static const uint8_t path[] = "PC/C:/a";
    uint8_t request[19] = {0};
    uint8_t frame[BAURA_IPC_HEADER_SIZE + sizeof(request)];
    size_t frame_size;
    BauraIpcV2Header header = request_header(
        BAURA_ABI_OPCODE_FS_CREATE, sizeof(request), 0);

    put_u32_le(request, (uint32_t)(sizeof(path) - 1u));
    memcpy(request + 4, path, sizeof(path) - 1u);
    put_u64_le(request + 11, BAURA_FS_CREATE_NEW);
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame), &frame_size) == BAURA_IPC_V2_OK);

    put_u64_le(request + 11, BAURA_FS_CREATE_NEW | BAURA_FS_CREATE_ALWAYS);
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);
    put_u64_le(request + 11, 0);
    request[4] = 'X';
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);
}

static void test_filesystem_read_payload(void) {
    uint8_t request[36] = {0};
    uint8_t frame[BAURA_IPC_HEADER_SIZE + sizeof(request)];
    size_t frame_size;
    BauraIpcV2Header header = request_header(
        BAURA_ABI_OPCODE_FS_READ, sizeof(request), 1);

    put_u64_le(request, 8);
    put_u64_le(request + 8, 16);
    put_u64_le(request + 16, UINT64_MAX);
    put_u64_le(request + 28, 32);
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame), &frame_size) == BAURA_IPC_V2_OK);

    put_u64_le(request + 16, UINT64_MAX - 1u);
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);
    put_u64_le(request + 16, UINT64_MAX);
    put_u64_le(request + 28, UINT64_MAX - 7u);
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);

    header.attachment_count = 0;
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);
}

static void test_process_and_memory_requests(void) {
    static const uint8_t path[] = "PC/C:/app";
    static const uint8_t arguments[] = "x=1";
    uint8_t process_request[28] = {0};
    uint8_t memory_request[16] = {0};
    uint8_t frame[BAURA_IPC_HEADER_SIZE + sizeof(process_request)];
    size_t frame_size;
    BauraIpcV2Header header;

    put_u32_le(process_request, (uint32_t)(sizeof(path) - 1u));
    memcpy(process_request + 4, path, sizeof(path) - 1u);
    put_u32_le(process_request + 13, (uint32_t)(sizeof(arguments) - 1u));
    memcpy(process_request + 17, arguments, sizeof(arguments) - 1u);
    header = request_header(BAURA_ABI_OPCODE_PROCESS_CREATE,
                            sizeof(process_request), 0);
    assert(baura_ipc_v2_encode_frame(&header, process_request,
                                     sizeof(process_request), frame,
                                     sizeof(frame), &frame_size) == BAURA_IPC_V2_OK);

    process_request[17] = 0xFFu;
    assert(baura_ipc_v2_encode_frame(&header, process_request,
                                     sizeof(process_request), frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);

    put_u64_le(memory_request, 4096);
    header = request_header(BAURA_ABI_OPCODE_MEMORY_ALLOCATE,
                            sizeof(memory_request), 0);
    assert(baura_ipc_v2_encode_frame(&header, memory_request,
                                     sizeof(memory_request), frame,
                                     sizeof(frame), &frame_size) == BAURA_IPC_V2_OK);
    put_u64_le(memory_request + 8, 1);
    assert(baura_ipc_v2_encode_frame(&header, memory_request,
                                     sizeof(memory_request), frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);
}

static void test_response_validation(void) {
    uint8_t response_payload[16] = {0};
    uint8_t frame[BAURA_IPC_HEADER_SIZE + sizeof(response_payload)];
    size_t frame_size;
    const uint8_t *decoded_payload;
    BauraIpcV2Header header;
    BauraIpcV2Header decoded;

    header = request_header(BAURA_ABI_OPCODE_MEMORY_ALLOCATE,
                            sizeof(response_payload), 1);
    header.kind = BAURA_IPC_KIND_RESPONSE;
    put_u32_le(response_payload, BAURA_ABI_STATUS_SUCCESS);
    put_u32_le(response_payload + 4, 0);
    put_u64_le(response_payload + 8, 4096);
    assert(baura_ipc_v2_encode_frame(&header, response_payload,
                                     sizeof(response_payload), frame,
                                     sizeof(frame), &frame_size) == BAURA_IPC_V2_OK);
    assert(baura_ipc_v2_decode_frame(frame, frame_size, &decoded,
                                     &decoded_payload) == BAURA_IPC_V2_OK);

    header.opcode = BAURA_ABI_OPCODE_NETWORK_CONNECT;
    header.attachment_count = 0;
    assert(baura_ipc_v2_encode_frame(&header, response_payload,
                                     sizeof(response_payload), frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);

    header = request_header(BAURA_ABI_OPCODE_NETWORK_CONNECT, 8, 0);
    header.kind = BAURA_IPC_KIND_RESPONSE;
    put_u32_le(response_payload, BAURA_ABI_STATUS_FAILURE);
    put_u32_le(response_payload + 4, BAURA_ABI_ERR_TIMEOUT);
    assert(baura_ipc_v2_encode_frame(&header, response_payload, 8, frame,
                                     sizeof(frame), &frame_size) == BAURA_IPC_V2_OK);
    put_u32_le(response_payload + 4, 0x7FFFu);
    assert(baura_ipc_v2_encode_frame(&header, response_payload, 8, frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);
}

static void test_pending_and_completion(void) {
    uint8_t response_payload[16] = {0};
    uint8_t frame[BAURA_IPC_HEADER_SIZE + sizeof(response_payload)];
    size_t frame_size;
    const uint8_t *payload;
    BauraIpcV2Header header = request_header(
        BAURA_ABI_OPCODE_PROCESS_CREATE, 8, 0);

    header.kind = BAURA_IPC_KIND_RESPONSE;
    put_u32_le(response_payload, BAURA_ABI_STATUS_PENDING);
    put_u32_le(response_payload + 4, 0);
    assert(baura_ipc_v2_encode_frame(&header, response_payload, 8, frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_OK);

    header.kind = BAURA_IPC_KIND_COMPLETION;
    header.payload_bytes = sizeof(response_payload);
    put_u32_le(response_payload, BAURA_ABI_STATUS_SUCCESS);
    put_u64_le(response_payload + 8, 73);
    assert(baura_ipc_v2_encode_frame(&header, response_payload,
                                     sizeof(response_payload), frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_OK);
    assert(baura_ipc_v2_decode_frame(frame, frame_size, &header, &payload) ==
           BAURA_IPC_V2_OK);
    assert(get_u32_le(payload) == (uint32_t)BAURA_ABI_STATUS_SUCCESS);

    put_u32_le(response_payload, BAURA_ABI_STATUS_PENDING);
    header.payload_bytes = 8;
    assert(baura_ipc_v2_encode_frame(&header, response_payload, 8, frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);
}

static void test_malformed_frames(void) {
    static const uint8_t endpoint[] = "host:443";
    uint8_t request[20] = {0};
    uint8_t frame[TEST_FRAME_CAPACITY];
    size_t frame_size;
    const uint8_t *payload;
    BauraIpcV2Header header = request_header(
        BAURA_ABI_OPCODE_NETWORK_CONNECT, sizeof(request), 0);
    BauraIpcV2Header decoded;

    put_u32_le(request, (uint32_t)(sizeof(endpoint) - 1u));
    memcpy(request + 4, endpoint, sizeof(endpoint) - 1u);
    put_u64_le(request + 12, 0);
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame), &frame_size) == BAURA_IPC_V2_OK);
    assert(baura_ipc_v2_decode_frame(frame, frame_size - 1u, &decoded, &payload) ==
           BAURA_IPC_V2_ERR_MALFORMED_FRAME);
    assert(baura_ipc_v2_decode_frame(frame, frame_size + 1u, &decoded, &payload) ==
           BAURA_IPC_V2_ERR_MALFORMED_FRAME);

    frame[0] ^= 1u;
    assert(baura_ipc_v2_decode_frame(frame, frame_size, &decoded, &payload) ==
           BAURA_IPC_V2_ERR_MALFORMED_FRAME);
    frame[0] ^= 1u;

    frame[BAURA_IPC_MAJOR_OFFSET] = 3u;
    assert(baura_ipc_v2_decode_frame(frame, frame_size, &decoded, &payload) ==
           BAURA_IPC_V2_ERR_UNSUPPORTED_VERSION);
    frame[BAURA_IPC_MAJOR_OFFSET] = 2u;

    frame[BAURA_IPC_FLAGS_OFFSET] = 1u;
    assert(baura_ipc_v2_decode_frame(frame, frame_size, &decoded, &payload) ==
           BAURA_IPC_V2_ERR_MALFORMED_FRAME);
    frame[BAURA_IPC_FLAGS_OFFSET] = 0u;

    frame[BAURA_IPC_RESERVED_OFFSET] = 1u;
    assert(baura_ipc_v2_decode_frame(frame, frame_size, &decoded, &payload) ==
           BAURA_IPC_V2_ERR_MALFORMED_FRAME);
    frame[BAURA_IPC_RESERVED_OFFSET] = 0u;

    frame[BAURA_IPC_REQUEST_ID_OFFSET] = 0u;
    frame[BAURA_IPC_REQUEST_ID_OFFSET + 1u] = 0u;
    frame[BAURA_IPC_REQUEST_ID_OFFSET + 2u] = 0u;
    frame[BAURA_IPC_REQUEST_ID_OFFSET + 3u] = 0u;
    frame[BAURA_IPC_REQUEST_ID_OFFSET + 4u] = 0u;
    frame[BAURA_IPC_REQUEST_ID_OFFSET + 5u] = 0u;
    frame[BAURA_IPC_REQUEST_ID_OFFSET + 6u] = 0u;
    frame[BAURA_IPC_REQUEST_ID_OFFSET + 7u] = 0u;
    assert(baura_ipc_v2_decode_frame(frame, frame_size, &decoded, &payload) ==
           BAURA_IPC_V2_ERR_MALFORMED_FRAME);
}

static void test_encoder_arguments_and_attachment_count(void) {
    static const uint8_t endpoint[] = "host:443";
    uint8_t request[20] = {0};
    uint8_t frame[BAURA_IPC_HEADER_SIZE + sizeof(request)];
    size_t frame_size;
    BauraIpcV2Header header = request_header(
        BAURA_ABI_OPCODE_NETWORK_CONNECT, sizeof(request), 0);

    put_u32_le(request, (uint32_t)(sizeof(endpoint) - 1u));
    memcpy(request + 4, endpoint, sizeof(endpoint) - 1u);
    put_u64_le(request + 12, 0);
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame) - 1u, &frame_size) ==
           BAURA_IPC_V2_ERR_BUFFER_TOO_SMALL);
    header.attachment_count = 1;
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame), &frame_size) ==
           BAURA_IPC_V2_ERR_INVALID_PAYLOAD);

    header = request_header(BAURA_ABI_OPCODE_NETWORK_CONNECT, sizeof(request), 0);
    header.opcode = UINT32_C(0xFFFFFFFF);
    assert(baura_ipc_v2_encode_frame(&header, request, sizeof(request), frame,
                                     sizeof(frame),
                                     &frame_size) == BAURA_IPC_V2_ERR_UNKNOWN_OPCODE);
}

int main(void) {
    test_network_frame_round_trip();
    test_filesystem_create_payload();
    test_filesystem_read_payload();
    test_process_and_memory_requests();
    test_response_validation();
    test_pending_and_completion();
    test_malformed_frames();
    test_encoder_arguments_and_attachment_count();
    return 0;
}
