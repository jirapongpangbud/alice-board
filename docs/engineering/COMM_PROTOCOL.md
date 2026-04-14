# Communication Protocol

This document defines the Phase 1 discovery protocol between the gateway and tiles over `I2C`, plus the tile-local reporting contract for directional neighbor links.

The protocol is deliberately small, versioned, and bounded. It is designed for discovery only. It is not a generic transport for future gameplay.

## Protocol Goals

- identify tiles reliably
- support runtime `I2C` address claim
- return fixed-size discovery reports
- bound all request and response sizes
- provide explicit status and fault codes
- keep tiles simple and gateway-controlled

## Link Roles

### I2C: Gateway to Tile

`I2C` is used for command and response traffic between the gateway and individually addressable tiles.

Responsibilities:

- scan control
- address claim
- identity query
- explicit edge-scan trigger
- discovery report collection
- health and error reporting

### Directional Edge Link: Tile to Tile

Each tile edge implements a custom single-wire half-duplex local link.

Responsibilities:

- local presence detection
- minimal neighbor identity exchange
- edge-level handshake status for the current scan

The directional link never carries board-wide state.

Tiles must not probe neighbors autonomously as a normal operating mode. Edge probing is started by a gateway command for the active scan generation.

## Versioning

Every tile response must include:

- `protocol_version`
- `message_type`
- `status_code`

Rules:

- incompatible protocol versions must be surfaced as gateway-visible faults
- Phase 1 should begin with protocol version `1`
- version is checked before the gateway trusts detailed payload fields
- `message_type` should mirror the originating command ID in Phase 1 responses

## I2C Framing Model

Use small fixed request and response frames.

Suggested request header:

| Field | Size | Notes |
| --- | --- | --- |
| `command` | 1 byte | Command ID |
| `scan_generation` | 2 bytes | Active discovery session |
| `payload_length` | 1 byte | Fixed or bounded length |
| `payload` | N bytes | Command-specific data |

Suggested response header:

| Field | Size | Notes |
| --- | --- | --- |
| `protocol_version` | 1 byte | Starts at `1` |
| `message_type` | 1 byte | Mirrors response kind |
| `status_code` | 1 byte | Success or specific failure |
| `scan_generation` | 2 bytes | Echo or confirm active session |
| `payload_length` | 1 byte | Fixed or bounded length |
| `payload` | N bytes | Response-specific data |

Guidelines:

- avoid variable-length lists unless the upper bound is part of the command definition
- encode all multi-byte integer fields as little-endian on the wire
- reject payloads that do not match the expected fixed size for a command

## I2C Command Set

### `0x01` RESET_SCAN_STATE

Purpose:

- clear scan-local edge cache
- invalidate any previous neighbor observations
- prepare the tile for a new discovery session

Request payload:

- none or fixed control flags byte

Response payload:

| Field | Size | Meaning |
| --- | --- | --- |
| `reset_flags` | 2 bytes | Cache cleared and address-release result bits |

Expected result:

- tile enters idle pre-scan state for the specified `scan_generation`

### `0x02` CLAIM_RUNTIME_ADDRESS

Purpose:

- move a tile from unassigned to assigned state on the shared `I2C` bus

Request payload:

| Field | Size | Meaning |
| --- | --- | --- |
| `candidate_address` | 1 byte | Proposed runtime address |
| `claim_nonce` | 2 bytes | Gateway-generated claim token |

Response payload:

| Field | Size | Meaning |
| --- | --- | --- |
| `tile_uid` | 8 bytes | Permanent identity |
| `claim_result` | 1 byte | Accepted, rejected, collision, busy |

Notes:

- exact electrical collision mitigation is implementation-dependent
- the protocol contract requires explicit success or failure reporting
- gateway must validate that each accepted runtime address maps to one unique `TileUID`

### `0x03` GET_TILE_IDENTITY

Purpose:

- retrieve canonical tile identity and basic metadata

Response payload:

| Field | Size | Meaning |
| --- | --- | --- |
| `tile_uid` | 8 bytes | Permanent identity |
| `runtime_address` | 1 byte | Current I2C address |
| `capability_flags` | 2 bytes | Reserved for discovery capabilities |

### `0x04` START_EDGE_PROBE

Purpose:

- instruct a tile to probe all four directional links for the current scan
- act as the normal trigger for tile-side neighbor scanning

Response payload:

- immediate acknowledgement only

Notes:

- tile may complete probing before the response or mark itself busy and complete shortly after
- Phase 1 should prefer bounded local probing time instead of indefinite retry loops
- without this command, a tile should remain idle and should not start a fresh neighbor scan on its own
- the normal acknowledgement payload length is `0`

### `0x05` GET_SCAN_STATUS

Purpose:

- read current tile scan state

Response payload:

| Field | Size | Meaning |
| --- | --- | --- |
| `scan_state` | 1 byte | Idle, awaiting_probe, probing, ready, fault |
| `progress_flags` | 1 byte | Optional bounded state bits |
| `tile_error_flags` | 2 bytes | Tile-local scan faults |

### `0x06` GET_NEIGHBOR_REPORT

Purpose:

- read the tile's fixed-size directional report for all four edges

Response payload:

| Field | Size | Meaning |
| --- | --- | --- |
| `tile_uid` | 8 bytes | Permanent identity |
| `runtime_address` | 1 byte | Current address |
| `report_flags` | 2 bytes | Report validity flags |
| `neighbors[4]` | fixed | Four `NeighborReport` entries |

This is the primary Phase 1 data collection command.

### `0x07` PING

Purpose:

- verify the tile is responsive at its claimed address

Response payload:

- none or single health byte

## Status Codes

Suggested common `status_code` values:

```text
0x00 = OK
0x01 = BUSY
0x02 = INVALID_COMMAND
0x03 = INVALID_SCAN_GENERATION
0x04 = INVALID_STATE
0x05 = PAYLOAD_ERROR
0x06 = EDGE_TIMEOUT
0x07 = EDGE_HANDSHAKE_FAILED
0x08 = ADDRESS_CONFLICT
0x09 = INTERNAL_FAULT
```

Rules:

- a non-OK status must still use a bounded response
- status codes should be stable and globally documented
- status codes must be precise enough for the gateway to classify recovery actions

## Neighbor Reporting Format

Each tile returns four directional neighbor records in a fixed order:

```text
Index 0 = North
Index 1 = East
Index 2 = South
Index 3 = West
```

Suggested per-edge payload:

| Field | Size | Meaning |
| --- | --- | --- |
| `direction` | 1 byte | `N/E/S/W` |
| `edge_state` | 1 byte | Open, confirmed, timeout, etc. |
| `neighbor_uid` | 8 bytes | Valid only when confirmed |
| `signal_quality` | 1 byte | Optional simple confidence or link-quality byte |
| `edge_error_flags` | 1 byte | Edge-local anomalies |

Guidelines:

- keep it fixed-size even when no neighbor exists
- use `neighbor_uid = 0` when the edge is not confirmed
- do not include variable-length diagnostics in Phase 1
- the fixed per-edge payload size is `12` bytes

## Directional Edge Handshake Contract

The custom single-wire half-duplex link is intentionally minimal.

Expected tile-local edge handshake outcomes:

- `Open`: no valid neighbor detected
- `NeighborDetected`: physical presence or line activity detected, identity not yet trusted
- `NeighborConfirmed`: adjacent tile returned a valid `TileUID`
- `HandshakeFailed`: a neighbor appears present but the exchange was malformed
- `Timeout`: handshake started but did not complete within the bounded time

Recommended local exchange content:

- simple sync marker
- sender direction context if needed by implementation
- sender `TileUID`
- response `TileUID`
- checksum or compact integrity byte

This remains a local physical-link contract. The global system never forwards this traffic beyond the gateway summary.

## Runtime Address-Claim Flow

The address-claim flow is one of the main Phase 1 risks and should stay explicit.

Recommended sequence:

1. Gateway starts a new `scan_generation`.
2. Tiles are reset into unassigned scan state.
3. Gateway performs controlled claim steps for candidate addresses.
4. A tile accepts one temporary runtime address.
5. Gateway immediately verifies identity with `GET_TILE_IDENTITY`.
6. Gateway records the verified mapping `(runtime_address -> TileUID)`.
7. Gateway issues `START_EDGE_PROBE` when it wants the tile to scan its surrounding links.
8. If duplicates, collisions, or ambiguity appear, the gateway marks the issue and may restart the scan.

Implementation notes:

- the protocol must expose claim success or failure, not just assume bus behavior
- address claim must be finite and restartable
- the gateway must not trust address stability across scans

## Retry and Timeout Rules

Phase 1 should use bounded retries only.

Recommended rules:

- retry `PING` or identity reads a small finite number of times
- treat repeated `BUSY` or timeout responses as tile faults
- do not allow a single tile to stall the full scan indefinitely
- preserve fault records in the snapshot so bench testing can inspect the failure

The gateway should prefer "scan completes with explicit faults" over "scan hangs waiting for perfection."

## Validation Rules

The gateway should validate:

- protocol version compatibility
- runtime address uniqueness
- `TileUID` uniqueness
- response size and command/response matching
- scan generation freshness
- reciprocal edge consistency

The tile should validate:

- command validity
- scan generation alignment
- local state transitions
- edge handshake integrity
- that edge probing only occurs after an explicit gateway trigger for the active scan

## Out of Scope

The following are not part of this protocol:

- game state transport
- configuration management
- firmware update delivery
- external app communication
- rich diagnostics streaming

If future phases need those capabilities, they should extend the protocol intentionally rather than overloading discovery messages.
