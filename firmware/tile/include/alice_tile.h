#ifndef ALICE_TILE_ALICE_TILE_H
#define ALICE_TILE_ALICE_TILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "shared/protocol/alice_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ALICE_TILE_UNASSIGNED_RUNTIME_ADDRESS = 0u,
    ALICE_TILE_MAX_RESPONSE_PAYLOAD_SIZE = ALICE_GET_NEIGHBOR_REPORT_RESPONSE_SIZE
};

typedef struct alice_tile_probe_result_s {
    alice_neighbor_report_t neighbors[ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT];
    uint8_t progress_flags;
    uint16_t tile_error_flags;
} alice_tile_probe_result_t;

typedef alice_status_code_t (*alice_tile_begin_probe_fn)(
    void *context,
    alice_tile_uid_t tile_uid,
    uint16_t scan_generation,
    alice_tile_probe_result_t *result);

typedef struct alice_tile_config_s {
    alice_tile_uid_t tile_uid;
    uint16_t capability_flags;
    void *probe_context;
    alice_tile_begin_probe_fn begin_probe;
} alice_tile_config_t;

typedef struct alice_tile_state_s {
    alice_runtime_address_t runtime_address;
    uint16_t scan_generation;
    uint16_t tile_error_flags;
    uint16_t report_flags;
    uint8_t scan_state;
    uint8_t progress_flags;
    alice_neighbor_report_t neighbors[ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT];
} alice_tile_state_t;

typedef struct alice_tile_s {
    alice_tile_config_t config;
    alice_tile_state_t state;
} alice_tile_t;

typedef struct alice_tile_request_s {
    alice_request_header_t header;
    const uint8_t *payload;
} alice_tile_request_t;

typedef union alice_tile_response_payload_u {
    alice_reset_scan_state_response_t reset_scan_state;
    alice_claim_runtime_address_response_t claim_runtime_address;
    alice_get_tile_identity_response_t get_tile_identity;
    alice_get_scan_status_response_t get_scan_status;
    alice_get_neighbor_report_response_t get_neighbor_report;
    alice_ping_response_t ping;
    uint8_t raw[ALICE_TILE_MAX_RESPONSE_PAYLOAD_SIZE];
} alice_tile_response_payload_t;

typedef struct alice_tile_response_s {
    alice_response_header_t header;
    alice_tile_response_payload_t payload;
} alice_tile_response_t;

void alice_tile_init_probe_result(alice_tile_probe_result_t *result);

void alice_tile_init(alice_tile_t *tile, const alice_tile_config_t *config);

const alice_tile_state_t *alice_tile_get_state(const alice_tile_t *tile);

bool alice_tile_scan_generation_matches(const alice_tile_t *tile, uint16_t scan_generation);

alice_status_code_t alice_tile_complete_probe(
    alice_tile_t *tile,
    alice_status_code_t probe_status,
    const alice_tile_probe_result_t *result);

alice_status_code_t alice_tile_handle_request(
    alice_tile_t *tile,
    const alice_tile_request_t *request,
    alice_tile_response_t *response);

#ifdef __cplusplus
}
#endif

#endif
