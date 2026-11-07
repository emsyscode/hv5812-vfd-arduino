#ifndef VFD_DISPLAY_H
#define VFD_DISPLAY_H
#include "Hv5812Panel.h"
#include "VfdAscii.h"

class VfdDisplay : public Hv5812Panel {
public:
    void begin() { Hv5812Panel::begin(); clear(); }
    void setActiveScan(bool activeOnly) { activeOnly_ = activeOnly; }
    uint8_t scanPositionCount() const {
        if (!activeOnly_) return 40;
        if (countDirty_) {
            activeCount_ = 0;
            for (uint8_t r=0; r<2; ++r) for (uint8_t c=0; c<20; ++c)
                if (text_[r][c]!=' ' || extras_[r][c]) ++activeCount_;
            countDirty_ = false;
        }
        return activeCount_;
    }
    void resetScan() { row_ = 0; col_ = 0; resetFrameCount(); }
    void clear() {
        clearRow(0); clearRow(1);
    }
    void clearRow(uint8_t row) {
        if (row >= 2) return;
        countDirty_ = true;
        for (uint8_t col = 0; col < 20; ++col) {
            text_[row][col] = ' ';
            extras_[row][col] = 0;
        }
    }
    void write(uint8_t row, uint8_t col, const char *text) {
        if (row >= 2 || col >= 20 || !text) return;
        countDirty_ = true;
        while (*text && col < 20) text_[row][col++] = uint8_t(*text++);
    }
    // Raw mask: bits 0..4 control bit 0 of each transmitted column byte.
    // Identify the dot/colon/triangle mapping on the actual panel first.
    void setExtras(uint8_t row, uint8_t col, uint8_t mask) {
        if (row < 2 && col < 20) {
            extras_[row][col] = mask & 0x1F;
            countDirty_ = true;
        }
    }
    // Skip display time for empty cells, never bits in the 40-bit words.
    bool refresh() {
        if (activeOnly_) {
            if (!scanPositionCount()) {
                const uint8_t zero[5]={};
                scanRaw(40,zero);
                return true;
            }
            for (uint8_t i=0;i<40 && text_[row_][col_]==' ' && !extras_[row_][col_];++i) advance();
        }
        uint8_t pixels[5];
        for (uint8_t i = 0; i < 5; ++i)
            pixels[i] = ascii_5x7_column(text_[row_][col_], i) |
                ((extras_[row_][col_] >> i) & 1);
        scanRaw(Board::GridSlot[row_][col_], pixels);
        advance();
        if (--remaining_ == 0) { resetFrameCount(); return true; }
        return false;
    }
private:
    void advance() { if (++col_==20) { col_=0; row_^=1; } }
    void resetFrameCount() {
        remaining_=scanPositionCount();
        if (!remaining_) remaining_=1;
    }
    // Recount only after buffer changes, not on every refresh.
    mutable bool countDirty_ = true;
    mutable uint8_t activeCount_ = 0;
    uint8_t text_[2][20] = {};
    uint8_t extras_[2][20] = {};
    uint8_t row_ = 0, col_ = 0;
    uint8_t remaining_ = 40;
    bool activeOnly_ = true;
};
#endif
