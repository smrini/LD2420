# Changelog

## 0.2.1

Documentation correction -- no functional code changes.

- Corrected the "Wiring" section (README and source comments), which
  previously and incorrectly described OT1/OT2 as physically swapped on
  some units. They are not: physical pin position always matches the
  silkscreen. What actually differs by firmware version is which pin
  carries serial data vs. the presence-level output -- the manufacturer's
  printed diagram describes older firmware's assignment.
- Documented a real hazard the old wording didn't cover: whichever OT
  pin is *not* carrying serial data is still a module-driven output, and
  wiring `txPin` to it (instead of the module's actual RX pin) puts two
  drivers on one wire. Examples updated with an explicit warning at the
  pin definitions.

## 0.2.0

- Split line parsing into a standalone, Arduino-independent module
  (`LD2420Parser`), unit tested on PlatformIO's `native` platform.
- Added `setRoomExitGraceMs()` / `roomExitGraceMs()`: holds `isInRoom()`
  true for a configurable grace period after the last in-range
  detection, to smooth out flicker for a target near the cutoff.
- Added event callbacks: `onPresenceChange()`, `onRoomEnter()`,
  `onRoomExit()`.
- Added `detectBaud()`: tries 115200 then 256000 automatically and
  keeps whichever gets a response.
- New `EventCallbacks` example demonstrating all of the above together.
- Corrected `library.json`/`library.properties` to claim ESP32 only --
  the previous AVR/ESP8266 claim was inaccurate (the 4-argument
  `HardwareSerial::begin()` used here is ESP32-specific).

## 0.1.0

Initial release.

- Parses the LD2420's plain-text UART output (`OFF` / `ON` + `Range <n>`).
- Software distance cutoff (`isInRoom()`) for room-only filtering.
- Examples: `BasicRead`, `RoomPresence`.
- Does not yet implement the sensor's binary configuration protocol.
