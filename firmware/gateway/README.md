# Gateway Firmware Area

This directory is reserved for gateway firmware implementation.

Planned responsibilities:

- discovery session control
- runtime `I2C` address coordination
- tile polling over `I2C`
- snapshot assembly
- topology reconciliation
- coordinate derivation
- rescan handling

Implementation should follow the contracts in:

- `docs/engineering/ARCHITECTURE.md`
- `docs/engineering/COMM_PROTOCOL.md`
- `docs/engineering/DB_SCHEMA.md`

Phase 1 guidance:

- keep gateway logic authoritative for global topology
- keep all limits explicit and bounded
- build around a clear scan lifecycle rather than ad hoc polling
