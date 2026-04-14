# Next Task

## Immediate Goal

Start the first executable firmware-facing work for `Phase 1` on the gateway side.

The next task should implement the gateway discovery orchestration skeleton against the shared protocol contract and the tile skeleton.

## Recommended Next Task

Create the initial gateway discovery skeleton under `firmware/gateway` with explicit scan lifecycle handling.

Scope of this task:

- define gateway-local discovery session state
- implement the high-level scan lifecycle:
  - start scan generation
  - reset tile scan state
  - coordinate runtime address claim
  - trigger edge probing
  - poll scan status
  - collect neighbor reports
- store tile observations in bounded gateway-side records
- keep transport calls behind a narrow mockable boundary
- avoid full topology reconciliation and coordinate derivation if that would make the first gateway slice too large

This task should not implement gameplay, real hardware drivers, or final graph reconciliation yet. It should only establish a safe executable gateway-side orchestration skeleton.

## Why This Task Comes Next

It is the narrowest high-value step after documentation:

- it proves the shared contract works end to end against the tile skeleton
- it locks the gateway-owned scan sequence early
- it creates the execution path needed for simulation fixtures
- it prepares the repo for the first `1 gateway + 2 tiles` bring-up

## Expected Deliverables

- gateway discovery state and command sequencing skeleton in `firmware/gateway`
- explicit scan-session lifecycle
- bounded storage for per-tile observations
- mockable transport interface for tile command exchange
- directory `README` update if implementation layout needs explanation
- no dynamic memory
- no gameplay logic
- no full hardware driver implementation yet

## Acceptance Criteria

The next task is complete when:

- gateway code can execute the documented Phase 1 command sequence
- scan-session state is explicit and bounded
- tile interactions go through the shared protocol structs
- the design can be driven by mocks in simulation
- the implementation still matches `docs/engineering/COMM_PROTOCOL.md` and `docs/engineering/ARCHITECTURE.md`

## After This Task

Once shared protocol definitions are in place, the recommended order is:

1. implement gateway discovery orchestration skeleton
2. build simulation fixtures for protocol and topology cases
3. implement graph reconciliation and coordinate derivation
4. bring up the first real-board test with `1 gateway + 2 tiles`

## Constraints For The Next Task

- do not add gameplay logic
- do not add networking
- do not add OTA or configuration systems
- do not let tiles initiate discovery on their own
- do not use unbounded or heap-dependent structures

## Notes For Whoever Picks This Up

If gateway implementation reveals ambiguity in scan ordering or response handling, resolve it in the docs and shared header in the same change set. Do not let code silently redefine the protocol.
