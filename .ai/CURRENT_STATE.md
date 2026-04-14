# Current State

## Repository Status

This repository is currently in a documentation-first initialization state for `Phase 1: Topology Discovery`.

Implemented so far:

- root project documentation
- engineering architecture and protocol documents
- product scope and PRD
- process decisions and work breakdown
- placeholder directories for future firmware, shared protocol, tests, and hardware work

Not implemented yet:

- gateway firmware
- tile firmware
- shared protocol code
- simulation harness
- hardware test fixtures
- board-level validation scripts

## Current Architectural Direction

The system is intentionally gateway-centric.

- `gateway` is the only component that owns the global topology map
- `tile` owns only local identity, local edge observations, and fixed-size report data
- `gateway` explicitly commands when a tile may scan its surrounding edges
- `tile` must not start discovery scans autonomously
- canonical topology is stored as an adjacency graph with derived coordinates

## Locked Phase 1 Decisions

- gateway platform: `ESP32-C3 SuperMini`
- tile count target: up to `100`
- gateway to tile communication: `I2C`
- tile to tile communication: custom single-wire half-duplex per direction
- permanent tile identity: immutable `TileUID`
- temporary tile addressing: runtime `I2C` address per scan/session
- rescan model: full rebuild from fresh observations
- memory model: bounded, fixed-size, embedded-safe structures

## Documents Available

Primary references:

- `README.md`
- `agent.md`
- `docs/engineering/ARCHITECTURE.md`
- `docs/engineering/COMM_PROTOCOL.md`
- `docs/engineering/DB_SCHEMA.md`
- `docs/product/PRD.md`
- `docs/product/SCOPE.md`
- `docs/process/DECISIONS.md`
- `docs/process/WORK_BREAKDOWN.md`

## Important Behavioral Rules

- discovery is command-driven by the gateway
- tiles scan edges only after explicit gateway trigger
- tiles do not compute global topology
- the gateway reconciles conflicting reports instead of trusting a single tile blindly
- a full rescan must clear stale state before rebuilding the snapshot

## Open Technical Risks

- runtime address claim on a shared `I2C` bus
- electrical reliability of the directional single-wire link
- timing bounds for edge scan and gateway polling
- duplicate `TileUID` handling
- deterministic behavior during rearrangement and rescan

## Recommended Reading Order For New Work

1. `docs/product/PRD.md`
2. `docs/product/SCOPE.md`
3. `docs/engineering/ARCHITECTURE.md`
4. `docs/engineering/COMM_PROTOCOL.md`
5. `docs/engineering/DB_SCHEMA.md`
6. `docs/process/DECISIONS.md`
7. `docs/process/WORK_BREAKDOWN.md`
8. `agent.md`

## Definition of "Current Progress"

Progress is currently at the point where the project has a clear specification and directory structure, but no executable firmware or tests.

The next work should focus on locking the implementation-facing contract, not on adding product features.
