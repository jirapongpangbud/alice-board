# Key Decisions

This document records the major design decisions for Phase 1 and why they were chosen.

## Decision 1: Gateway Owns the Global Map

### Choice

The gateway is the only component allowed to build and store the full topology.

### Why

- tiles have limited memory and should stay simple
- global reconciliation is easier in one place
- rescan logic is clearer when one component owns map replacement
- testing and debugging are easier with a single canonical source

### Tradeoff

- gateway firmware becomes more complex than tile firmware
- the system depends on the gateway being healthy for discovery

This is acceptable because the gateway has more resources and is the natural orchestration point.

## Decision 1A: Gateway Also Controls Scan Timing

### Choice

Tiles scan their surrounding edges only when the gateway explicitly instructs them to do so.

### Why

- keeps scan timing deterministic
- avoids background tile behavior that can create stale or conflicting data
- makes rescans easier to reason about and test
- reinforces the gateway as the orchestration point

### Tradeoff

- discovery depends on the gateway issuing the correct scan sequence

That tradeoff is acceptable because command-driven scanning is simpler and safer than autonomous tile behavior in Phase 1.

## Decision 2: Tiles Use Immutable `TileUID`

### Choice

Each tile has a permanent unique identity, separate from its temporary runtime address.

### Why

- physical position can change after rearrangement
- runtime addresses may be reassigned
- identity must survive resets and rescans
- reconciliation becomes much simpler when identity is stable

### Tradeoff

- manufacturing or provisioning must guarantee unique IDs

That tradeoff is better than trying to infer identity from topology.

## Decision 3: Runtime I2C Address Is Temporary

### Choice

Tiles claim or receive temporary runtime addresses for the current scan or session.

### Why

- the system needs individual polling on a shared `I2C` bus
- fixed physical address assignment does not fit a modular tile system well
- temporary addressing supports rescan and replacement workflows

### Tradeoff

- startup address assignment becomes a system risk that must be validated carefully

The risk is acceptable as long as it is treated explicitly and tested early.

## Decision 4: Tile-to-Tile Links Stay Local and Minimal

### Choice

Each edge uses a custom single-wire half-duplex link only for direct neighbor detection and identity exchange.

### Why

- the system only needs local adjacency for Phase 1
- richer peer protocols would increase implementation and electrical risk
- the gateway can reconstruct the global map from local facts

### Tradeoff

- the tile edge link cannot carry complex future features without extension

That is acceptable because discovery should stay isolated from later gameplay concerns.

## Decision 5: Canonical Map Is a Graph First, Coordinates Second

### Choice

The gateway stores an adjacency graph as the source of truth and derives coordinates from it.

### Why

- graph relationships are the direct discovery result
- coordinates are derived and may become invalid if reports conflict
- graph-first modeling makes faults easier to preserve without inventing positions

### Tradeoff

- consumers may need both graph and coordinate views

That is still preferable to forcing all discovery results into a coordinate-only model.

## Decision 6: Full Rescan Rebuilds the Map from Scratch

### Choice

A rescan clears previous topology state and rebuilds the snapshot from fresh reports.

### Why

- tiles can be rearranged physically
- incremental topology updates are more complex and easier to get wrong
- stale neighbor assumptions are dangerous in embedded discovery logic

### Tradeoff

- a rescan may cost more time than an incremental update

That tradeoff is acceptable in Phase 1 because correctness is more important than optimization.

## Decision 7: Fixed-Size Structures Everywhere That Matter

### Choice

The protocol and tile-side data structures are fixed-size. Gateway structures should also remain bounded and preferably fixed-capacity.

### Why

- supports no-dynamic-memory constraints on tiles
- makes serialization inspectable
- improves predictability and fault handling
- simplifies worst-case resource analysis

### Tradeoff

- less flexible than dynamic containers

That is acceptable because Phase 1 scope is tightly defined.

## Working Assumptions

- maximum system size is `100` tiles
- each tile has at most `4` neighbors
- there is one active gateway for a board
- board discovery is a bounded operation, not a continuous background negotiation
- SDK and implementation language are not locked yet

## Known Risks

- runtime address claim on a shared `I2C` bus may require careful startup sequencing
- electrical behavior of the edge link must be validated with real hardware
- duplicate or malformed tile identities can invalidate the map
- mid-scan rearrangement may create inconsistent snapshots if not detected
- protocol timing may need tuning once real boards exist

## Deferred Items

These are explicitly postponed until discovery is working:

- game-specific tile roles
- player interaction logic
- UI or map rendering
- persistent storage of game sessions
- networking and remote control
- OTA updates
- performance optimization beyond bounded discovery correctness
