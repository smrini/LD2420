// Minimal example: prints presence and distance as they change.
// No room filtering -- see the RoomPresence example for that.

#include <LD2420.h>

// Adjust these to match your wiring -- see README "Wiring" first: which
// physical module pin carries serial data (rx here) depends on firmware
// version, and LD2420_TX_PIN must go to the module's actual RX pin, not
// to whichever OT pin isn't carrying serial data (that one's an output
// too, and driving it as TX would fight the module's own signal).
constexpr int LD2420_RX_PIN = 4;  // ESP RX <- sensor's serial-data pin
constexpr int LD2420_TX_PIN = 5;  // ESP TX -> sensor's RX pin

HardwareSerial LD2420Serial(1);
LD2420 sensor(LD2420Serial, LD2420_RX_PIN, LD2420_TX_PIN);

void setup() {
    Serial.begin(115200);
    sensor.begin();
}

void loop() {
    sensor.update();

    static bool lastPresence = false;
    if (sensor.isPresent() != lastPresence) {
        lastPresence = sensor.isPresent();
        if (lastPresence) {
            Serial.print("Detected at ");
            Serial.print(sensor.distanceCm());
            Serial.println("cm");
        } else {
            Serial.println("Clear");
        }
    }
}
