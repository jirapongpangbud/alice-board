#include "firmware/tile/include/alice_tile.h"

#include <string.h>

enum {
    ALICE_TILE_PROGRESS_COMPLETE =
        ALICE_PROGRESS_FLAG_NORTH_DONE |
        ALICE_PROGRESS_FLAG_EAST_DONE |
        ALICE_PROGRESS_FLAG_SOUTH_DONE |
        ALICE_PROGRESS_FLAG_WEST_DONE
};

static void alice_tile_reset_neighbor_report(alice_neighbor_report_t *report, uint8_t direction)
{
    report->direction = direction;
    report->edge_state = ALICE_EDGE_STATE_UNKNOWN;
    report->neighbor_uid = alice_u64_to_le(0u);
    report->signal_quality = 0u;
    report->edge_error_flags = ALICE_EDGE_ERROR_NONE;
}

static void alice_tile_clear_neighbors(alice_neighbor_report_t neighbors[ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT])
{
    uint8_t direction;

    for (direction = 0u; direction < ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT; ++direction) {
        alice_tile_reset_neighbor_report(&neighbors[direction], direction);
    }
}

static void alice_tile_copy_neighbors(
    alice_neighbor_report_t destination[ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT],
    const alice_neighbor_report_t source[ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT])
{
    uint8_t direction;

    if (source == NULL) {
        alice_tile_clear_neighbors(destination);
        return;
    }

    for (direction = 0u; direction < ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT; ++direction) {
        destination[direction] = source[direction];
        destination[direction].direction = direction;
    }
}

static void alice_tile_clear_scan_cache(alice_tile_t *tile)
{
    tile->state.tile_error_flags = ALICE_TILE_ERROR_NONE;
    tile->state.report_flags = ALICE_REPORT_FLAG_NONE;
    tile->state.progress_flags = ALICE_PROGRESS_FLAG_NONE;
    alice_tile_clear_neighbors(tile->state.neighbors);
}

static void alice_tile_prepare_response(
    alice_tile_response_t *response,
    uint8_t command,
    uint16_t scan_generation)
{
    memset(response, 0, sizeof(*response));
    response->header.protocol_version = ALICE_PROTOCOL_VERSION;
    response->header.message_type = command;
    response->header.status_code = ALICE_STATUS_OK;
    response->header.scan_generation = alice_u16_to_le(scan_generation);
    response->header.payload_length = 0u;
}

static void alice_tile_finish_response(
    alice_tile_response_t *response,
    uint16_t scan_generation,
    uint8_t status_code,
    uint8_t payload_length)
{
    response->header.status_code = status_code;
    response->header.scan_generation = alice_u16_to_le(scan_generation);
    response->header.payload_length = payload_length;
}

static bool alice_tile_payload_present(const alice_tile_request_t *request)
{
    return request->header.payload_length == 0u || request->payload != NULL;
}

static bool alice_tile_payload_size_is(
    const alice_tile_request_t *request,
    uint8_t expected_payload_length)
{
    return request->header.payload_length == expected_payload_length && alice_tile_payload_present(request);
}

static void alice_tile_build_default_probe_result(alice_tile_probe_result_t *result)
{
    uint8_t direction;

    alice_tile_init_probe_result(result);

    for (direction = 0u; direction < ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT; ++direction) {
        result->neighbors[direction].edge_state = ALICE_EDGE_STATE_OPEN;
    }

    result->progress_flags = ALICE_TILE_PROGRESS_COMPLETE;
}

static alice_status_code_t alice_tile_default_begin_probe(
    void *context,
    alice_tile_uid_t tile_uid,
    uint16_t scan_generation,
    alice_tile_probe_result_t *result)
{
    (void)context;
    (void)tile_uid;
    (void)scan_generation;

    alice_tile_build_default_probe_result(result);
    return ALICE_STATUS_OK;
}

static uint16_t alice_tile_status_to_error_flags(alice_status_code_t status)
{
    switch (status) {
    case ALICE_STATUS_EDGE_TIMEOUT:
    case ALICE_STATUS_EDGE_HANDSHAKE_FAILED:
        return ALICE_TILE_ERROR_PROBE_INCOMPLETE;
    case ALICE_STATUS_ADDRESS_CONFLICT:
        return ALICE_TILE_ERROR_ADDRESS_CONFLICT;
    case ALICE_STATUS_INTERNAL_FAULT:
        return ALICE_TILE_ERROR_INTERNAL_FAULT;
    default:
        return ALICE_TILE_ERROR_PROBE_INCOMPLETE;
    }
}

static alice_status_code_t alice_tile_handle_reset_scan_state(
    alice_tile_t *tile,
    const alice_tile_request_t *request,
    alice_tile_response_t *response,
    uint16_t request_scan_generation)
{
    const uint16_t reset_flags = ALICE_RESET_FLAG_CACHE_CLEARED | ALICE_RESET_FLAG_RUNTIME_ADDRESS_RELEASED;

    if (!alice_tile_payload_present(request) || request->header.payload_length > 1u) {
        return ALICE_STATUS_PAYLOAD_ERROR;
    }

    tile->state.runtime_address = ALICE_TILE_UNASSIGNED_RUNTIME_ADDRESS;
    tile->state.scan_generation = request_scan_generation;
    tile->state.scan_state = ALICE_SCAN_STATE_IDLE;
    alice_tile_clear_scan_cache(tile);

    response->payload.reset_scan_state.reset_flags = alice_u16_to_le(reset_flags);
    alice_tile_finish_response(
        response,
        tile->state.scan_generation,
        ALICE_STATUS_OK,
        ALICE_RESET_SCAN_STATE_RESPONSE_SIZE);

    return ALICE_STATUS_OK;
}

static alice_status_code_t alice_tile_handle_claim_runtime_address(
    alice_tile_t *tile,
    const alice_tile_request_t *request,
    alice_tile_response_t *response,
    uint16_t request_scan_generation)
{
    const alice_claim_runtime_address_request_t *claim_request;
    uint8_t claim_result = ALICE_CLAIM_RESULT_REJECTED;

    if (!alice_tile_payload_size_is(request, ALICE_CLAIM_RUNTIME_ADDRESS_REQUEST_SIZE)) {
        return ALICE_STATUS_PAYLOAD_ERROR;
    }

    if (!alice_tile_scan_generation_matches(tile, request_scan_generation)) {
        return ALICE_STATUS_INVALID_SCAN_GENERATION;
    }

    claim_request = (const alice_claim_runtime_address_request_t *)request->payload;

    if (claim_request->candidate_address == ALICE_TILE_UNASSIGNED_RUNTIME_ADDRESS) {
        claim_result = ALICE_CLAIM_RESULT_REJECTED;
    } else if (tile->state.scan_state == ALICE_SCAN_STATE_PROBING) {
        claim_result = ALICE_CLAIM_RESULT_BUSY;
    } else if (tile->state.runtime_address == ALICE_TILE_UNASSIGNED_RUNTIME_ADDRESS) {
        tile->state.runtime_address = claim_request->candidate_address;
        tile->state.scan_state = ALICE_SCAN_STATE_AWAITING_PROBE;
        claim_result = ALICE_CLAIM_RESULT_ACCEPTED;
    } else if (tile->state.runtime_address == claim_request->candidate_address) {
        claim_result = ALICE_CLAIM_RESULT_ACCEPTED;
    } else {
        claim_result = ALICE_CLAIM_RESULT_REJECTED;
    }

    response->payload.claim_runtime_address.tile_uid = alice_u64_to_le(tile->config.tile_uid);
    response->payload.claim_runtime_address.claim_result = claim_result;
    alice_tile_finish_response(
        response,
        tile->state.scan_generation,
        ALICE_STATUS_OK,
        ALICE_CLAIM_RUNTIME_ADDRESS_RESPONSE_SIZE);

    return ALICE_STATUS_OK;
}

static alice_status_code_t alice_tile_handle_get_tile_identity(
    alice_tile_t *tile,
    const alice_tile_request_t *request,
    alice_tile_response_t *response,
    uint16_t request_scan_generation)
{
    if (!alice_tile_payload_size_is(request, 0u)) {
        return ALICE_STATUS_PAYLOAD_ERROR;
    }

    if (!alice_tile_scan_generation_matches(tile, request_scan_generation)) {
        return ALICE_STATUS_INVALID_SCAN_GENERATION;
    }

    response->payload.get_tile_identity.tile_uid = alice_u64_to_le(tile->config.tile_uid);
    response->payload.get_tile_identity.runtime_address = tile->state.runtime_address;
    response->payload.get_tile_identity.capability_flags = alice_u16_to_le(tile->config.capability_flags);
    alice_tile_finish_response(
        response,
        tile->state.scan_generation,
        ALICE_STATUS_OK,
        ALICE_GET_TILE_IDENTITY_RESPONSE_SIZE);

    return ALICE_STATUS_OK;
}

static alice_status_code_t alice_tile_handle_start_edge_probe(
    alice_tile_t *tile,
    const alice_tile_request_t *request,
    alice_tile_response_t *response,
    uint16_t request_scan_generation)
{
    alice_status_code_t status;
    alice_tile_begin_probe_fn begin_probe;
    alice_tile_probe_result_t probe_result;

    if (!alice_tile_payload_size_is(request, 0u)) {
        return ALICE_STATUS_PAYLOAD_ERROR;
    }

    if (!alice_tile_scan_generation_matches(tile, request_scan_generation)) {
        return ALICE_STATUS_INVALID_SCAN_GENERATION;
    }

    if (tile->state.runtime_address == ALICE_TILE_UNASSIGNED_RUNTIME_ADDRESS) {
        return ALICE_STATUS_INVALID_STATE;
    }

    if (tile->state.scan_state == ALICE_SCAN_STATE_PROBING) {
        return ALICE_STATUS_BUSY;
    }

    if (tile->state.scan_state != ALICE_SCAN_STATE_AWAITING_PROBE &&
        tile->state.scan_state != ALICE_SCAN_STATE_READY &&
        tile->state.scan_state != ALICE_SCAN_STATE_FAULT) {
        return ALICE_STATUS_INVALID_STATE;
    }

    tile->state.scan_state = ALICE_SCAN_STATE_PROBING;
    alice_tile_clear_scan_cache(tile);
    alice_tile_init_probe_result(&probe_result);

    begin_probe = tile->config.begin_probe != NULL
        ? tile->config.begin_probe
        : alice_tile_default_begin_probe;

    status = begin_probe(
        tile->config.probe_context,
        tile->config.tile_uid,
        tile->state.scan_generation,
        &probe_result);

    if (status == ALICE_STATUS_BUSY) {
        alice_tile_finish_response(response, tile->state.scan_generation, ALICE_STATUS_BUSY, 0u);
        return ALICE_STATUS_BUSY;
    }

    status = alice_tile_complete_probe(tile, status, &probe_result);
    alice_tile_finish_response(response, tile->state.scan_generation, status, 0u);
    return status;
}

static alice_status_code_t alice_tile_handle_get_scan_status(
    alice_tile_t *tile,
    const alice_tile_request_t *request,
    alice_tile_response_t *response,
    uint16_t request_scan_generation)
{
    if (!alice_tile_payload_size_is(request, 0u)) {
        return ALICE_STATUS_PAYLOAD_ERROR;
    }

    if (!alice_tile_scan_generation_matches(tile, request_scan_generation)) {
        return ALICE_STATUS_INVALID_SCAN_GENERATION;
    }

    response->payload.get_scan_status.scan_state = tile->state.scan_state;
    response->payload.get_scan_status.progress_flags = tile->state.progress_flags;
    response->payload.get_scan_status.tile_error_flags = alice_u16_to_le(tile->state.tile_error_flags);
    alice_tile_finish_response(
        response,
        tile->state.scan_generation,
        ALICE_STATUS_OK,
        ALICE_GET_SCAN_STATUS_RESPONSE_SIZE);

    return ALICE_STATUS_OK;
}

static alice_status_code_t alice_tile_handle_get_neighbor_report(
    alice_tile_t *tile,
    const alice_tile_request_t *request,
    alice_tile_response_t *response,
    uint16_t request_scan_generation)
{
    if (!alice_tile_payload_size_is(request, 0u)) {
        return ALICE_STATUS_PAYLOAD_ERROR;
    }

    if (!alice_tile_scan_generation_matches(tile, request_scan_generation)) {
        return ALICE_STATUS_INVALID_SCAN_GENERATION;
    }

    if (tile->state.scan_state == ALICE_SCAN_STATE_PROBING) {
        return ALICE_STATUS_BUSY;
    }

    if (tile->state.scan_state != ALICE_SCAN_STATE_READY &&
        tile->state.scan_state != ALICE_SCAN_STATE_FAULT) {
        return ALICE_STATUS_INVALID_STATE;
    }

    response->payload.get_neighbor_report.tile_uid = alice_u64_to_le(tile->config.tile_uid);
    response->payload.get_neighbor_report.runtime_address = tile->state.runtime_address;
    response->payload.get_neighbor_report.report_flags = alice_u16_to_le(tile->state.report_flags);
    alice_tile_copy_neighbors(
        response->payload.get_neighbor_report.neighbors,
        tile->state.neighbors);
    alice_tile_finish_response(
        response,
        tile->state.scan_generation,
        ALICE_STATUS_OK,
        ALICE_GET_NEIGHBOR_REPORT_RESPONSE_SIZE);

    return ALICE_STATUS_OK;
}

static alice_status_code_t alice_tile_handle_ping(
    alice_tile_t *tile,
    const alice_tile_request_t *request,
    alice_tile_response_t *response,
    uint16_t request_scan_generation)
{
    uint8_t health_flags = ALICE_HEALTH_FLAG_RESPONSIVE;

    if (!alice_tile_payload_size_is(request, 0u)) {
        return ALICE_STATUS_PAYLOAD_ERROR;
    }

    if (!alice_tile_scan_generation_matches(tile, request_scan_generation)) {
        return ALICE_STATUS_INVALID_SCAN_GENERATION;
    }

    if (tile->state.scan_state == ALICE_SCAN_STATE_FAULT || tile->state.tile_error_flags != ALICE_TILE_ERROR_NONE) {
        health_flags |= ALICE_HEALTH_FLAG_FAULT;
    }

    response->payload.ping.health_flags = health_flags;
    alice_tile_finish_response(
        response,
        tile->state.scan_generation,
        ALICE_STATUS_OK,
        ALICE_PING_RESPONSE_SIZE);

    return ALICE_STATUS_OK;
}

void alice_tile_init_probe_result(alice_tile_probe_result_t *result)
{
    if (result == NULL) {
        return;
    }

    result->progress_flags = ALICE_PROGRESS_FLAG_NONE;
    result->tile_error_flags = ALICE_TILE_ERROR_NONE;
    alice_tile_clear_neighbors(result->neighbors);
}

void alice_tile_init(alice_tile_t *tile, const alice_tile_config_t *config)
{
    if (tile == NULL) {
        return;
    }

    memset(tile, 0, sizeof(*tile));

    if (config != NULL) {
        tile->config = *config;
    }

    tile->state.runtime_address = ALICE_TILE_UNASSIGNED_RUNTIME_ADDRESS;
    tile->state.scan_state = ALICE_SCAN_STATE_IDLE;
    alice_tile_clear_scan_cache(tile);
}

const alice_tile_state_t *alice_tile_get_state(const alice_tile_t *tile)
{
    return tile != NULL ? &tile->state : NULL;
}

bool alice_tile_scan_generation_matches(const alice_tile_t *tile, uint16_t scan_generation)
{
    return tile != NULL && tile->state.scan_generation == scan_generation;
}

alice_status_code_t alice_tile_complete_probe(
    alice_tile_t *tile,
    alice_status_code_t probe_status,
    const alice_tile_probe_result_t *result)
{
    if (tile == NULL) {
        return ALICE_STATUS_INTERNAL_FAULT;
    }

    if (tile->state.scan_state != ALICE_SCAN_STATE_PROBING) {
        return ALICE_STATUS_INVALID_STATE;
    }

    tile->state.progress_flags = result != NULL ? result->progress_flags : ALICE_PROGRESS_FLAG_NONE;
    alice_tile_copy_neighbors(tile->state.neighbors, result != NULL ? result->neighbors : NULL);

    if (probe_status == ALICE_STATUS_OK) {
        tile->state.tile_error_flags = result != NULL ? result->tile_error_flags : ALICE_TILE_ERROR_NONE;
        tile->state.report_flags = ALICE_REPORT_FLAG_COMPLETE | ALICE_REPORT_FLAG_SCAN_GENERATION_MATCH;

        if (tile->state.progress_flags != ALICE_TILE_PROGRESS_COMPLETE) {
            tile->state.tile_error_flags |= ALICE_TILE_ERROR_PROBE_INCOMPLETE;
        }

        if (tile->state.tile_error_flags != ALICE_TILE_ERROR_NONE) {
            tile->state.report_flags |= ALICE_REPORT_FLAG_HAS_FAULT;
            tile->state.scan_state = ALICE_SCAN_STATE_FAULT;
        } else {
            tile->state.scan_state = ALICE_SCAN_STATE_READY;
        }

        return ALICE_STATUS_OK;
    }

    tile->state.tile_error_flags = result != NULL ? result->tile_error_flags : ALICE_TILE_ERROR_NONE;
    tile->state.tile_error_flags |= alice_tile_status_to_error_flags(probe_status);
    tile->state.report_flags = ALICE_REPORT_FLAG_HAS_FAULT | ALICE_REPORT_FLAG_SCAN_GENERATION_MATCH;
    tile->state.scan_state = ALICE_SCAN_STATE_FAULT;

    return probe_status;
}

alice_status_code_t alice_tile_handle_request(
    alice_tile_t *tile,
    const alice_tile_request_t *request,
    alice_tile_response_t *response)
{
    alice_status_code_t status;
    uint16_t request_scan_generation;

    if (tile == NULL || request == NULL || response == NULL) {
        return ALICE_STATUS_INTERNAL_FAULT;
    }

    request_scan_generation = alice_u16_from_le(request->header.scan_generation);
    alice_tile_prepare_response(response, request->header.command, tile->state.scan_generation);

    switch (request->header.command) {
    case ALICE_COMMAND_RESET_SCAN_STATE:
        status = alice_tile_handle_reset_scan_state(tile, request, response, request_scan_generation);
        break;
    case ALICE_COMMAND_CLAIM_RUNTIME_ADDRESS:
        status = alice_tile_handle_claim_runtime_address(tile, request, response, request_scan_generation);
        break;
    case ALICE_COMMAND_GET_TILE_IDENTITY:
        status = alice_tile_handle_get_tile_identity(tile, request, response, request_scan_generation);
        break;
    case ALICE_COMMAND_START_EDGE_PROBE:
        status = alice_tile_handle_start_edge_probe(tile, request, response, request_scan_generation);
        break;
    case ALICE_COMMAND_GET_SCAN_STATUS:
        status = alice_tile_handle_get_scan_status(tile, request, response, request_scan_generation);
        break;
    case ALICE_COMMAND_GET_NEIGHBOR_REPORT:
        status = alice_tile_handle_get_neighbor_report(tile, request, response, request_scan_generation);
        break;
    case ALICE_COMMAND_PING:
        status = alice_tile_handle_ping(tile, request, response, request_scan_generation);
        break;
    default:
        status = ALICE_STATUS_INVALID_COMMAND;
        break;
    }

    if (response->header.payload_length == 0u) {
        alice_tile_finish_response(response, tile->state.scan_generation, status, 0u);
    }

    return status;
}
