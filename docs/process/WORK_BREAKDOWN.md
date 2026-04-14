# Work Breakdown

This document splits Phase 1 into engineering tasks with explicit inputs, outputs, and dependencies. It is intended to guide real implementation sequencing for a small embedded team.

## Workstream 1: Hardware Assumptions

### Description

Define the operating assumptions that firmware and tests depend on:

- power delivery model
- tile orientation convention
- connector expectations for `N/E/S/W`
- immutable `TileUID` source
- `I2C` electrical constraints and bus limits
- reset and rescan behavior

### Inputs

- project goals
- gateway choice: `ESP32-C3 SuperMini`
- tile count target: `100`
- communication split: `I2C` plus directional single-wire links

### Outputs

- documented electrical and logical assumptions
- orientation and naming conventions
- baseline risks and lab validation topics

### Dependencies

- none

## Workstream 2: Tile Identity and Addressing

### Description

Define how tiles are permanently identified and temporarily addressed during a scan.

### Inputs

- immutable `TileUID` decision
- shared `I2C` bus requirement
- gateway-centric control model

### Outputs

- runtime address lifecycle
- address-claim flow
- address uniqueness validation rules
- duplicate identity handling rules

### Dependencies

- Workstream 1

## Workstream 3: Directional Link Contract

### Description

Define the local tile-to-tile edge protocol for `N/E/S/W`.

### Inputs

- four-edge physical tile layout
- custom single-wire half-duplex link choice
- need for neighbor identity reporting

### Outputs

- edge state model
- handshake outcomes
- neighbor UID reporting rules
- timeout and handshake failure rules

### Dependencies

- Workstream 1

## Workstream 4: Tile Firmware Contract

### Description

Constrain tile behavior to the minimum needed for discovery.

### Inputs

- addressing model
- directional link contract
- embedded constraints

### Outputs

- tile state machine definition
- command-driven edge probe behavior
- scan-local cache behavior
- bounded command handlers
- tile-owned fault flags

### Dependencies

- Workstream 2
- Workstream 3

## Workstream 5: Gateway Discovery Pipeline

### Description

Define the full discovery flow executed by the gateway from reset to final map publication.

### Inputs

- tile firmware contract
- addressing model
- schema and protocol rules

### Outputs

- scan orchestration sequence
- polling order
- snapshot lifecycle
- rescan behavior
- gateway-visible fault policy

### Dependencies

- Workstream 2
- Workstream 4

## Workstream 6: Data Modeling

### Description

Define the fixed-size structures used to store observations and the canonical topology.

### Inputs

- maximum tile count
- edge state model
- gateway map requirements

### Outputs

- tile record model
- neighbor report model
- discovery snapshot model
- graph and coordinate assignment model

### Dependencies

- Workstream 2
- Workstream 3
- Workstream 5

## Workstream 7: Topology Reconciliation

### Description

Define how the gateway converts tile-local observations into a reliable global map.

### Inputs

- neighbor reports
- graph model
- coordinate derivation rules

### Outputs

- reciprocal edge matching rules
- conflict classification
- graph normalization rules
- coordinate conflict handling

### Dependencies

- Workstream 5
- Workstream 6

## Workstream 8: Test Strategy

### Description

Define the bench and simulation strategy before firmware implementation grows.

### Inputs

- architecture
- protocol
- schema
- known failure classes

### Outputs

- acceptance scenarios
- fault injection scenarios
- layout fixture set
- rescan regression matrix

### Dependencies

- Workstream 4
- Workstream 5
- Workstream 6
- Workstream 7

## Recommended Implementation Order

The practical build order is:

1. Confirm hardware assumptions and naming conventions.
2. Lock tile identity and runtime address behavior.
3. Lock edge handshake and neighbor report semantics.
4. Implement shared protocol constants and record layouts.
5. Implement tile scan-local behavior.
6. Implement gateway discovery orchestration.
7. Implement graph reconciliation and coordinate derivation.
8. Build simulation and bench tests for stable layouts and faults.

## Delivery Definition for Phase 1

Phase 1 is complete when:

- the gateway can discover the full set of reachable tiles
- each tile can report `N/E/S/W` neighbor status
- the gateway builds a graph from current observations
- a full rescan replaces stale topology with fresh data
- known faults are surfaced explicitly
- the documented limits remain bounded and embedded-safe
