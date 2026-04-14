# Scope

This document defines what Phase 1 includes and excludes so implementation stays disciplined.

## Included in Phase 1

Phase 1 includes only the discovery foundation required to understand the physical board layout.

### Discovery Capabilities

- detect all reachable tiles connected to the system
- identify each tile using immutable `TileUID`
- assign or confirm temporary runtime `I2C` addresses for the active scan
- let the gateway explicitly command when tiles perform neighbor scans
- probe and report directional neighbor state for `N/E/S/W`
- gather fixed-size neighbor reports from every tile
- build a gateway-resident adjacency graph
- derive relative coordinates from that graph when consistent
- support a full rescan after board rearrangement

### Fault Handling

- surface duplicate tile identities
- surface address claim failures or ambiguity
- surface missing reciprocal neighbor reports
- surface edge handshake failures and timeouts
- surface malformed or stale scan data

### Engineering Deliverables

- repo structure for firmware, shared protocol, tests, and hardware work
- documented architecture
- documented communication protocol
- documented data model
- documented work breakdown and design decisions

## Excluded from Phase 1

The following are explicitly out of scope and should not be added indirectly:

### Game Logic

- rules engines
- turn management
- scoring
- tile role behavior beyond discovery
- game state persistence

### User Interface

- mobile app
- web UI
- onboard display workflow
- polished visualization of the board map

### Networking

- Wi-Fi control plane
- cloud services
- remote synchronization
- multiplayer state exchange

### Device Management

- OTA firmware updates
- advanced configuration systems
- manufacturing provisioning tooling beyond basic UID assumptions

### Advanced System Features

- distributed topology computation across tiles
- multi-gateway support
- automatic live topology streaming outside an explicit scan model
- generalized messaging frameworks for future features

## Scope Guardrails

If a proposed task does not directly improve:

- tile detection
- neighbor detection
- rescan correctness
- gateway map construction
- Phase 1 fault visibility

then it is probably not Phase 1 work.

## Exit Criteria for Scope Completion

Phase 1 scope is complete when:

- the documented protocol and schema are implemented
- the system can build a correct topology snapshot on supported layouts
- the system can discard stale topology and rebuild on full rescan
- known failure classes are observable and testable

Anything beyond that should be treated as the next phase, not as a hidden extension of this one.
