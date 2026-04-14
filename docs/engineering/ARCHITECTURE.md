# Architecture

## System Overview

Alice Board Phase 1 is a topology discovery system for a modular hardware board made from connected tiles. A gateway identifies the set of attached tiles, gathers each tile's local neighbor observations, and builds a global board map.

The architecture is intentionally asymmetric:

- Tiles are simple sensing and reporting devices.
- The gateway owns orchestration, reconciliation, and the canonical topology model.

This keeps the distributed hardware manageable while still allowing the final map to reflect the physical arrangement of tiles.

## Component Model

### Gateway

The gateway is based on an `ESP32-C3 SuperMini` and has the following responsibilities:

- start and end discovery sessions
- reset gateway-side map state before each scan
- trigger tile scan state reset
- explicitly command when a tile may scan its surrounding edges
- coordinate runtime `I2C` address claim at startup or rescan
- poll every discovered tile for identity and neighbor state
- detect protocol or data integrity errors
- merge local reports into a global graph
- derive relative coordinates from the graph when consistent
- expose a complete discovery snapshot to future software layers

The gateway is the only component allowed to maintain the full board map.

### Tile

Each tile is a microcontroller-based unit with:

- one immutable `TileUID`
- one temporary runtime `I2C` address
- four directional single-wire half-duplex links
- local state for edge probe results

Tile responsibilities are intentionally narrow:

- respond to discovery control commands from the gateway
- participate in runtime address claim
- probe local `N/E/S/W` edges only when instructed by the gateway
- exchange a minimal identity handshake with directly adjacent neighbors
- cache the current edge report for the active scan
- return fixed-size reports over `I2C`

Tiles do not:

- compute the global map
- assign coordinates
- arbitrate conflicts between reports
- run gameplay logic
- autonomously start discovery scans
- coordinate scans among themselves

## Communication Boundaries

### Gateway to Tile: I2C

`I2C` is the control and collection channel.

Use `I2C` for:

- bus-level discovery control
- runtime address assignment or confirmation
- protocol version reporting
- tile identity query
- scan status query
- neighbor report retrieval
- health and fault reporting

Reasons for using `I2C` here:

- centralized, addressable polling model
- clean fit for gateway-owned orchestration
- simpler framing than trying to route global state across tile links

### Tile to Tile: Custom Single-Wire Half-Duplex Links

Each tile edge exposes one directional interface: `N`, `E`, `S`, `W`.

Use the edge link for:

- local presence detection
- minimal handshake with an adjacent tile
- neighbor `TileUID` exchange
- optional edge-level integrity indicator for the current scan

The edge link is not a shared global bus. It is a local adjacency mechanism. Each edge only describes the directly touching neighbor on that side.

Reasons for keeping this custom and narrow:

- lower complexity than turning edge links into a distributed network
- easier to reason about electrical behavior and timing
- enough information for the gateway to reconstruct global topology

## Discovery Lifecycle

### 1. Scan Reset

The gateway starts by invalidating its previous discovery snapshot. It then instructs all tiles to clear any cached edge report and to enter a known pre-scan state.

Requirements:

- no stale topology data may survive into the new scan
- runtime addresses may be reassigned
- tiles must not keep previous neighbor assumptions after reset

### 2. Runtime Address Claim

After reset, tiles move through a startup address-claim sequence so that each tile becomes individually addressable on the shared `I2C` bus.

Phase 1 assumption:

- runtime address is temporary
- `TileUID` is permanent
- gateway validates uniqueness at the identity layer, not by trusting addresses alone

This flow is documented as a controlled startup sequence rather than a passive plug-and-play guarantee because address claim is one of the main early system risks.

### 3. Local Edge Probe

The gateway explicitly instructs a tile to probe its four edges. For each direction, the commanded tile determines:

- open edge with no neighbor
- neighbor detected but identity handshake failed
- neighbor detected with valid `TileUID`
- local error or timeout

The tile stores this data in a fixed-size report structure for gateway polling.

Requirements:

- tiles remain idle with respect to edge probing until the gateway issues the scan command
- tiles do not launch periodic or self-triggered discovery activity
- the scan result belongs to the current `scan_generation` only

### 4. Gateway Poll and Snapshot Collection

The gateway polls every known runtime address and collects:

- `TileUID`
- protocol version
- scan generation or freshness marker
- four directional edge records
- tile-local error flags

The gateway treats tile reports as observations, not final truth.

### 5. Topology Reconciliation

The gateway builds the canonical adjacency graph by comparing reports from both sides of every claimed edge.

Expected behaviors:

- reciprocal matches become confirmed edges
- one-sided links become flagged anomalies
- conflicting claims remain visible as faults, not silently corrected
- duplicate `TileUID` values invalidate the affected records

### 6. Coordinate Derivation

The canonical data model is a graph. Relative coordinates are derived from graph traversal after reconciliation.

Coordinate derivation rules:

- choose an arbitrary root tile as `(0,0)`
- apply directional deltas:
  - `N = (0, -1)`
  - `E = (1, 0)`
  - `S = (0, 1)`
  - `W = (-1, 0)`
- stop and flag an inconsistency if traversal assigns conflicting coordinates to the same `TileUID`

Coordinates are derived metadata for the current scan, not persistent identity.

## Data Flow

The end-to-end data flow is:

1. Gateway begins scan.
2. Tile clears scan-local cache.
3. Gateway issues the edge-probe command.
4. Tile probes each directional edge.
5. Neighbor tile handshake returns either:
   - no response
   - invalid response
   - valid `TileUID`
6. Tile stores a fixed edge report.
7. Gateway polls the tile over `I2C`.
8. Gateway stores tile reports in a discovery snapshot.
9. Gateway reconciles all reports into a graph.
10. Gateway derives coordinates and fault annotations.
11. Gateway publishes the new map as the current snapshot.

## Failure Handling

Phase 1 must fail clearly rather than failing silently.

Key failure classes:

- duplicate tile identity
- missing tile during polling
- address-claim collision
- asymmetric edge reports
- edge handshake timeout
- malformed protocol response
- coordinate conflict during graph traversal
- mid-scan board rearrangement

Gateway behavior on faults:

- preserve the raw observations where possible
- mark the affected edge or tile as faulty
- avoid inventing corrected topology data
- allow a full rescan to rebuild from scratch

Tile behavior on faults:

- record simple explicit error states
- avoid retries that can stall the whole system
- return bounded failure codes to the gateway
- wait for a new explicit gateway command before attempting another edge scan

## Architectural Constraints

- Maximum supported tiles: `100`
- Maximum logical neighbors per tile: `4`
- All tile reports must be fixed-size
- Tile firmware must not require heap allocation
- Global topology belongs only to the gateway
- Discovery must support repeated full rescans after rearrangement

## Out of Scope for This Architecture

The following are intentionally excluded from Phase 1:

- game rules and tile roles
- visual UI rendering
- internet or app connectivity
- OTA update systems
- persistent game state
- distributed consensus across tiles

Those concerns can only be added after discovery is stable, measurable, and testable.
