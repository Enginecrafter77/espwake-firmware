#include "util.h"

char hex2char(uint8_t hex) {
    if (hex < 10) {
        return hex + '0';
    } else if (hex < 16) {
        return (hex - 10) + 'A';
    } else {
        return 'X';
    }
}

void hex2str(char *out, const uint8_t *bytes, uint16_t digits) {
    for (uint16_t i = 0; i < digits; ++i) {
        uint8_t byte = bytes[i >> 1];
        if ((i & 0x1) == 0) {
            byte >>= 4;
        }
        out[i] = hex2char(byte & 0xF);
    }
    out[digits] = 0;
}
