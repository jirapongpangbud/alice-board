#ifndef ALICE_PROTOCOL_ALICE_PROTOCOL_H
#define ALICE_PROTOCOL_ALICE_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define ALICE_PROTOCOL_PACKED __attribute__((packed))
#else
#define ALICE_PROTOCOL_PACKED
#endif

#if defined(__cplusplus)
#define ALICE_PROTOCOL_STATIC_ASSERT(condition, message) static_assert((condition), message)
#else
#define ALICE_PROTOCOL_STATIC_ASSERT(condition, message) _Static_assert((condition), message)
#endif

typedef uint64_t alice_tile_uid_t;
typedef uint8_t alice_runtime_address_t;

enum {
    ALICE_PROTOCOL_VERSION = 1u,
    ALICE_PROTOCOL_MAX_TILES = 100u,
    ALICE_PROTOCOL_DIRECTION_COUNT = 4u,
    ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT = 4u,
    ALICE_PROTOCOL_MAX_NEIGHBORS_PER_TILE = 4u
};

typedef enum alice_direction_e {
    ALICE_DIRECTION_NORTH = 0u,
    ALICE_DIRECTION_EAST = 1u,
    ALICE_DIRECTION_SOUTH = 2u,
    ALICE_DIRECTION_WEST = 3u
} alice_direction_t;

typedef enum alice_edge_state_e {
    ALICE_EDGE_STATE_UNKNOWN = 0u,
    ALICE_EDGE_STATE_OPEN = 1u,
    ALICE_EDGE_STATE_NEIGHBOR_DETECTED = 2u,
    ALICE_EDGE_STATE_NEIGHBOR_CONFIRMED = 3u,
    ALICE_EDGE_STATE_HANDSHAKE_FAILED = 4u,
    ALICE_EDGE_STATE_TIMEOUT = 5u,
    ALICE_EDGE_STATE_CONFLICT = 6u,
    ALICE_EDGE_STATE_RESERVED = 7u
} alice_edge_state_t;

typedef enum alice_command_e {
    ALICE_COMMAND_RESET_SCAN_STATE = 0x01u,
    ALICE_COMMAND_CLAIM_RUNTIME_ADDRESS = 0x02u,
    ALICE_COMMAND_GET_TILE_IDENTITY = 0x03u,
    ALICE_COMMAND_START_EDGE_PROBE = 0x04u,
    ALICE_COMMAND_GET_SCAN_STATUS = 0x05u,
    ALICE_COMMAND_GET_NEIGHBOR_REPORT = 0x06u,
    ALICE_COMMAND_PING = 0x07u
} alice_command_t;

typedef enum alice_status_code_e {
    ALICE_STATUS_OK = 0x00u,
    ALICE_STATUS_BUSY = 0x01u,
    ALICE_STATUS_INVALID_COMMAND = 0x02u,
    ALICE_STATUS_INVALID_SCAN_GENERATION = 0x03u,
    ALICE_STATUS_INVALID_STATE = 0x04u,
    ALICE_STATUS_PAYLOAD_ERROR = 0x05u,
    ALICE_STATUS_EDGE_TIMEOUT = 0x06u,
    ALICE_STATUS_EDGE_HANDSHAKE_FAILED = 0x07u,
    ALICE_STATUS_ADDRESS_CONFLICT = 0x08u,
    ALICE_STATUS_INTERNAL_FAULT = 0x09u
} alice_status_code_t;

typedef enum alice_scan_state_e {
    ALICE_SCAN_STATE_IDLE = 0x00u,
    ALICE_SCAN_STATE_AWAITING_PROBE = 0x01u,
    ALICE_SCAN_STATE_PROBING = 0x02u,
    ALICE_SCAN_STATE_READY = 0x03u,
    ALICE_SCAN_STATE_FAULT = 0x04u
} alice_scan_state_t;

typedef enum alice_claim_result_e {
    ALICE_CLAIM_RESULT_ACCEPTED = 0x00u,
    ALICE_CLAIM_RESULT_REJECTED = 0x01u,
    ALICE_CLAIM_RESULT_COLLISION = 0x02u,
    ALICE_CLAIM_RESULT_BUSY = 0x03u
} alice_claim_result_t;

enum {
    ALICE_CAPABILITY_NONE = 0x0000u,
    ALICE_CAPABILITY_DIRECTIONAL_PROBE = 0x0001u,
    ALICE_CAPABILITY_SIGNAL_QUALITY = 0x0002u
};

enum {
    ALICE_RESET_FLAG_NONE = 0x0000u,
    ALICE_RESET_FLAG_CACHE_CLEARED = 0x0001u,
    ALICE_RESET_FLAG_RUNTIME_ADDRESS_RELEASED = 0x0002u
};

enum {
    ALICE_TILE_ERROR_NONE = 0x0000u,
    ALICE_TILE_ERROR_SCAN_GENERATION_MISMATCH = 0x0001u,
    ALICE_TILE_ERROR_ADDRESS_CONFLICT = 0x0002u,
    ALICE_TILE_ERROR_PROBE_INCOMPLETE = 0x0004u,
    ALICE_TILE_ERROR_INTERNAL_FAULT = 0x0008u
};

enum {
    ALICE_REPORT_FLAG_NONE = 0x0000u,
    ALICE_REPORT_FLAG_COMPLETE = 0x0001u,
    ALICE_REPORT_FLAG_HAS_FAULT = 0x0002u,
    ALICE_REPORT_FLAG_SCAN_GENERATION_MATCH = 0x0004u
};

enum {
    ALICE_EDGE_ERROR_NONE = 0x00u,
    ALICE_EDGE_ERROR_SIGNAL_INVALID = 0x01u,
    ALICE_EDGE_ERROR_HANDSHAKE_FAILED = 0x02u,
    ALICE_EDGE_ERROR_TIMEOUT = 0x04u,
    ALICE_EDGE_ERROR_REMOTE_UID_INVALID = 0x08u
};

enum {
    ALICE_PROGRESS_FLAG_NONE = 0x00u,
    ALICE_PROGRESS_FLAG_NORTH_DONE = 0x01u,
    ALICE_PROGRESS_FLAG_EAST_DONE = 0x02u,
    ALICE_PROGRESS_FLAG_SOUTH_DONE = 0x04u,
    ALICE_PROGRESS_FLAG_WEST_DONE = 0x08u
};

enum {
    ALICE_HEALTH_FLAG_NONE = 0x00u,
    ALICE_HEALTH_FLAG_RESPONSIVE = 0x01u,
    ALICE_HEALTH_FLAG_FAULT = 0x02u
};

typedef struct ALICE_PROTOCOL_PACKED alice_u16_le_s {
    uint8_t bytes[2];
} alice_u16_le_t;

typedef struct ALICE_PROTOCOL_PACKED alice_u64_le_s {
    uint8_t bytes[8];
} alice_u64_le_t;

static inline alice_u16_le_t alice_u16_to_le(uint16_t value)
{
    alice_u16_le_t encoded = {{
        (uint8_t)(value & 0xffu),
        (uint8_t)((value >> 8) & 0xffu)
    }};

    return encoded;
}

static inline uint16_t alice_u16_from_le(alice_u16_le_t encoded)
{
    return (uint16_t)encoded.bytes[0]
        | (uint16_t)((uint16_t)encoded.bytes[1] << 8);
}

static inline alice_u64_le_t alice_u64_to_le(uint64_t value)
{
    alice_u64_le_t encoded = {{
        (uint8_t)(value & 0xffu),
        (uint8_t)((value >> 8) & 0xffu),
        (uint8_t)((value >> 16) & 0xffu),
        (uint8_t)((value >> 24) & 0xffu),
        (uint8_t)((value >> 32) & 0xffu),
        (uint8_t)((value >> 40) & 0xffu),
        (uint8_t)((value >> 48) & 0xffu),
        (uint8_t)((value >> 56) & 0xffu)
    }};

    return encoded;
}

static inline uint64_t alice_u64_from_le(alice_u64_le_t encoded)
{
    return (uint64_t)encoded.bytes[0]
        | ((uint64_t)encoded.bytes[1] << 8)
        | ((uint64_t)encoded.bytes[2] << 16)
        | ((uint64_t)encoded.bytes[3] << 24)
        | ((uint64_t)encoded.bytes[4] << 32)
        | ((uint64_t)encoded.bytes[5] << 40)
        | ((uint64_t)encoded.bytes[6] << 48)
        | ((uint64_t)encoded.bytes[7] << 56);
}

static inline uint8_t alice_direction_opposite(uint8_t direction)
{
    switch (direction) {
    case ALICE_DIRECTION_NORTH:
        return ALICE_DIRECTION_SOUTH;
    case ALICE_DIRECTION_EAST:
        return ALICE_DIRECTION_WEST;
    case ALICE_DIRECTION_SOUTH:
        return ALICE_DIRECTION_NORTH;
    case ALICE_DIRECTION_WEST:
        return ALICE_DIRECTION_EAST;
    default:
        return ALICE_PROTOCOL_DIRECTION_COUNT;
    }
}

typedef struct ALICE_PROTOCOL_PACKED alice_request_header_s {
    uint8_t command;
    alice_u16_le_t scan_generation;
    uint8_t payload_length;
} alice_request_header_t;

typedef struct ALICE_PROTOCOL_PACKED alice_response_header_s {
    uint8_t protocol_version;
    uint8_t message_type;
    uint8_t status_code;
    alice_u16_le_t scan_generation;
    uint8_t payload_length;
} alice_response_header_t;

typedef struct ALICE_PROTOCOL_PACKED alice_reset_scan_state_response_s {
    alice_u16_le_t reset_flags;
} alice_reset_scan_state_response_t;

typedef struct ALICE_PROTOCOL_PACKED alice_claim_runtime_address_request_s {
    uint8_t candidate_address;
    alice_u16_le_t claim_nonce;
} alice_claim_runtime_address_request_t;

typedef struct ALICE_PROTOCOL_PACKED alice_claim_runtime_address_response_s {
    alice_u64_le_t tile_uid;
    uint8_t claim_result;
} alice_claim_runtime_address_response_t;

typedef struct ALICE_PROTOCOL_PACKED alice_get_tile_identity_response_s {
    alice_u64_le_t tile_uid;
    uint8_t runtime_address;
    alice_u16_le_t capability_flags;
} alice_get_tile_identity_response_t;

typedef struct ALICE_PROTOCOL_PACKED alice_get_scan_status_response_s {
    uint8_t scan_state;
    uint8_t progress_flags;
    alice_u16_le_t tile_error_flags;
} alice_get_scan_status_response_t;

typedef struct ALICE_PROTOCOL_PACKED alice_neighbor_report_s {
    uint8_t direction;
    uint8_t edge_state;
    alice_u64_le_t neighbor_uid;
    uint8_t signal_quality;
    uint8_t edge_error_flags;
} alice_neighbor_report_t;

typedef struct ALICE_PROTOCOL_PACKED alice_get_neighbor_report_response_s {
    alice_u64_le_t tile_uid;
    uint8_t runtime_address;
    alice_u16_le_t report_flags;
    alice_neighbor_report_t neighbors[ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT];
} alice_get_neighbor_report_response_t;

typedef struct ALICE_PROTOCOL_PACKED alice_ping_response_s {
    uint8_t health_flags;
} alice_ping_response_t;

enum {
    ALICE_REQUEST_HEADER_SIZE = (int)sizeof(alice_request_header_t),
    ALICE_RESPONSE_HEADER_SIZE = (int)sizeof(alice_response_header_t),
    ALICE_RESET_SCAN_STATE_RESPONSE_SIZE = (int)sizeof(alice_reset_scan_state_response_t),
    ALICE_CLAIM_RUNTIME_ADDRESS_REQUEST_SIZE = (int)sizeof(alice_claim_runtime_address_request_t),
    ALICE_CLAIM_RUNTIME_ADDRESS_RESPONSE_SIZE = (int)sizeof(alice_claim_runtime_address_response_t),
    ALICE_GET_TILE_IDENTITY_RESPONSE_SIZE = (int)sizeof(alice_get_tile_identity_response_t),
    ALICE_GET_SCAN_STATUS_RESPONSE_SIZE = (int)sizeof(alice_get_scan_status_response_t),
    ALICE_NEIGHBOR_REPORT_SIZE = (int)sizeof(alice_neighbor_report_t),
    ALICE_GET_NEIGHBOR_REPORT_RESPONSE_SIZE = (int)sizeof(alice_get_neighbor_report_response_t),
    ALICE_PING_RESPONSE_SIZE = (int)sizeof(alice_ping_response_t)
};

ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_u16_le_t) == 2u, "alice_u16_le_t must be 2 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_u64_le_t) == 8u, "alice_u64_le_t must be 8 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_request_header_t) == 4u, "alice_request_header_t must be 4 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_response_header_t) == 6u, "alice_response_header_t must be 6 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_reset_scan_state_response_t) == 2u, "alice_reset_scan_state_response_t must be 2 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_claim_runtime_address_request_t) == 3u, "alice_claim_runtime_address_request_t must be 3 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_claim_runtime_address_response_t) == 9u, "alice_claim_runtime_address_response_t must be 9 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_get_tile_identity_response_t) == 11u, "alice_get_tile_identity_response_t must be 11 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_get_scan_status_response_t) == 4u, "alice_get_scan_status_response_t must be 4 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_neighbor_report_t) == 12u, "alice_neighbor_report_t must be 12 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_get_neighbor_report_response_t) == 59u, "alice_get_neighbor_report_response_t must be 59 bytes");
ALICE_PROTOCOL_STATIC_ASSERT(sizeof(alice_ping_response_t) == 1u, "alice_ping_response_t must be 1 byte");

#ifdef __cplusplus
}
#endif

#endif
