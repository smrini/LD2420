#pragma once

// Pure parsing logic for one line of the LD2420's plain-text protocol.
// Deliberately has zero Arduino dependency, so it can be unit tested on
// the native platform (see test/test_parser) without mocking any serial
// hardware.

enum class LD2420LineType {
    Unknown,
    PresenceOn,
    PresenceOff,
    Range,
};

struct LD2420ParsedLine {
    LD2420LineType type = LD2420LineType::Unknown;
    int distanceCm = -1; // valid only when type == Range
};

// Parses a single line (a trailing \r, if present, is trimmed
// internally -- pass lines with or without it). Returns
// LD2420LineType::Unknown for anything unrecognized, including null or
// empty input.
LD2420ParsedLine ld2420ParseLine(const char *rawLine);
