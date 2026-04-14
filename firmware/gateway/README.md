# Gateway Firmware Area

This directory now contains the initial gateway discovery orchestration skeleton for Phase 1.

## Current Structure

```text
firmware/gateway/
├── README.md
├── include/
│   └── alice_gateway.h
└── src/
    └── alice_gateway.c
```

## Implemented Baseline

- gateway discovery session state
- explicit discovery phases:
  - reset
  - address claim
  - edge probe trigger
  - scan status poll
  - neighbor report collection
- bounded snapshot storage for discovered tile observations
- narrow mockable transport interface for claim and addressed transactions
- synchronous `alice_gateway_run_discovery()` flow

## Responsibilities

The current gateway skeleton is responsible for:

- starting a new scan generation
- clearing prior snapshot state
- coordinating runtime address claim
- validating protocol responses
- collecting tile identity, status, and neighbor observations
- preserving per-tile faults in bounded records

## Not Implemented Yet

- real `I2C` transport driver
- broadcast/reset bus implementation
- graph reconciliation
- coordinate derivation
- production retry policy tuning
- hardware timing integration

## Rules

Gateway firmware should:

- remain the only owner of global topology state
- keep all buffers and tile arrays bounded
- treat transport and protocol issues as explicit faults
- continue discovery when possible instead of hanging on one tile

Implementation should follow:

- `docs/engineering/ARCHITECTURE.md`
- `docs/engineering/COMM_PROTOCOL.md`
- `docs/engineering/DB_SCHEMA.md`
- `shared/protocol/alice_protocol.h`
