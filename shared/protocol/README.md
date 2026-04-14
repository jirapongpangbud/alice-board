# Shared Protocol Area

This directory contains the shared discovery contract used by both `gateway` and `tile` firmware.

## Current Files

- `alice_protocol.h`

## What `alice_protocol.h` Defines

- protocol version constant
- compile-time limits for Phase 1 discovery
- command IDs
- status codes
- scan states
- direction and edge-state enums
- claim result values
- fixed-size wire records for request and response payloads
- little-endian helper types for `uint16_t` and `uint64_t`
- compile-time size checks for on-wire layouts

## Design Notes

- all on-wire multi-byte integers are encoded as little-endian byte arrays
- request and response records are fixed-size and packed
- the header is intentionally transport-agnostic
- this area does not implement `I2C`, edge-link logic, or gateway behavior

## Usage Rules

- both firmware sides must include these definitions instead of redefining protocol values locally
- if a wire field changes, update the docs and this header in the same change set
- do not add gameplay or configuration protocol here during Phase 1
