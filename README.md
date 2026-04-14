# Alice Board

Alice Board is a modular board game platform built from hardware tiles connected through a central gateway. The immediate goal is not gameplay. Phase 1 is limited to discovering which tiles exist, which neighboring tiles are connected on each side, and building a consistent map in the gateway after startup or after a full rearrangement.

The system is intentionally simple:

- One gateway based on `ESP32-C3 SuperMini`
- Up to `100` microcontroller-based tiles
- `I2C` between gateway and tiles for centralized control
- One custom single-wire half-duplex link on each tile edge for local neighbor detection
- Gateway-owned topology model and rescan flow

## Why This Project Exists

A physical board made from rearrangeable tiles is only useful if the system can reliably understand its current layout. Before adding any game-specific rules, scoring, visuals, or networking, the platform needs a robust discovery layer that answers three questions:

1. Which tiles are currently present?
2. Which neighboring tiles are connected in each direction?
3. What is the resulting board map from the gateway's point of view?

This repository is organized to answer those questions first and to keep the early architecture practical for embedded systems.

## System Architecture Summary

The architecture is deliberately gateway-centric.

- Tiles are lightweight endpoints. Each tile owns only local knowledge:
  - its immutable `TileUID`
  - its temporary runtime `I2C` address
  - the current state of its four directional links: `N`, `E`, `S`, `W`
- The gateway is the coordinator and the source of truth. It is responsible for:
  - starting a discovery cycle
  - explicitly instructing tiles when to probe their surrounding edges
  - assigning or confirming runtime I2C addresses
  - polling each tile for its local neighbor report
  - reconciling inconsistent or incomplete reports
  - building the canonical topology graph
  - deriving relative coordinates when the graph is consistent
  - clearing and rebuilding the map on a full rescan

That split keeps tile firmware small and bounded while allowing the gateway to own the more complex logic.

## High-Level Discovery Flow

The Phase 1 operating model is:

1. The gateway triggers a discovery session or rescan.
2. Tiles enter a known scan state and claim temporary runtime `I2C` addresses.
3. The gateway explicitly commands tiles to probe their four directional single-wire interfaces.
4. Each commanded tile scans its surrounding edges and exchanges a minimal neighbor identity handshake with directly adjacent tiles.
5. The gateway polls each addressed tile over `I2C` and collects:
   - tile identity
   - protocol version
   - per-edge state
   - neighbor identity, if detected
6. The gateway merges all reports into an adjacency graph.
7. The gateway derives relative coordinates from the graph when possible.
8. The previous map is discarded and replaced with the current scan result.

The map built in Phase 1 is not gameplay state. It is only a validated physical topology snapshot.

## Phase Roadmap

### Phase 1: Discovery

Phase 1 is the only scope currently documented for implementation.

- Detect all connected tiles
- Discover neighbor relationships for `N/E/S/W`
- Support full rescan after board rearrangement
- Build a gateway-resident map using graph relationships and derived coordinates
- Handle common faults such as missing tiles, duplicate identities, asymmetric reports, and stale scan state

### Later Phases

Later phases are intentionally deferred until discovery is stable.

- Game logic and rules
- Tile role assignment
- User interface or visualization
- Network connectivity
- OTA updates
- Multi-gateway or distributed coordination

## Design Principles

- Prefer centralized logic in the gateway.
- Keep tile behavior local, bounded, and deterministic.
- Trigger tile scans by explicit gateway command, not autonomous tile behavior.
- Avoid dynamic memory and unbounded message formats.
- Favor explicit scan/reset flows over always-on background complexity.
- Build documents and interfaces first so firmware work starts from a stable contract.

## Repository Layout

This repo is docs-first. The structure is intended to look like a real engineering project before firmware code begins.

```text
.
├── README.md
├── agent.md
├── docs/
│   ├── engineering/
│   ├── process/
│   └── product/
├── firmware/
│   ├── gateway/
│   └── tile/
├── hardware/
├── shared/
│   └── protocol/
└── tests/
    └── simulation/
```

## Quick Start Concept

There are no firmware commands yet. The intended reading and implementation path is:

1. Read `README.md` for the system goal and repository layout.
2. Read `docs/product/PRD.md` and `docs/product/SCOPE.md` to understand what Phase 1 must and must not do.
3. Read `docs/engineering/ARCHITECTURE.md` for component responsibilities and discovery flow.
4. Read `docs/engineering/COMM_PROTOCOL.md` and `docs/engineering/DB_SCHEMA.md` before writing any firmware or tests.
5. Read `docs/process/DECISIONS.md` and `docs/process/WORK_BREAKDOWN.md` to understand tradeoffs, assumptions, and work sequencing.
6. Follow `agent.md` when generating or reviewing code.

## Current State

This repository currently defines:

- project structure
- product scope
- engineering contracts
- data model
- communication protocol
- work breakdown for implementation

This repository does not yet contain:

- gateway firmware
- tile firmware
- production test harnesses
- electrical schematics
- manufacturing files

Those are the next steps after the documentation baseline is accepted.
