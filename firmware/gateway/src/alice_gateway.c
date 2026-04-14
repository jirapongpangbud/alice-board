#include "firmware/gateway/include/alice_gateway.h"

#include <string.h>

static void alice_gateway_clear_tile_record(alice_gateway_tile_record_t *record)
{
    uint8_t direction;

    memset(record, 0, sizeof(*record));

    for (direction = 0u; direction < ALICE_PROTOCOL_NEIGHBOR_REPORT_COUNT; ++direction) {
        record->neighbors[direction].direction = direction;
        record->neighbors[direction].edge_state = ALICE_EDGE_STATE_UNKNOWN;
        record->neighbors[direction].neighbor_uid = alice_u64_to_le(0u);
    }
}

static void alice_gateway_clear_snapshot(alice_gateway_snapshot_t *snapshot)
{
    uint16_t index;
    const uint32_t snapshot_id = snapshot->snapshot_id;
    const uint16_t scan_generation = snapshot->scan_generation;

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->snapshot_id = snapshot_id;
    snapshot->scan_generation = scan_generation;

    for (index = 0u; index < ALICE_PROTOCOL_MAX_TILES; ++index) {
        alice_gateway_clear_tile_record(&snapshot->tiles[index]);
    }
}

static uint16_t alice_gateway_next_scan_generation(uint16_t current)
{
    current = (uint16_t)(current + 1u);
    return current == 0u ? 1u : current;
}

static uint16_t alice_gateway_make_claim_nonce(uint16_t scan_generation, uint8_t runtime_address)
{
    return (uint16_t)(((uint16_t)runtime_address << 8) ^ scan_generation ^ 0x55aau);
}

static alice_gateway_tile_record_t *alice_gateway_find_tile_by_uid(
    alice_gateway_t *gateway,
    alice_tile_uid_t tile_uid)
{
    uint16_t index;

    for (index = 0u; index < gateway->snapshot.tile_count; ++index) {
        if (gateway->snapshot.tiles[index].tile_uid == tile_uid) {
            return &gateway->snapshot.tiles[index];
        }
    }

    return NULL;
}

static void alice_gateway_note_fault(
    alice_gateway_t *gateway,
    alice_gateway_tile_record_t *record,
    uint16_t tile_flag)
{
    gateway->snapshot.fault_count = (uint16_t)(gateway->snapshot.fault_count + 1u);

    if (record != NULL) {
        record->tile_flags |= (uint16_t)(tile_flag | ALICE_GATEWAY_TILE_FLAG_HAS_FAULT);
    }
}

static void alice_gateway_prepare_request(
    alice_gateway_request_t *request,
    uint8_t command,
    uint16_t scan_generation,
    uint8_t payload_length)
{
    memset(request, 0, sizeof(*request));
    request->header.command = command;
    request->header.scan_generation = alice_u16_to_le(scan_generation);
    request->header.payload_length = payload_length;
}

static alice_status_code_t alice_gateway_validate_response(
    alice_gateway_t *gateway,
    uint8_t command,
    uint16_t scan_generation,
    const alice_gateway_response_t *response,
    uint8_t expected_payload_length,
    alice_gateway_tile_record_t *record)
{
    if (response->header.protocol_version != ALICE_PROTOCOL_VERSION) {
        gateway->last_transport_result = ALICE_GATEWAY_TRANSPORT_MALFORMED_RESPONSE;
        if (record != NULL) {
            record->protocol_version = response->header.protocol_version;
            record->tile_flags |= (uint16_t)(ALICE_GATEWAY_TILE_FLAG_PROTOCOL_MISMATCH | ALICE_GATEWAY_TILE_FLAG_HAS_FAULT);
        }
        return ALICE_STATUS_INTERNAL_FAULT;
    }

    if (response->header.message_type != command ||
        alice_u16_from_le(response->header.scan_generation) != scan_generation ||
        response->header.payload_length != expected_payload_length) {
        gateway->last_transport_result = ALICE_GATEWAY_TRANSPORT_MALFORMED_RESPONSE;
        if (record != NULL) {
            record->tile_flags |= (uint16_t)(ALICE_GATEWAY_TILE_FLAG_TRANSPORT_ERROR | ALICE_GATEWAY_TILE_FLAG_HAS_FAULT);
        }
        return ALICE_STATUS_PAYLOAD_ERROR;
    }

    return (alice_status_code_t)response->header.status_code;
}

static alice_gateway_transport_result_t alice_gateway_claim_address(
    alice_gateway_t *gateway,
    alice_runtime_address_t runtime_address,
    alice_gateway_response_t *response)
{
    alice_gateway_request_t request;

    alice_gateway_prepare_request(
        &request,
        ALICE_COMMAND_CLAIM_RUNTIME_ADDRESS,
        gateway->snapshot.scan_generation,
        ALICE_CLAIM_RUNTIME_ADDRESS_REQUEST_SIZE);
    request.payload[0] = runtime_address;
    memcpy(&request.payload[1], alice_u16_to_le(
        alice_gateway_make_claim_nonce(gateway->snapshot.scan_generation, runtime_address)).bytes, 2u);

    if (gateway->config.transport.claim_address == NULL) {
        return ALICE_GATEWAY_TRANSPORT_BUS_ERROR;
    }

    return gateway->config.transport.claim_address(
        gateway->config.transport_context,
        gateway->snapshot.scan_generation,
        &request,
        response);
}

static alice_gateway_transport_result_t alice_gateway_transact(
    alice_gateway_t *gateway,
    alice_runtime_address_t runtime_address,
    uint8_t command,
    const void *payload,
    uint8_t payload_length,
    alice_gateway_response_t *response)
{
    alice_gateway_request_t request;

    alice_gateway_prepare_request(
        &request,
        command,
        gateway->snapshot.scan_generation,
        payload_length);

    if (payload_length > 0u && payload != NULL) {
        memcpy(request.payload, payload, payload_length);
    }

    if (gateway->config.transport.transact == NULL) {
        return ALICE_GATEWAY_TRANSPORT_BUS_ERROR;
    }

    return gateway->config.transport.transact(
        gateway->config.transport_context,
        runtime_address,
        &request,
        response);
}

static alice_status_code_t alice_gateway_collect_identity(
    alice_gateway_t *gateway,
    alice_gateway_tile_record_t *record)
{
    alice_gateway_transport_result_t transport_result;
    alice_gateway_response_t response;
    alice_status_code_t status;
    alice_gateway_tile_record_t *duplicate_record;

    transport_result = alice_gateway_transact(
        gateway,
        record->runtime_address,
        ALICE_COMMAND_GET_TILE_IDENTITY,
        NULL,
        0u,
        &response);
    gateway->last_transport_result = (uint8_t)transport_result;

    if (transport_result != ALICE_GATEWAY_TRANSPORT_OK) {
        alice_gateway_note_fault(gateway, record, ALICE_GATEWAY_TILE_FLAG_TRANSPORT_ERROR);
        return ALICE_STATUS_INTERNAL_FAULT;
    }

    status = alice_gateway_validate_response(
        gateway,
        ALICE_COMMAND_GET_TILE_IDENTITY,
        gateway->snapshot.scan_generation,
        &response,
        ALICE_GET_TILE_IDENTITY_RESPONSE_SIZE,
        record);
    gateway->last_status_code = (uint8_t)status;

    if (status != ALICE_STATUS_OK) {
        alice_gateway_note_fault(gateway, record, ALICE_GATEWAY_TILE_FLAG_TRANSPORT_ERROR);
        return status;
    }

    record->tile_uid = alice_u64_from_le(response.payload.get_tile_identity.tile_uid);
    record->runtime_address = response.payload.get_tile_identity.runtime_address;
    record->protocol_version = response.header.protocol_version;
    record->scan_generation = gateway->snapshot.scan_generation;
    record->tile_flags |= (uint16_t)(ALICE_GATEWAY_TILE_FLAG_CLAIMED | ALICE_GATEWAY_TILE_FLAG_IDENTITY_VALID);

    duplicate_record = alice_gateway_find_tile_by_uid(gateway, record->tile_uid);
    if (duplicate_record != NULL && duplicate_record != record) {
        duplicate_record->tile_flags |= (uint16_t)(ALICE_GATEWAY_TILE_FLAG_DUPLICATE_UID | ALICE_GATEWAY_TILE_FLAG_HAS_FAULT);
        record->tile_flags |= (uint16_t)(ALICE_GATEWAY_TILE_FLAG_DUPLICATE_UID | ALICE_GATEWAY_TILE_FLAG_HAS_FAULT);
        gateway->snapshot.fault_count = (uint16_t)(gateway->snapshot.fault_count + 1u);
    }

    return ALICE_STATUS_OK;
}

static void alice_gateway_reset_session(alice_gateway_t *gateway)
{
    gateway->snapshot.snapshot_id += 1u;
    gateway->snapshot.scan_generation = alice_gateway_next_scan_generation(gateway->snapshot.scan_generation);
    alice_gateway_clear_snapshot(&gateway->snapshot);
    gateway->last_transport_result = ALICE_GATEWAY_TRANSPORT_OK;
    gateway->last_status_code = ALICE_STATUS_OK;
}

static alice_status_code_t alice_gateway_phase_reset(alice_gateway_t *gateway)
{
    alice_gateway_transport_result_t result;

    gateway->phase = ALICE_GATEWAY_PHASE_RESETTING;

    if (gateway->config.transport.reset_all == NULL) {
        gateway->phase = ALICE_GATEWAY_PHASE_FAULT;
        gateway->last_transport_result = ALICE_GATEWAY_TRANSPORT_BUS_ERROR;
        return ALICE_STATUS_INTERNAL_FAULT;
    }

    result = gateway->config.transport.reset_all(
        gateway->config.transport_context,
        gateway->snapshot.scan_generation);
    gateway->last_transport_result = (uint8_t)result;

    if (result != ALICE_GATEWAY_TRANSPORT_OK) {
        gateway->phase = ALICE_GATEWAY_PHASE_FAULT;
        gateway->snapshot.fault_count = (uint16_t)(gateway->snapshot.fault_count + 1u);
        return ALICE_STATUS_INTERNAL_FAULT;
    }

    return ALICE_STATUS_OK;
}

static alice_status_code_t alice_gateway_phase_claim(alice_gateway_t *gateway)
{
    alice_runtime_address_t runtime_address;

    gateway->phase = ALICE_GATEWAY_PHASE_CLAIMING;

    for (runtime_address = gateway->config.first_runtime_address;
         runtime_address <= gateway->config.last_runtime_address &&
         gateway->snapshot.tile_count < ALICE_PROTOCOL_MAX_TILES;
         ++runtime_address) {
        alice_gateway_transport_result_t transport_result;
        alice_gateway_response_t response;
        alice_status_code_t status;
        alice_gateway_tile_record_t *record;

        transport_result = alice_gateway_claim_address(gateway, runtime_address, &response);
        gateway->last_transport_result = (uint8_t)transport_result;

        if (transport_result == ALICE_GATEWAY_TRANSPORT_NO_DEVICE) {
            continue;
        }

        if (transport_result != ALICE_GATEWAY_TRANSPORT_OK) {
            gateway->snapshot.fault_count = (uint16_t)(gateway->snapshot.fault_count + 1u);
            continue;
        }

        status = alice_gateway_validate_response(
            gateway,
            ALICE_COMMAND_CLAIM_RUNTIME_ADDRESS,
            gateway->snapshot.scan_generation,
            &response,
            ALICE_CLAIM_RUNTIME_ADDRESS_RESPONSE_SIZE,
            NULL);
        gateway->last_status_code = (uint8_t)status;

        if (status != ALICE_STATUS_OK) {
            gateway->snapshot.fault_count = (uint16_t)(gateway->snapshot.fault_count + 1u);
            continue;
        }

        if (response.payload.claim_runtime_address.claim_result != ALICE_CLAIM_RESULT_ACCEPTED) {
            if (response.payload.claim_runtime_address.claim_result == ALICE_CLAIM_RESULT_COLLISION) {
                gateway->snapshot.fault_count = (uint16_t)(gateway->snapshot.fault_count + 1u);
            }
            continue;
        }

        record = &gateway->snapshot.tiles[gateway->snapshot.tile_count];
        alice_gateway_clear_tile_record(record);
        record->runtime_address = runtime_address;
        record->scan_generation = gateway->snapshot.scan_generation;

        status = alice_gateway_collect_identity(gateway, record);
        if (status == ALICE_STATUS_OK) {
            gateway->snapshot.tile_count = (uint16_t)(gateway->snapshot.tile_count + 1u);
        }
    }

    return ALICE_STATUS_OK;
}

static alice_status_code_t alice_gateway_phase_probe(alice_gateway_t *gateway)
{
    uint16_t index;

    gateway->phase = ALICE_GATEWAY_PHASE_PROBING;

    for (index = 0u; index < gateway->snapshot.tile_count; ++index) {
        alice_gateway_tile_record_t *record = &gateway->snapshot.tiles[index];
        alice_gateway_transport_result_t transport_result;
        alice_gateway_response_t response;
        alice_status_code_t status;

        transport_result = alice_gateway_transact(
            gateway,
            record->runtime_address,
            ALICE_COMMAND_START_EDGE_PROBE,
            NULL,
            0u,
            &response);
        gateway->last_transport_result = (uint8_t)transport_result;

        if (transport_result != ALICE_GATEWAY_TRANSPORT_OK) {
            alice_gateway_note_fault(gateway, record, ALICE_GATEWAY_TILE_FLAG_TRANSPORT_ERROR);
            continue;
        }

        status = alice_gateway_validate_response(
            gateway,
            ALICE_COMMAND_START_EDGE_PROBE,
            gateway->snapshot.scan_generation,
            &response,
            0u,
            record);
        gateway->last_status_code = (uint8_t)status;

        if (status != ALICE_STATUS_OK && status != ALICE_STATUS_BUSY) {
            alice_gateway_note_fault(gateway, record, ALICE_GATEWAY_TILE_FLAG_TRANSPORT_ERROR);
        }
    }

    return ALICE_STATUS_OK;
}

static alice_status_code_t alice_gateway_phase_poll(alice_gateway_t *gateway)
{
    uint16_t index;

    gateway->phase = ALICE_GATEWAY_PHASE_POLLING;

    for (index = 0u; index < gateway->snapshot.tile_count; ++index) {
        alice_gateway_tile_record_t *record = &gateway->snapshot.tiles[index];
        uint8_t attempt;
        uint8_t ready = 0u;

        for (attempt = 0u; attempt < gateway->config.status_poll_attempts; ++attempt) {
            alice_gateway_transport_result_t transport_result;
            alice_gateway_response_t response;
            alice_status_code_t status;

            transport_result = alice_gateway_transact(
                gateway,
                record->runtime_address,
                ALICE_COMMAND_GET_SCAN_STATUS,
                NULL,
                0u,
                &response);
            gateway->last_transport_result = (uint8_t)transport_result;

            if (transport_result != ALICE_GATEWAY_TRANSPORT_OK) {
                alice_gateway_note_fault(gateway, record, ALICE_GATEWAY_TILE_FLAG_TRANSPORT_ERROR);
                break;
            }

            status = alice_gateway_validate_response(
                gateway,
                ALICE_COMMAND_GET_SCAN_STATUS,
                gateway->snapshot.scan_generation,
                &response,
                ALICE_GET_SCAN_STATUS_RESPONSE_SIZE,
                record);
            gateway->last_status_code = (uint8_t)status;

            if (status == ALICE_STATUS_BUSY) {
                continue;
            }

            if (status != ALICE_STATUS_OK) {
                alice_gateway_note_fault(gateway, record, ALICE_GATEWAY_TILE_FLAG_TRANSPORT_ERROR);
                break;
            }

            record->scan_state = response.payload.get_scan_status.scan_state;
            record->tile_error_flags = alice_u16_from_le(response.payload.get_scan_status.tile_error_flags);
            record->tile_flags |= ALICE_GATEWAY_TILE_FLAG_STATUS_VALID;

            if (record->tile_error_flags != ALICE_TILE_ERROR_NONE) {
                record->tile_flags |= (uint16_t)(ALICE_GATEWAY_TILE_FLAG_HAS_FAULT);
            }

            if (record->scan_state == ALICE_SCAN_STATE_READY ||
                record->scan_state == ALICE_SCAN_STATE_FAULT) {
                ready = 1u;
                break;
            }
        }

        if (ready == 0u) {
            alice_gateway_note_fault(gateway, record, ALICE_GATEWAY_TILE_FLAG_STATUS_TIMEOUT);
        }
    }

    return ALICE_STATUS_OK;
}

static alice_status_code_t alice_gateway_phase_collect(alice_gateway_t *gateway)
{
    uint16_t index;

    gateway->phase = ALICE_GATEWAY_PHASE_COLLECTING;

    for (index = 0u; index < gateway->snapshot.tile_count; ++index) {
        alice_gateway_tile_record_t *record = &gateway->snapshot.tiles[index];
        alice_gateway_transport_result_t transport_result;
        alice_gateway_response_t response;
        alice_status_code_t status;

        if (record->scan_state != ALICE_SCAN_STATE_READY &&
            record->scan_state != ALICE_SCAN_STATE_FAULT) {
            continue;
        }

        transport_result = alice_gateway_transact(
            gateway,
            record->runtime_address,
            ALICE_COMMAND_GET_NEIGHBOR_REPORT,
            NULL,
            0u,
            &response);
        gateway->last_transport_result = (uint8_t)transport_result;

        if (transport_result != ALICE_GATEWAY_TRANSPORT_OK) {
            alice_gateway_note_fault(gateway, record, ALICE_GATEWAY_TILE_FLAG_TRANSPORT_ERROR);
            continue;
        }

        status = alice_gateway_validate_response(
            gateway,
            ALICE_COMMAND_GET_NEIGHBOR_REPORT,
            gateway->snapshot.scan_generation,
            &response,
            ALICE_GET_NEIGHBOR_REPORT_RESPONSE_SIZE,
            record);
        gateway->last_status_code = (uint8_t)status;

        if (status != ALICE_STATUS_OK) {
            alice_gateway_note_fault(gateway, record, ALICE_GATEWAY_TILE_FLAG_TRANSPORT_ERROR);
            continue;
        }

        record->report_flags = alice_u16_from_le(response.payload.get_neighbor_report.report_flags);
        memcpy(
            record->neighbors,
            response.payload.get_neighbor_report.neighbors,
            sizeof(record->neighbors));
        record->tile_flags |= ALICE_GATEWAY_TILE_FLAG_REPORT_VALID;

        if ((record->report_flags & ALICE_REPORT_FLAG_HAS_FAULT) != 0u) {
            record->tile_flags |= ALICE_GATEWAY_TILE_FLAG_HAS_FAULT;
        }
    }

    return ALICE_STATUS_OK;
}

void alice_gateway_init(alice_gateway_t *gateway, const alice_gateway_config_t *config)
{
    if (gateway == NULL) {
        return;
    }

    memset(gateway, 0, sizeof(*gateway));

    gateway->config.first_runtime_address = ALICE_GATEWAY_DEFAULT_FIRST_RUNTIME_ADDRESS;
    gateway->config.last_runtime_address = ALICE_GATEWAY_DEFAULT_LAST_RUNTIME_ADDRESS;
    gateway->config.status_poll_attempts = ALICE_GATEWAY_DEFAULT_STATUS_POLL_ATTEMPTS;

    if (config != NULL) {
        gateway->config = *config;
    }

    if (gateway->config.first_runtime_address == 0u) {
        gateway->config.first_runtime_address = ALICE_GATEWAY_DEFAULT_FIRST_RUNTIME_ADDRESS;
    }

    if (gateway->config.last_runtime_address < gateway->config.first_runtime_address) {
        gateway->config.last_runtime_address = gateway->config.first_runtime_address;
    }

    if (gateway->config.status_poll_attempts == 0u) {
        gateway->config.status_poll_attempts = ALICE_GATEWAY_DEFAULT_STATUS_POLL_ATTEMPTS;
    }

    alice_gateway_clear_snapshot(&gateway->snapshot);
    gateway->phase = ALICE_GATEWAY_PHASE_IDLE;
    gateway->last_transport_result = ALICE_GATEWAY_TRANSPORT_OK;
    gateway->last_status_code = ALICE_STATUS_OK;
}

const alice_gateway_snapshot_t *alice_gateway_get_snapshot(const alice_gateway_t *gateway)
{
    return gateway != NULL ? &gateway->snapshot : NULL;
}

uint8_t alice_gateway_get_phase(const alice_gateway_t *gateway)
{
    return gateway != NULL ? gateway->phase : ALICE_GATEWAY_PHASE_FAULT;
}

alice_status_code_t alice_gateway_run_discovery(alice_gateway_t *gateway)
{
    alice_status_code_t status;

    if (gateway == NULL) {
        return ALICE_STATUS_INTERNAL_FAULT;
    }

    alice_gateway_reset_session(gateway);

    status = alice_gateway_phase_reset(gateway);
    if (status != ALICE_STATUS_OK) {
        return status;
    }

    status = alice_gateway_phase_claim(gateway);
    if (status != ALICE_STATUS_OK) {
        gateway->phase = ALICE_GATEWAY_PHASE_FAULT;
        return status;
    }

    status = alice_gateway_phase_probe(gateway);
    if (status != ALICE_STATUS_OK) {
        gateway->phase = ALICE_GATEWAY_PHASE_FAULT;
        return status;
    }

    status = alice_gateway_phase_poll(gateway);
    if (status != ALICE_STATUS_OK) {
        gateway->phase = ALICE_GATEWAY_PHASE_FAULT;
        return status;
    }

    status = alice_gateway_phase_collect(gateway);
    if (status != ALICE_STATUS_OK) {
        gateway->phase = ALICE_GATEWAY_PHASE_FAULT;
        return status;
    }

    gateway->phase = ALICE_GATEWAY_PHASE_COMPLETE;
    return ALICE_STATUS_OK;
}
