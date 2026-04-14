# Shared Protocol Area

This directory is reserved for definitions shared between gateway and tile implementations.

Expected contents in a later implementation phase:

- command IDs
- protocol version constants
- status codes
- fixed-size record layouts
- direction and edge-state enums

Shared definitions should remain minimal and stable so both sides interpret discovery messages the same way.
