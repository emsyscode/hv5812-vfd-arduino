#include "Hv5812Panel.h"

Hv5812Panel panel;
const uint8_t letterA[5] = {0x7E, 0x90, 0x90, 0x90, 0x7E};

void setup() {
    panel.begin();
    panel.enable(true);
}

void loop() {
    panel.scanRaw(Board::GridSlot[0][0], letterA);
    delayMicroseconds(5000); // Dark interval limits single-cell duty cycle.
}
