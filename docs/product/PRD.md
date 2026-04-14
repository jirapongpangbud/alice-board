# Product Requirements Document

## Problem Statement

Alice Board is a physical modular board built from hardware tiles. The board is only useful if the system can automatically understand how those tiles are currently arranged.

Today, the fundamental product problem is not game logic. It is physical topology awareness. Without a reliable discovery layer, every later feature depends on guesses, manual setup, or brittle assumptions.

Phase 1 exists to solve that foundational problem.

## Product Goal

The system must detect the current tile layout and build a usable map in the gateway after startup or after the user rearranges the board.

In practical terms, the system must:

- detect every reachable tile
- determine which neighbor is connected on each side of each tile
- have the gateway explicitly trigger tile-side neighbor scans
- support a full rescan after rearrangement
- produce a gateway-resident topology snapshot suitable for later game layers

## Target User

The immediate user for Phase 1 is the system itself and the engineering team building on top of it.

Secondary user:

- future gameplay software that needs a correct board map

Phase 1 is not yet optimized for an end-user interface. It is optimized for reliable machine-readable topology.

## User Story

As a user who physically rearranges modular tiles, I want the board to detect the current layout automatically so that later game software can use the correct board shape without manual configuration.

## Primary Use Cases

### Use Case 1: Initial Startup

The user powers on a board made from connected tiles.

Expected result:

- the gateway discovers all tiles
- the gateway identifies direct neighbor relationships
- the gateway stores a complete discovery snapshot

### Use Case 2: Rearrangement and Rescan

The user changes the board layout by moving or reconnecting tiles, then triggers or causes a full rescan.

Expected result:

- old topology data is discarded
- the new layout is detected
- the gateway replaces the previous snapshot with the new one

### Use Case 3: Faulty or Incomplete Layout

The user has a loose connection, missing tile, or partially failed tile.

Expected result:

- the system completes discovery with explicit fault markers where possible
- the gateway does not silently invent a clean topology

## Success Criteria

Phase 1 is successful when all of the following are true:

- the gateway can discover up to `100` tiles within the supported hardware limits
- each discovered tile reports directional neighbor state for `N/E/S/W`
- the gateway builds a canonical adjacency graph from the collected reports
- the gateway derives relative coordinates when the graph is consistent
- a full rescan clears stale data and rebuilds the current layout
- common failures are represented explicitly rather than hidden

## Non-Functional Requirements

- topology discovery must be bounded and restartable
- tile-side logic must fit low-memory embedded hardware
- message formats must be fixed-size or tightly bounded
- discovery must remain understandable and testable by a small engineering team

## Acceptance Scenarios

### Scenario A: Two-Tile Link

Given two properly connected tiles, when the gateway performs a discovery scan, then:

- both tiles are detected
- each reports the other on the appropriate opposite edge
- the gateway builds one confirmed adjacency

### Scenario B: Square Layout

Given a `2x2` tile arrangement, when discovery completes, then:

- all four tiles are detected
- horizontal and vertical adjacencies are reciprocal
- derived coordinates are internally consistent

### Scenario C: Rearranged Layout

Given a previously scanned board, when the user rearranges the tiles and a full rescan occurs, then:

- the previous snapshot is invalidated
- only fresh observations are used
- the resulting map reflects the new arrangement

### Scenario D: Fault Condition

Given a board with an asymmetric or failing edge link, when discovery completes, then:

- the affected edge is flagged
- the gateway preserves the rest of the valid topology where possible
- the system does not silently mark the edge as healthy

## Product Boundaries

Phase 1 does not attempt to deliver gameplay, networking, or UI. Its sole product outcome is a correct and inspectable physical topology model.

That narrow focus is intentional. It reduces risk and creates a stable base for future phases.
