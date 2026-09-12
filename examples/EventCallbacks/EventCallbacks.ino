// Demonstrates the event-callback API, the room-exit grace period, and
// baud auto-detection -- no manual "compare against last state" logic
// needed in loop().

#include <LD2420.h>

// Adjust these to match your wiring -- see README "Wiring": which OT
// pin carries serial data depends on firmware, and LD2420_TX_PIN must
// go to the module's actual RX pin, never to the other OT pin (that
// one's a module-driven output too).
constexpr int LD2420_RX_PIN = 4;  // ESP RX <- sensor's serial-data pin
constexpr int LD2420_TX_PIN = 5;  // ESP TX -> sensor's RX pin
constexpr int ROOM_MAX_DISTANCE_CM = 200;
constexpr unsigned long ROOM_EXIT_GRACE_MS = 3000; // smooths flicker near the cutoff

HardwareSerial LD2420Serial(1);
LD2420 sensor(LD2420Serial, LD2420_RX_PIN, LD2420_TX_PIN);

void onPresence(bool present, int distanceCm) {
    if (present) {
        Serial.print("Presence: target at ");
        Serial.print(distanceCm);
        Serial.println("cm");
    } else {
        Serial.println("Presence: clear");
    }
}

void onRoomEnter() {
    Serial.println("Room: occupied");
}

void onRoomExit() {
    Serial.println("Room: empty");
}

void setup() {
    Serial.begin(115200);

    // Tries 115200 then 256000 and keeps whichever gets a valid reply.
    // Skip this and call sensor.begin() instead if you already know
    // your sensor's baud rate.
    uint32_t baud = sensor.detectBaud();
    if (baud == 0) {
        Serial.println("No response at either known baud rate -- check wiring.");
    } else {
        Serial.print("LD2420 responding at ");
        Serial.print(baud);
        Serial.println(" baud");
    }

    sensor.setRoomMaxDistanceCm(ROOM_MAX_DISTANCE_CM);
    sensor.setRoomExitGraceMs(ROOM_EXIT_GRACE_MS);

    sensor.onPresenceChange(onPresence);
    sensor.onRoomEnter(onRoomEnter);
    sensor.onRoomExit(onRoomExit);
}

void loop() {
    sensor.update(); // callbacks fire from inside here
}
