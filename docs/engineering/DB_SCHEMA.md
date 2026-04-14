# Discovery Data Model

This document defines the Phase 1 logical schema for discovery data. The term "schema" is used in the embedded sense: fixed-size records, field definitions, and canonical relationships. There is no database engine implied.

The model is designed for:

- bounded memory use
- predictable serialization
- no dynamic allocation on tiles
- straightforward validation on the gateway

## Design Rules

- `TileUID` is the permanent identifier.
- Runtime `I2C` address is temporary and scan-scoped.
- The canonical map is an adjacency graph.
- Coordinates are derived metadata.
- Neighbor information is directional and local.
- Fault annotations are stored explicitly rather than hidden.

## Core Types

### TileUID

Unique, immutable tile identity.

Recommended representation:

- `uint64_t` logical value
- encoded as `8` bytes on the wire

Requirements:

- stable across power cycles
- unique within a deployed board set
- never reused during a scan to represent a different physical tile

### RuntimeAddress

Temporary gateway-facing `I2C` address.

Recommended representation:

- `uint8_t`

Rules:

- valid only for the current active scan or boot session
- may change after reset or rescan
- must never be used as the canonical tile identity

### Direction

Enumerated edge identifier.

```text
0 = North
1 = East
2 = South
3 = West
```

Recommended representation:

- `uint8_t`

### EdgeState

Represents the tile-local observation for one directional edge.

```text
0 = Unknown
1 = Open
2 = NeighborDetected
3 = NeighborConfirmed
4 = HandshakeFailed
5 = Timeout
6 = Conflict
7 = Reserved
```

Guidance:

- `Unknown` is valid only before or during scan.
- `Open` means no neighbor was detected.
- `NeighborDetected` means physical presence was observed but identity is not trustworthy yet.
- `NeighborConfirmed` means a valid neighbor `TileUID` was obtained.
- `Conflict` is used by the gateway when local reports disagree.

## Fixed-Size Records

### NeighborReport

Represents one directional edge record as reported by a tile.

Suggested logical fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `direction` | `Direction` | Edge being described |
| `state` | `EdgeState` | Local observation outcome |
| `neighbor_uid` | `TileUID` | Neighbor identity if confirmed, else zero |
| `signal_quality` | `uint8_t` | Optional simple quality or confidence byte |
| `error_flags` | `uint8_t` | Edge-local fault bits |

Notes:

- This must be fixed-size even if some fields are unused.
- `neighbor_uid = 0` means not available for this edge report.

### TileRecord

Canonical gateway-side record for one discovered tile.

Suggested logical fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `tile_uid` | `TileUID` | Permanent identity |
| `runtime_address` | `RuntimeAddress` | Current I2C address |
| `protocol_version` | `uint8_t` | Tile protocol version |
| `scan_generation` | `uint16_t` | Scan/session marker |
| `tile_flags` | `uint16_t` | Tile-level health and validation flags |
| `neighbors[4]` | `NeighborReport[4]` | `N/E/S/W` local edge reports |

Tile records are gateway-resident. A tile may serialize the same logical data, but the gateway owns the canonical copy.

### DiscoverySnapshot

Represents one full discovery pass from the gateway.

Suggested logical fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `snapshot_id` | `uint32_t` | Monotonic gateway-side snapshot number |
| `scan_generation` | `uint16_t` | Active discovery session ID |
| `tile_count` | `uint8_t` or `uint16_t` | Number of valid tile records |
| `fault_count` | `uint16_t` | Number of detected anomalies |
| `tiles[MAX_TILES]` | `TileRecord[]` | Collected tile records |
| `map_graph` | `MapGraph` | Reconciled topology |

For `MAX_TILES = 100`, fixed arrays are acceptable at the gateway and should be preferred over dynamic containers unless measurement proves otherwise.

### MapGraph

Canonical reconciled topology graph derived by the gateway.

Suggested logical fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `node_count` | `uint16_t` | Number of valid tile nodes |
| `edge_count` | `uint16_t` | Number of confirmed undirected edges |
| `nodes[MAX_TILES]` | `MapNode[]` | Tile-indexed node records |
| `edges[MAX_TILES * 4]` | `MapEdge[]` | Directional or normalized edge storage |

Recommended logical node fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `tile_uid` | `TileUID` | Node identity |
| `record_index` | `uint8_t` or `uint16_t` | Index into `tiles[]` |
| `coord` | `CoordinateAssignment` | Derived coordinate info |
| `node_flags` | `uint16_t` | Validation and consistency flags |

Recommended logical edge fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `from_uid` | `TileUID` | Source tile |
| `direction` | `Direction` | Source direction |
| `to_uid` | `TileUID` | Neighbor tile |
| `edge_flags` | `uint16_t` | Confirmation or conflict flags |

### CoordinateAssignment

Derived relative position for a tile in the current snapshot.

Suggested logical fields:

| Field | Type | Meaning |
| --- | --- | --- |
| `x` | `int16_t` | Relative X |
| `y` | `int16_t` | Relative Y |
| `coord_flags` | `uint8_t` | Validity, conflict, root marker |

Rules:

- Coordinates are assigned only by the gateway.
- Coordinates are not persistent across snapshots.
- Invalid or conflicting assignments must be represented explicitly.

## Normalization Rules

### Tile Identity

- Use `TileUID` as the canonical key across all gateway structures.
- Runtime addresses are lookup helpers only.

### Directional Symmetry

Opposite directions are:

```text
North <-> South
East  <-> West
```

A confirmed undirected adjacency requires reciprocal directional agreement after gateway reconciliation.

### Canonical Edge Rules

When building the graph:

- create a confirmed edge only when both sides agree on identity and direction
- create a faulted edge record when only one side reports the link
- create a conflict record when both sides report incompatible identities
- never silently drop contradictory data without a fault record

## Fault Flags

Suggested gateway-visible fault categories:

- duplicate `TileUID`
- invalid runtime address
- stale scan generation
- missing reciprocal edge
- handshake failure
- timeout
- malformed payload
- coordinate conflict
- unreachable tile during poll

These may be represented as bitfields for compact storage.

## Memory Planning Guidance

Phase 1 requires up to `100` tiles and at most `400` directional edge reports.

Practical planning rules:

- keep tile-local scan data to one record per direction
- keep gateway arrays preallocated for maximum size
- prefer fixed-capacity indexes and bitfields
- avoid linked structures and heap-dependent graphs

The gateway may use slightly richer data structures than tiles if still bounded and inspectable, but the logical schema should remain fixed-size and serializable.
