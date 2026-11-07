#pragma once
#include <Arduino.h>

// Row drawings, leftmost pixel = bit 4. Rendering maps these to column bytes.
const uint8_t Invader[2][7] PROGMEM = {
    {0b01010, 0b10001, 0b11111, 0b10101, 0b11111, 0b01010, 0b10001},
    {0b01010, 0b10001, 0b11111, 0b10101, 0b11111, 0b10001, 0b01010}
};
const uint8_t Ship[3] PROGMEM = {0b00100, 0b01110, 0b11111};
// Compact art: two 4-dot-high enemy rows fit above the 2-dot-high ship.
const uint8_t CompactInvader[2][4] PROGMEM = {
    {0b01010, 0b11111, 0b10101, 0b01010},
    {0b01010, 0b11111, 0b10101, 0b10001}
};
// Left-aligned within the same five-bit row encoding; visible width is three.
const uint8_t CompactShip[2] PROGMEM = {0b01000, 0b11100};
// Boss: eleven columns, seven rows; leftmost dot is bit 10.
const uint16_t BossArt[7] PROGMEM = {
    0b00100000100, 0b00010001000, 0b00111111100, 0b01101110110,
    0b11111111111, 0b10100000101, 0b00011011000
};
const uint8_t Explosion[5] PROGMEM = {0b10001, 0b01010, 0b00100, 0b01010, 0b10001};
