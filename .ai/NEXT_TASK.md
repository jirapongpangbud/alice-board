# Next Task

## Immediate Goal

Start the first executable testing-facing work for `Phase 1` using the tile and gateway skeletons together.

The next task should build the first simulation harness and protocol-level discovery tests.

## Recommended Next Task

Create a simulation test harness under `tests/simulation` that can drive the gateway skeleton against one or more tile skeleton instances.

Scope of this task:

- implement a mock transport adapter that translates gateway requests into tile command calls
- support controlled claim-address behavior for multiple simulated tiles
- add deterministic discovery scenarios:
  - single tile
  - two connected tiles
  - scan with no tiles
- add fault-oriented scenarios where practical:
  - duplicate UID
  - tile stays busy
  - transport failure on one tile
- verify snapshot contents, phase completion, and fault counting

This task should not implement real hardware drivers or final graph reconciliation yet. It should prove that the current protocol and command sequencing work end to end in simulation.

## Why This Task Comes Next

It is the narrowest high-value step after documentation:

- it validates the gateway and tile skeletons against each other
- it exposes protocol and state-machine mismatches early
- it provides repeatable regression tests before board bring-up
- it reduces hardware debugging load by catching logic errors off-target first

## Expected Deliverables

- simulation harness code in `tests/simulation`
- at least a small set of executable discovery tests
- documented expected scenarios and assertions
- directory `README` update if implementation layout needs explanation
- no dynamic memory
- no gameplay logic
- no real hardware driver implementation yet

## Acceptance Criteria

The next task is complete when:

- the gateway skeleton can run against simulated tiles
- discovery succeeds for simple deterministic layouts
- at least one fault scenario is exercised and asserted
- the test harness uses the same shared protocol contract as firmware code
- the implementation still matches `docs/engineering/COMM_PROTOCOL.md` and `docs/engineering/ARCHITECTURE.md`

## After This Task

Once shared protocol definitions are in place, the recommended order is:

1. build simulation fixtures for protocol and topology cases
2. implement graph reconciliation and coordinate derivation
3. bring up the first real-board test with `1 gateway + 2 tiles`
4. expand fault coverage and timing validation

## Constraints For The Next Task

- do not add gameplay logic
- do not add networking
- do not add OTA or configuration systems
- do not let tiles initiate discovery on their own
- do not use unbounded or heap-dependent structures

## Notes For Whoever Picks This Up

If simulation exposes ambiguity in command order, payload meaning, or fault handling, resolve it in the docs and shared header in the same change set. Do not let tests silently normalize broken behavior.
