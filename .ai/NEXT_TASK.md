# Next Task

## Immediate Goal

Start the first executable firmware-facing work for `Phase 1` on the tile side.

The next task should implement the tile scan-state machine skeleton against the shared protocol contract.

## Recommended Next Task

Create the initial tile firmware skeleton under `firmware/tile` with explicit state handling for discovery.

Scope of this task:

- define tile-local state struct for the active scan
- implement command handler skeletons for:
  - `RESET_SCAN_STATE`
  - `CLAIM_RUNTIME_ADDRESS`
  - `GET_TILE_IDENTITY`
  - `START_EDGE_PROBE`
  - `GET_SCAN_STATUS`
  - `GET_NEIGHBOR_REPORT`
  - `PING`
- keep edge probing as a stub or mockable function boundary
- populate fixed-size response records from `shared/protocol/alice_protocol.h`
- keep the implementation transport-agnostic where practical
- avoid real hardware driver code if it would force premature design decisions

This task should not implement gameplay, topology reconciliation, or full hardware drivers yet. It should only establish a safe executable tile-side skeleton.

## Why This Task Comes Next

It is the narrowest high-value step after documentation:

- it proves the shared contract is usable in real code
- it locks the command-driven scan lifecycle on the tile side
- it gives the gateway team a concrete responder model to target
- it creates a clean seam for later simulation and hardware bring-up

## Expected Deliverables

- tile protocol handler skeleton in `firmware/tile`
- explicit scan-state transitions
- fixed-size response builders using the shared protocol header
- directory `README` update if implementation layout needs explanation
- no dynamic memory
- no gameplay logic
- no full hardware driver implementation yet

## Acceptance Criteria

The next task is complete when:

- tile code can accept every documented Phase 1 command
- scan-state transitions are explicit and bounded
- tile never starts edge scans without `START_EDGE_PROBE`
- all responses use fixed-size shared protocol structs
- the implementation still matches `docs/engineering/COMM_PROTOCOL.md` and `docs/engineering/ARCHITECTURE.md`

## After This Task

Once shared protocol definitions are in place, the recommended order is:

1. implement tile scan-state machine skeleton
2. implement gateway discovery orchestration skeleton
3. build simulation fixtures for protocol and topology cases
4. bring up the first real-board test with `1 gateway + 2 tiles`

## Constraints For The Next Task

- do not add gameplay logic
- do not add networking
- do not add OTA or configuration systems
- do not let tiles initiate discovery on their own
- do not use unbounded or heap-dependent structures

## Notes For Whoever Picks This Up

If tile implementation reveals ambiguity in state transitions or payload meaning, resolve it in the docs and shared header in the same change set. Do not let code silently redefine the protocol.
