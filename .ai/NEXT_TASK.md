# Next Task

## Immediate Goal

Start the first implementation-facing work for `Phase 1` without writing full firmware yet.

The next task should define the shared protocol contract in a way that both `gateway` and `tile` firmware can implement consistently.

## Recommended Next Task

Create the initial shared protocol specification package under `shared/protocol` as code-ready definitions.

Scope of this task:

- define command IDs
- define protocol version constant
- define status codes
- define `Direction` enum
- define `EdgeState` enum
- define fixed-size wire-level record layouts
- define explicit scan states such as `idle`, `awaiting_probe`, `probing`, `ready`, `fault`
- define compile-time limits like maximum neighbors and fixed report sizes

This task should not implement transport drivers or business logic yet. It should only lock the shared contract.

## Why This Task Comes Next

It is the narrowest high-value step after documentation:

- both firmware sides depend on the same constants and message shapes
- test harnesses also depend on these definitions
- it reduces drift between docs and implementation
- it keeps Phase 1 focused on bounded interfaces first

## Expected Deliverables

- shared protocol header or equivalent definition files in `shared/protocol`
- a short `README` update in that directory if needed
- explicit mapping from documented protocol to code-level constants
- no dynamic memory
- no firmware task scheduling
- no hardware driver implementation yet

## Acceptance Criteria

The next task is complete when:

- gateway and tile teams can refer to one shared source of truth for protocol constants
- every documented Phase 1 command has a code-level identifier
- response status codes are fixed and unambiguous
- all wire-level records are bounded and fixed-size
- the shared definitions still match `docs/engineering/COMM_PROTOCOL.md` and `docs/engineering/DB_SCHEMA.md`

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
- do not use unbounded or heap-dependent structures in tile-facing definitions

## Notes For Whoever Picks This Up

If the shared protocol task reveals ambiguity, resolve it in the docs first or in the same change set. Do not let code silently redefine the protocol.
