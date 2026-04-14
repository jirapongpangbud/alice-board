# Tile Firmware Area

This directory now contains the initial tile firmware skeleton for Phase 1 discovery.

## Current Structure

```text
firmware/tile/
├── README.md
├── include/
│   └── alice_tile.h
└── src/
    └── alice_tile.c
```

## Implemented Baseline

- tile device struct and runtime state
- command handler entry point for all Phase 1 discovery commands
- explicit scan-state transitions
- fixed-size response builders using `shared/protocol/alice_protocol.h`
- mockable edge-probe hook
- asynchronous probe completion path via `alice_tile_complete_probe()`

## Responsibilities

The current tile skeleton is responsible for:

- hold immutable `TileUID`
- participate in runtime address claim
- probe local `N/E/S/W` directional links
- cache scan-local neighbor results
- return fixed-size discovery responses over `I2C`

The skeleton intentionally keeps transport and hardware details outside the core state machine so it can be reused by both simulation and embedded drivers.

## Not Implemented Yet

- actual `I2C` transport glue
- actual directional single-wire probing driver
- electrical timing control
- interrupt integration
- production logging or diagnostics

## Rules

Tile firmware should not:

- compute the global map
- assign global coordinates
- store game logic
- depend on dynamic memory
- start discovery scans without an explicit gateway command

Implementation must stay small, deterministic, and aligned with:

- `docs/engineering/ARCHITECTURE.md`
- `docs/engineering/COMM_PROTOCOL.md`
- `shared/protocol/alice_protocol.h`
