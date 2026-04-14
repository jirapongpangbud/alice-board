#ifndef ALICE_GATEWAY_ALICE_GATEWAY_H
#define ALICE_GATEWAY_ALICE_GATEWAY_H

#include <stddef.h>
#include <stdint.h>

#include "shared/protocol/alice_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ALICE_GATEWAY_DEFAULT_FIRST_RUNTIME_ADDRESS = 1u,
    ALICE_GATEWAY_DEFAULT_LAST_RUNTIME_ADDRESS = 100u,
    ALICE_GATEWAY_DEFAULT_STATUS_POLL_ATTEMPTS = 3u,
    ALICE_GATEWAY_MAX_REQUEST_PAYLOAD_SIZE = ALICE_CLAIM_RUNTIME_ADDRESS_REQUEST_SIZE,
    ALICE_GATEWAY_MAX_RESPONSE_PAYLOAD_SIZE = ALICE_GET_NEIGHBOR_REPORT_RESPONSE_SIZE
};

typedef enum alice_gateway_phase_e {
    ALICE_GATEWAY_PHASE_IDLE = 0u,
    ALICE_GATEWAY_PHASE_RESETTING = 1u,
    ALICE_GATEWAY_PHASE_CLAIMING = 2u,
    ALICE_GATEWAY_PHASE_PROBING = 3u,
    ALICE_GATEWAY_PHASE_POLLING = 4u,
    ALICE_GATEWAY_PHASE_COLLECTING = 5u,
    ALICE_GATEWAY_PHASE_COMPLETE = 6u,
    ALICE_GATEWAY_PHASE_FAULT = 7u
} alice_gateway_phase_t;

typedef enum alice_gateway_transport_result_e {
    ALICE_GATEWAY_TRANSPORT_OK = 0u,
    ALICE_GATEWAY_TRANSPORT_NO_DEVICE = 1u,
    ALICE_GATEWAY_TRANSPORT_BUS_ERROR = 2u,
    ALICE_GATEWAY_TRANSPORT_MALFORMED_RESPONSE = 3u
} alice_gateway_transport_result_t;

enum {
    ALICE_GATEWAY_TILE_FLAG_NONE = 0x0000u,
    ALICE_GATEWAY_TILE_FLAG_CLAIMED = 0x0001u,
    ALICE_GATEWAY_TILE_FLAG_IDENTITY_VALID = 0x0002u,
    ALICE_GATEWAY_TILE_FLAG_STATUS_VALID = 0x0004u,
    ALICE_GATEWAY_TILE_FLAG_REPORT_VALID = 0x0008u,
    ALICE_GATEWAY_TILE_FLAG_HAS_FAULT = 0x0010u,
    ALICE_GATEWAY_TILE_FLAG_DUPLICATE_UID = 0x0020u,
    ALICE_GATEWAY_TILE_FLAG_PROTOCOL_MISMATCH = 0x0040u,
    ALICE_GATEWAY_TILE_FLAG_TRANSPORT_ERROR = 0x0080u,
    ALICE_GATEWAY_TILE_FLAG_STATUS_TIMEOUT = 0x0100u
};

typedef struct alice_gateway_request_s {
    alice_request_header_t header;
    uint8_t payload[ALICE_GATEWAY_MAX_REQUEST_PAYLOAD_SIZE];
} alice_gateway_request_t;

typedef union alice_gateway_response_payload_u {
    alice_reset_scan_state_response_t reset_scan_state;
    alice_claim_runtime_address_response_t claim_runtime_address;
    alice_get_tile_identity_response_t get_tile_identity;
    alice_get_scan_status_response_t get_scan_status;
    alice_get_neighbor_report_response_t get_neighbor_report;
    alice_ping_response_t ping;
    uint8_t raw[ALICE_GATEWAY_MAX_RESPONSE_PAYLOAD_SIZE];
} alice_gateway_response_payload_t;

typedef struct alice_gateway_response_s {
    alice_response_header_t header;
    alice_gateway_response_payload_t payload;
} alice_gateway_response_t;

typedef struct alice_gateway_tile_record_s {
    alice_tile_uid_t tile_uid;
    alice_runtime_address_t runtime_address;
    uint8_t protocol_version;
    uint8_t scan_state;
    uint16_t scan_generation;
    uint16_t tile_flags;
    uint16_t tile_error_flags;
    uint16_t report_flags;
    alice_neighbor_report_t neighbors[ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT];
} alice_gateway_tile_record_t;

typedef struct alice_gateway_snapshot_s {
    uint32_t snapshot_id;
    uint16_t scan_generation;
    uint16_t tile_count;
    uint16_t fault_count;
    alice_gateway_tile_record_t tiles[ALICE_PROTOCOL_MAX_TILES];
} alice_gateway_snapshot_t;

typedef alice_gateway_transport_result_t (*alice_gateway_reset_all_fn)(
    void *context,
    uint16_t scan_generation);

typedef alice_gateway_transport_result_t (*alice_gateway_claim_address_fn)(
    void *context,
    uint16_t scan_generation,
    const alice_gateway_request_t *request,
    alice_gateway_response_t *response);

typedef alice_gateway_transport_result_t (*alice_gateway_transact_fn)(
    void *context,
    alice_runtime_address_t runtime_address,
    const alice_gateway_request_t *request,
    alice_gateway_response_t *response);

typedef struct alice_gateway_transport_ops_s {
    alice_gateway_reset_all_fn reset_all;
    alice_gateway_claim_address_fn claim_address;
    alice_gateway_transact_fn transact;
} alice_gateway_transport_ops_t;

typedef struct alice_gateway_config_s {
    alice_runtime_address_t first_runtime_address;
    alice_runtime_address_t last_runtime_address;
    uint8_t status_poll_attempts;
    void *transport_context;
    alice_gateway_transport_ops_t transport;
} alice_gateway_config_t;

typedef struct alice_gateway_s {
    alice_gateway_config_t config;
    alice_gateway_snapshot_t snapshot;
    uint8_t phase;
    uint8_t last_transport_result;
    uint8_t last_status_code;
} alice_gateway_t;

void alice_gateway_init(alice_gateway_t *gateway, const alice_gateway_config_t *config);

const alice_gateway_snapshot_t *alice_gateway_get_snapshot(const alice_gateway_t *gateway);

uint8_t alice_gateway_get_phase(const alice_gateway_t *gateway);

alice_status_code_t alice_gateway_run_discovery(alice_gateway_t *gateway);

#ifdef __cplusplus
}
#endif

#endif
