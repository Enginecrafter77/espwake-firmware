#pragma once
#include <stdint.h>

char hex2char(uint8_t hex);
void hex2str(char *out, const uint8_t *bytes, uint16_t digits);
