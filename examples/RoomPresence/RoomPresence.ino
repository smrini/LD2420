// Filters detections down to a single room by distance cutoff. Useful
// since the LD2420 has no side-to-side awareness -- distance along its
// sensing axis is the main lever available for "only sense my room"
// (e.g. rejecting detections through a wall or down a hallway).

#include <LD2420.h>

// Adjust these to match your wiring -- see README "Wiring": which OT
// pin carries serial data depends on firmware, and LD2420_TX_PIN must
// go to the module's actual RX pin, never to the other OT pin (that
// one's a module-driven output too).
constexpr int LD2420_RX_PIN = 4;  // ESP RX <- sensor's serial-data pin
constexpr int LD2420_TX_PIN = 5;  // ESP TX -> sensor's RX pin
constexpr int ROOM_MAX_DISTANCE_CM = 200;
constexpr unsigned long ROOM_EXIT_GRACE_MS = 3000; // holds "occupied" a bit after last detection

HardwareSerial LD2420Serial(1);
LD2420 sensor(LD2420Serial, LD2420_RX_PIN, LD2420_TX_PIN);

void setup() {
    Serial.begin(115200);
    sensor.begin();
    sensor.setRoomMaxDistanceCm(ROOM_MAX_DISTANCE_CM);
    sensor.setRoomExitGraceMs(ROOM_EXIT_GRACE_MS);
}

void loop() {
    sensor.update();

    static bool lastInRoom = false;
    if (sensor.isInRoom() != lastInRoom) {
        lastInRoom = sensor.isInRoom();
        if (lastInRoom) {
            Serial.println("Room occupied");
        } else if (sensor.isPresent()) {
            Serial.print("Detected beyond cutoff (");
            Serial.print(sensor.distanceCm());
            Serial.println("cm)");
        } else {
            Serial.println("Room empty");
        }
    }
}
