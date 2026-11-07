#pragma once
#include "Hv5812Transport.h"

// A panel pulse always returns with grid data cleared; BLANK stays LOW.
class Hv5812Panel {
public:
    Hv5812Panel() : grids_(Board::GridData, Board::GridClock),
        segments_(Board::SegmentData, Board::SegmentClock), latch_(Board::Latch) {}
    void begin() {
        digitalWrite(Board::Blank, HIGH);
        pinMode(Board::Blank, OUTPUT);
        latch_.begin();
        grids_.begin();
        segments_.begin();
        const uint8_t zero[5] = {};
        present(zero, zero);
        delay(Board::StartupBlankMs);
        digitalWrite(Board::Blank, LOW);
    }
    void enable(bool enabled) { enabled_ = enabled; clearGrids(); }
    bool isEnabled() const { return enabled_; }
    void setHoldUs(uint16_t value) { holdUs_ = value; }
    void setClockHalfPeriodUs(uint16_t value) {
        grids_.setClockHalfPeriodUs(value);
        segments_.setClockHalfPeriodUs(value);
    }
    uint32_t lastOnUs() const { return lastOnUs_; }
    void scanRaw(uint8_t slot, const uint8_t pixels[5]) {
        uint8_t select[5] = {};
        if (enabled_ && slot < 40) select[slot / 8] = uint8_t(1U << (slot % 8));
        present(select, pixels);
        const uint32_t started = micros();
        if (enabled_) {
            delayMicroseconds(holdUs_);
        }
        clearGrids(); // Return with no grid selected; D10 stays LOW.
        lastOnUs_ = enabled_ && slot < 40 ? micros() - started : 0;
    }
private:
    void clearGrids() {
        const uint8_t zero[5] = {};
        grids_.shift(zero);
        // A shared latch also relatches the unchanged segment shift register.
        latch();
        delayMicroseconds(3);
    }
    void present(const uint8_t grids[5], const uint8_t pixels[5]) {
        // The previous pulse has cleared all grids. Shift both complete words
        // before the common latch exposes the new grid and matching segments.
        grids_.shift(grids);
        segments_.shift(pixels);
        latch();
        delayMicroseconds(3);
    }
    void latch() {
        delayMicroseconds(1);
        latch_.write(true);
        delayMicroseconds(1);
        latch_.write(false);
    }
    Hv5812Chain grids_, segments_;
    Hv5812Pin latch_;
    bool enabled_ = false;
    uint16_t holdUs_ = Board::GridOnTimeUs;
    uint32_t lastOnUs_ = 0;
};
