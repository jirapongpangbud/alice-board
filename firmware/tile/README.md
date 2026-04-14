# Tile Firmware Area

This directory is reserved for tile firmware implementation.

Planned responsibilities:

- hold immutable `TileUID`
- participate in runtime address claim
- probe local `N/E/S/W` directional links
- cache scan-local neighbor results
- return fixed-size discovery responses over `I2C`

Tile firmware should not:

- compute the global map
- assign global coordinates
- store game logic
- depend on dynamic memory

Implementation must stay small, deterministic, and aligned with the protocol and schema documents.
