# AI Agent Working Rules

This file defines how AI agents and contributors should approach work in the Alice Board repository.

The project is an embedded systems project first. It is not a web app, not a distributed platform, and not a place to invent architecture for its own sake. Every decision should reduce implementation risk on low-resource hardware and move the system toward reliable topology discovery.

## Project Rules

- Treat `Phase 1` as topology discovery only.
- Keep the gateway as the single source of truth for global state.
- Keep tiles limited to local observation and response.
- Make edge scanning command-driven: tiles scan neighbors only when instructed by the gateway.
- Preserve fixed-size data structures and bounded execution paths.
- Assume low RAM, low flash budget, and no dynamic memory allocation in tile firmware.
- Prefer explicit state machines over hidden side effects.
- Make resets and rescans first-class behaviors, not recovery hacks.
- Document protocol changes before implementing them.
- Treat fault handling as part of the design, not as optional cleanup.

## Coding Philosophy

The required style for this project is:

- Simple over clever
- Robust over feature-rich
- Deterministic over adaptive
- Observable over implicit
- Embedded-safe over framework-heavy

Practical interpretation:

- Use small functions with clear ownership.
- Use enums, fixed buffers, and explicit status codes.
- Prefer compile-time limits such as `MAX_TILES`, `MAX_EDGES_PER_TILE`, and frame-size constants.
- Keep interrupt-facing logic minimal.
- Keep protocol payloads versioned and bounded.
- Separate local hardware handling from topology logic.

## Constraints

All generated code and designs should assume:

- No dynamic memory allocation on tiles
- Very limited RAM and stack space
- Up to `100` tiles in the system
- Four directional neighbor links per tile
- Shared `I2C` bus between gateway and tiles
- Temporary runtime addresses that may change between scans
- Full rescan support after physical rearrangement
- Partial failures must not corrupt the gateway map

## What Not To Do

Do not introduce any of the following without a documented reason and explicit approval:

- Complex distributed consensus between tiles
- Peer-to-peer map construction
- Runtime discovery protocols with unbounded retries
- Dynamic containers that grow without hard limits
- Generic abstraction layers that hide hardware behavior
- Heavy C++ patterns, exceptions, RTTI, or allocator-dependent designs
- Multi-stage protocol negotiation when a version byte is enough
- Background autonomous behavior that mutates topology state outside a scan
- Autonomous tile-side neighbor scanning outside explicit gateway control
- Optimization work before there is a measured bottleneck

This project does not need:

- service meshes
- event buses
- actor frameworks
- plugin systems
- speculative extensibility for future game modes

## Step-by-Step Task Approach

When working on a task, use this sequence:

1. Confirm the task belongs to Phase 1 discovery.
2. Read the relevant docs before touching code:
   - `docs/product/PRD.md`
   - `docs/product/SCOPE.md`
   - `docs/engineering/ARCHITECTURE.md`
   - `docs/engineering/COMM_PROTOCOL.md`
   - `docs/engineering/DB_SCHEMA.md`
3. Identify the bounded inputs, outputs, limits, and failure cases.
4. Decide whether the logic belongs on the tile, the gateway, or in shared protocol definitions.
5. Implement the smallest correct version that matches the documented contract.
6. Add or update tests for success paths and fault paths.
7. Review for memory safety, fixed-size behavior, and rescan correctness.
8. Document any design drift before merging.

## Safe Code Generation Checklist

Before generating code, confirm:

- The ownership boundary is clear.
- The data structure sizes are known.
- Buffer lengths are explicit.
- Retry counts and timeouts are finite.
- Error states are represented explicitly.
- No logic depends on heap allocation.
- No module assumes stable physical coordinates without gateway reconciliation.
- No tile stores or computes the full board topology.
- No tile initiates a discovery scan on its own.

When generating code:

- Prefer plain C or restrained embedded C++ as the implementation style.
- Use named constants for all protocol sizes, state counts, and timing values.
- Return explicit status results.
- Validate all external input, even on local buses.
- Keep serialization and parsing code simple and inspectable.
- Guard against stale scan data by requiring clear reset points.

## Review Expectations

Reviews should focus on:

- correctness under rescan
- fixed-size memory behavior
- clear ownership of state
- consistency with documented protocol
- fault containment
- test coverage for edge cases

If a change adds complexity, the burden is on the author to show that the complexity is necessary and still safe for an embedded target.

## Decision Heuristics

If there is a choice between two designs:

- choose the one that keeps more intelligence in the gateway
- choose the one with fewer persistent states in the tile
- choose the one with smaller and more explicit message formats
- choose the one that is easier to test on a bench with known layouts

If a change cannot be explained in one short paragraph to another embedded engineer, it is probably too complicated for Phase 1.

## Documentation Discipline

- Update docs when interfaces change.
- Keep product scope and engineering scope aligned.
- Record tradeoffs in `docs/process/DECISIONS.md`.
- Add new work items to `docs/process/WORK_BREAKDOWN.md` if architecture assumptions change.

The repo should stay readable to a new engineer who joins before any firmware exists.
