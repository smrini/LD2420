# LD2420

[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Arduino/PlatformIO driver for the **HLK-LD2420** 24GHz presence sensor's
plain-text UART output mode, with a configurable distance cutoff for
room-only detection.

## Why this exists

The LD2420 datasheet describes a binary framed protocol (`FD FC FB FA`
headers, register reads/writes, per-gate sensitivity). Every existing
"LD2420 library" found at the time this was written either implemented
the *wrong* sensor's protocol (LD2410, not LD2420), or didn't implement
a working protocol at all. Testing against real hardware showed the
sensor actually streams simple, human-readable lines by default:

```
OFF
ON
Range 105
```

This library reads and parses that stream. **It does not implement the
binary configuration protocol** -- so it can't reconfigure the sensor's
own sensitivity or gates. What it does instead is apply a distance
cutoff in software, since the sensor has no side-to-side awareness
anyway -- "how far away" is the main signal available for filtering out
detections you don't want (through a wall, down a hallway, etc.).

## Installation

**PlatformIO** -- add directly from GitHub, no registry needed:

```ini
lib_deps =
    https://github.com/smrini/LD2420.git
```

**Arduino IDE** -- download this repo as a ZIP and use
*Sketch > Include Library > Add .ZIP Library*.

## Wiring

| LD2420 pin (silkscreen) | ESP32 |
|---|---|
| VCC | 3.3V |
| GND | GND |
| whichever of OT1/OT2 carries serial data on your firmware | GPIO -> `rxPin` |
| RX | GPIO <- `txPin` |
| the other of OT1/OT2 (a plain presence-level output) | leave unconnected, or read separately as a digital input -- **never** wire it to `txPin` |

⚠️ **OT1 and OT2 swap *jobs* depending on firmware version -- the pins
themselves are not physically mislabeled or swapped.** On older LD2420
firmware, OT1 is the presence-level output and OT2 carries serial data.
On newer firmware (confirmed on the unit this library was built
against), it's reversed: OT1 carries serial data and OT2 is the
presence-level output. The manufacturer's printed pin diagram describes
the older assignment; it isn't wrong about pin position, just about
which job each pin does on newer firmware. If you get garbled/no UART
data, try wiring `rxPin` to the *other* OT pin instead -- don't assume
your wiring is at fault before checking this.

**Whichever OT pin is *not* carrying serial data is still an output
driven by the module** (the presence-level signal) -- never wire your
ESP's `txPin` to it. Both `rxPin` and that pin are inputs into an
already-driven line, so wiring your ESP's TX there creates two outputs
(the module's and the ESP's) fighting over one wire. Wire `txPin` only
to the module's actual `RX` pin, which is unambiguous regardless of
firmware version.

Also confirm your **baud rate**. This library defaults to 115200,
matching the firmware it was tested against. Some LD2420 firmware
versions reportedly use 256000 instead. A telltale sign of the wrong
baud rate is a *stable, repeating* garbled byte pattern (as opposed to
random noise) -- that means real data is arriving, just being sampled
at the wrong rate.

## Usage

```cpp
#include <LD2420.h>

HardwareSerial LD2420Serial(1);
// rxPin/txPin here are placeholders -- see "Wiring" above for which
// physical module pin each one must actually go to on your firmware.
LD2420 sensor(LD2420Serial, /*rxPin=*/4, /*txPin=*/5);

void setup() {
    sensor.begin();
    sensor.setRoomMaxDistanceCm(200);   // tune to your space
    sensor.setRoomExitGraceMs(3000);    // optional: smooth flicker near the cutoff

    sensor.onRoomEnter([]() { /* ... */ });
    sensor.onRoomExit([]()  { /* ... */ });
}

void loop() {
    sensor.update(); // call every loop, non-blocking; callbacks fire from here
}
```

If you don't know your sensor's baud rate, call `sensor.detectBaud()`
instead of `sensor.begin()` in `setup()` -- it tries 115200 then 256000
and keeps whichever gets a response.

See `examples/BasicRead` for raw presence/distance without filtering,
`examples/RoomPresence` for the distance-cutoff + exit-grace pattern
above, and `examples/EventCallbacks` for the full callback API plus
baud auto-detection.

## API

| Method | Description |
|---|---|
| `LD2420(serial, rxPin, txPin, baud = 115200)` | Construct against any free `HardwareSerial`. |
| `void begin()` | Opens the UART at the constructor's baud. Call once in `setup()`. |
| `uint32_t detectBaud(timeoutMsPerBaud = 800)` | Alternative to `begin()`: tries 115200 then 256000, returns whichever worked (0 if neither did). Can't fix a wrong `rxPin`/`txPin` assignment, only the baud rate. |
| `void update()` | Non-blocking; call every `loop()`. Reads/parses available bytes and re-checks the exit grace period. |
| `bool isPresent() const` | True if the sensor currently reports a target, at any distance. |
| `int distanceCm() const` | Last reported distance in cm, or `-1` if not present. |
| `bool isInRoom() const` | True when present, within `roomMaxDistanceCm()`, or still inside the exit grace window. |
| `void setRoomMaxDistanceCm(int cm)` | Sets the distance cutoff used by `isInRoom()`. |
| `int roomMaxDistanceCm() const` | Reads the current cutoff (defaults to 200). |
| `void setRoomExitGraceMs(unsigned long ms)` | Keeps `isInRoom()` true for this long after the last in-range detection, to smooth flicker at the cutoff edge. `0` (default) disables it. |
| `unsigned long roomExitGraceMs() const` | Reads the current grace period. |
| `void onPresenceChange(cb)` | `void cb(bool presence, int distanceCm)` -- fires when raw presence flips. |
| `void onRoomEnter(cb)` / `onRoomExit(cb)` | `void cb()` -- fire on `isInRoom()` transitions (after cutoff/grace are applied). |

## Known limitations

- **No binary protocol support.** The sensor's own gate/sensitivity
  configuration (from the manufacturer datasheet) is not implemented.
  If you need per-gate energy thresholds or the sensor's native
  multi-zone output, this library isn't there yet -- contributions
  welcome.
- **Single-axis distance only.** There's no side-to-side or angular
  data, so "room-only" detection here means "within N cm," not true
  room-shape awareness. If your room isn't roughly a straight line in
  front of the sensor (e.g. an L-shaped space, or a doorway at a
  similar distance to the far wall), a plain distance cutoff won't
  fully solve it.
- **Through-wall detection.** 24GHz radar can detect motion through
  drywall. If your distance cutoff is farther than a wall behind which
  you don't want detection, you may still get false positives from the
  other side. Test this specifically for your mounting location.
- **Callbacks are plain function pointers**, not `std::function` --
  capturing lambdas won't compile against them (non-capturing lambdas
  and free functions work fine). Kept this way to avoid pulling in
  `<functional>` on memory-constrained targets.
- `detectBaud()` only helps with the baud rate, not with `rxPin`/`txPin`
  assignment -- see "Wiring" above for how to work out which physical
  pin goes where on your firmware.

## Testing

The line parser (`src/LD2420Parser.*`) has no Arduino dependency and is
unit tested on PlatformIO's `native` platform -- no hardware or serial
mocking required:

```
pio test -e native
```

The rest of the library (`LD2420.cpp`, the `HardwareSerial`-facing
class) is exercised via the examples building successfully for a real
target; see `.github/workflows/ci.yml`.

## License

MIT -- see [LICENSE](LICENSE).
