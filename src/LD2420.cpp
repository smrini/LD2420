#include "LD2420.h"

LD2420::LD2420(HardwareSerial &serial, int rxPin, int txPin, uint32_t baud)
    : _serial(serial), _rxPin(rxPin), _txPin(txPin), _baud(baud) {}

void LD2420::begin() {
    _serial.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    _lineLength = 0;
}

uint32_t LD2420::detectBaud(uint32_t timeoutMsPerBaud) {
    const uint32_t candidates[] = {115200, 256000};

    for (uint32_t candidate : candidates) {
        _serial.begin(candidate, SERIAL_8N1, _rxPin, _txPin);
        delay(50); // let the UART settle after reconfiguring

        String sample;
        sample.reserve(128);
        unsigned long start = millis();
        while (millis() - start < timeoutMsPerBaud) {
            while (_serial.available()) {
                sample += (char)_serial.read();
                if (sample.length() > 256) {
                    sample.remove(0, 128); // keep the buffer bounded
                }
            }
        }

        // A wrong baud rate produces either noise or a stable-but-wrong
        // repeating byte pattern -- either way, these exact ASCII
        // sequences are very unlikely to appear by chance.
        if (sample.indexOf("OFF\r\n") >= 0 || sample.indexOf("ON\r\n") >= 0) {
            _baud = candidate;
            _lineLength = 0;
            return candidate;
        }
    }

    // Nothing recognizable at either candidate baud -- fall back to the
    // baud passed to the constructor so update() still has a UART open
    // to debug against. This won't fix a wrong rxPin/txPin assignment.
    _serial.begin(_baud, SERIAL_8N1, _rxPin, _txPin);
    _lineLength = 0;
    return 0;
}

void LD2420::update() {
    while (_serial.available()) {
        feedByte((char)_serial.read());
    }
    // Re-check even without new data, so the exit grace period expires
    // on its own instead of waiting for the next line to arrive.
    evaluateRoomState();
}

void LD2420::feedByte(char c) {
    if (c == '\n') {
        _lineBuffer[_lineLength] = '\0';
        handleParsedLine(ld2420ParseLine(_lineBuffer));
        _lineLength = 0;
    } else if (c != '\r') {
        if (_lineLength < sizeof(_lineBuffer) - 1) {
            _lineBuffer[_lineLength++] = c;
        }
        // else: line longer than the buffer -- drop the overflow, the
        // real protocol's lines are always short.
    }
}

void LD2420::handleParsedLine(const LD2420ParsedLine &parsed) {
    switch (parsed.type) {
        case LD2420LineType::PresenceOn:
            // Wait for the Range line that follows before firing the
            // callback, so it reports the actual distance rather than a
            // stale one from a previous detection.
            _pendingPresenceOn = true;
            break;

        case LD2420LineType::PresenceOff:
            _pendingPresenceOn = false;
            _distance = -1;
            if (_presence) {
                _presence = false;
                if (_presenceCallback) _presenceCallback(false, -1);
            }
            evaluateRoomState();
            break;

        case LD2420LineType::Range:
            _distance = parsed.distanceCm;
            if (_pendingPresenceOn && !_presence) {
                _presence = true;
                if (_presenceCallback) _presenceCallback(true, _distance);
            }
            _pendingPresenceOn = false;
            evaluateRoomState();
            break;

        case LD2420LineType::Unknown:
        default:
            break;
    }
}

void LD2420::evaluateRoomState() {
    bool rawInRoom = _presence && _distance >= 0 && _distance <= _roomMaxDistanceCm;
    unsigned long now = millis();

    if (rawInRoom) {
        _lastRawInRoomAt = now;
        _everInRoom = true;
    }

    bool newInRoom = rawInRoom ||
        (_everInRoom && _roomExitGraceMs > 0 && (now - _lastRawInRoomAt) <= _roomExitGraceMs);

    if (newInRoom != _inRoom) {
        _inRoom = newInRoom;
        if (_inRoom) {
            if (_roomEnterCallback) _roomEnterCallback();
        } else {
            if (_roomExitCallback) _roomExitCallback();
        }
    }
}
