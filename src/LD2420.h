#pragma once

#include <Arduino.h>

#include "LD2420Parser.h"

// Driver for the HLK-LD2420 24GHz presence sensor's plain-text UART
// output mode:
//   "OFF\r\n"                 -- nothing detected
//   "ON\r\nRange <n>\r\n"     -- target detected at n cm
//
// Confirmed against real hardware at 115200 baud. This does NOT
// implement the sensor's binary configuration protocol (the
// register/gate setup described in the manufacturer datasheet) -- it
// only reads the plain-text status lines the sensor streams by default.
// See the README for what that means for you.
//
// "Room-only" detection is done in software: isInRoom() is true when
// the reported distance is within roomMaxDistanceCm() (optionally held
// true a bit longer via setRoomExitGraceMs() to smooth out flicker from
// a target hovering near the cutoff). The sensor has no side-to-side
// awareness, so a distance cutoff along its single sensing axis is the
// only filtering lever available here.
class LD2420 {
public:
    using PresenceCallback = void (*)(bool presence, int distanceCm);
    using RoomCallback = void (*)();

    // serial: any free HardwareSerial instance.
    // rxPin/txPin: GPIO wired to the sensor's TX/RX lines respectively.
    //   NOTE: which physical module pin (OT1 or OT2) carries serial data
    //   depends on firmware version -- the other one is a plain
    //   presence-level output, not an input. Never wire txPin to that
    //   one; it's driven by the module and will conflict with the ESP's
    //   own TX drive. Wire txPin only to the module's actual RX pin (see
    //   README "Wiring").
    // baud: used as-is by begin(), and as the first candidate tried by
    //   detectBaud().
    LD2420(HardwareSerial &serial, int rxPin, int txPin, uint32_t baud = 115200);

    // Opens the UART at the constructor's baud rate. Use this when you
    // already know your sensor's baud rate.
    void begin();

    // Alternative to begin(): tries 115200 then 256000 (the two known
    // LD2420 firmware bauds), waiting up to timeoutMsPerBaud for a
    // recognizable line at each, and keeps whichever one works. Returns
    // the baud rate that worked, or 0 if neither did (in which case the
    // UART is left open at the constructor's original baud so update()
    // still has something to read from). This cannot fix a wrong
    // rxPin/txPin assignment -- only the baud rate.
    uint32_t detectBaud(uint32_t timeoutMsPerBaud = 800);

    // Call every loop(). Non-blocking: reads whatever bytes are
    // currently available, parses complete lines, and re-checks the
    // exit grace period even when no new line has arrived.
    void update();

    bool isPresent() const { return _presence; }
    int distanceCm() const { return _distance; } // -1 when not present

    bool isInRoom() const { return _inRoom; }
    void setRoomMaxDistanceCm(int cm) { _roomMaxDistanceCm = cm; }
    int roomMaxDistanceCm() const { return _roomMaxDistanceCm; }

    // How long isInRoom() keeps reporting true after the last in-range
    // detection. Smooths out flicker for a target hovering right at the
    // cutoff distance. 0 (the default) disables the grace period --
    // isInRoom() then tracks the raw reading exactly.
    void setRoomExitGraceMs(unsigned long ms) { _roomExitGraceMs = ms; }
    unsigned long roomExitGraceMs() const { return _roomExitGraceMs; }

    // Fired from update() on real transitions -- no need to hand-roll a
    // "compare against last state" check in your own loop().
    // onPresenceChange fires when the raw sensor presence flips, with
    // the just-updated distance (-1 on the OFF transition).
    // onRoomEnter/onRoomExit fire on isInRoom() transitions, i.e. after
    // the distance cutoff (and exit grace, if set) are applied.
    void onPresenceChange(PresenceCallback cb) { _presenceCallback = cb; }
    void onRoomEnter(RoomCallback cb) { _roomEnterCallback = cb; }
    void onRoomExit(RoomCallback cb) { _roomExitCallback = cb; }

private:
    HardwareSerial &_serial;
    int _rxPin;
    int _txPin;
    uint32_t _baud;

    char _lineBuffer[64];
    uint8_t _lineLength = 0;

    bool _presence = false;
    int _distance = -1;
    bool _pendingPresenceOn = false; // seen "ON", waiting on the Range line that follows

    int _roomMaxDistanceCm = 200;
    unsigned long _roomExitGraceMs = 0;
    unsigned long _lastRawInRoomAt = 0;
    bool _everInRoom = false;
    bool _inRoom = false;

    PresenceCallback _presenceCallback = nullptr;
    RoomCallback _roomEnterCallback = nullptr;
    RoomCallback _roomExitCallback = nullptr;

    void feedByte(char c);
    void handleParsedLine(const LD2420ParsedLine &parsed);
    void evaluateRoomState();
};
