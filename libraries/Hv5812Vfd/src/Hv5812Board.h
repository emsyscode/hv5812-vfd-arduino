#ifndef VFD_BOARD_CONFIG_H
#define VFD_BOARD_CONFIG_H
#include <stdint.h>
namespace Board {
constexpr uint8_t GridData = 7;
constexpr uint8_t GridClock = 8;
constexpr uint8_t Latch = 9;
constexpr uint8_t SegmentData = 4;
constexpr uint8_t SegmentClock = 5;
// Both two-driver chains share D9 STROBE. D6 is unused.
constexpr uint8_t Blank = 10;
// D10 is HIGH during startup, then stays LOW for the rest of the run.
constexpr uint16_t StartupBlankMs = 50;
constexpr uint16_t ClockHalfPeriodUs = 1;
constexpr uint16_t GridOnTimeUs = 150; // Default grid hold; configurable at runtime.
// Logical row/column to serial slot: top row first, left to right.
constexpr uint8_t GridSlot[2][20] = {
    {39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20},
    {19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0}
};
}
#endif
