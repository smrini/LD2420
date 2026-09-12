#include "LD2420Parser.h"

#include <cstdlib>
#include <cstring>

namespace {

void trimTrailing(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\r' || s[len - 1] == '\n' || s[len - 1] == ' ')) {
        s[--len] = '\0';
    }
}

} // namespace

LD2420ParsedLine ld2420ParseLine(const char *rawLine) {
    LD2420ParsedLine result;
    if (rawLine == nullptr) {
        return result;
    }

    char buf[64];
    strncpy(buf, rawLine, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trimTrailing(buf);

    if (buf[0] == '\0') {
        return result;
    }

    if (strcmp(buf, "ON") == 0) {
        result.type = LD2420LineType::PresenceOn;
    } else if (strcmp(buf, "OFF") == 0) {
        result.type = LD2420LineType::PresenceOff;
    } else if (strncmp(buf, "Range ", 6) == 0) {
        result.type = LD2420LineType::Range;
        result.distanceCm = atoi(buf + 6);
    }

    return result;
}
