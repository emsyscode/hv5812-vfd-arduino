#include "VfdDisplay.h"
#include <stdlib.h>
#include <string.h>

VfdDisplay display;
char input[48];
uint8_t used = 0;
bool overflow = false;

void printHelp() {
    Serial.println(F("row1 <text> | row2 <text> | hold 100..10000 | scan active/all | clear | ?"));
}

void submit() {
    input[used] = 0;
    if (overflow) Serial.println(F("ERR line too long"));
    else if (!strcmp(input, "?")) printHelp();
    else if (!strcmp(input, "clear")) display.clear();
    else if (!strcmp(input, "scan active")) display.setActiveScan(true);
    else if (!strcmp(input, "scan all")) display.setActiveScan(false);
    else if (!strncmp(input, "hold ", 5)) {
        char *end;
        const unsigned long value = strtoul(input + 5, &end, 10);
        bool digits = input[5] != 0;
        for (char *p = input + 5; *p; ++p) if (*p < '0' || *p > '9') digits = false;
        if (digits && !*end && value >= 100 && value <= 10000) display.setHoldUs(value);
        else Serial.println(F("ERR hold 100..10000"));
    } else if (!strncmp(input, "row1 ", 5) || !strncmp(input, "row2 ", 5)) {
        bool valid = strlen(input + 5) <= 20;
        for (char *p = input + 5; *p; ++p) if (*p < 32 || *p > 126) valid = false;
        if (valid) {
            const uint8_t row = input[3] - '1';
            display.clearRow(row);
            display.write(row, 0, input + 5);
        } else Serial.println(F("ERR up to 20 printable ASCII characters"));
    } else if (used) Serial.println(F("ERR unknown command"));
    used = 0;
    overflow = false;
    display.resetScan();
}

void setup() {
    display.begin();
    display.write(0, 0, "HELLO VFD");
    display.write(1, 0, "HV5812 / UCN5812");
    display.resetScan();
    display.enable(true);
    Serial.begin(115200);
    printHelp();
}

void loop() {
    while (Serial.available()) {
        const char c = Serial.read();
        if (c == '\r' || c == '\n') { if (used || overflow) submit(); }
        else if (used < sizeof(input) - 1 && !overflow) input[used++] = c;
        else overflow = true;
    }
    const bool complete = display.refresh(); // One selected grid per call.
    if (complete && display.scanPositionCount() <= 1) delayMicroseconds(5000);
}
