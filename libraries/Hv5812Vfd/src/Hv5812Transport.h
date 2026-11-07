#ifndef HV5812_CHAIN_H
#define HV5812_CHAIN_H
#include <Arduino.h>
#include "Hv5812Board.h"

// Cache AVR port addresses so multiplex refresh does not pay digitalWrite's
// pin lookup overhead on every clock edge. Other boards use the Arduino API.
class Hv5812Pin {
public:
    explicit Hv5812Pin(uint8_t pin) : pin_(pin) {}
    void begin() {
        digitalWrite(pin_, LOW);
        pinMode(pin_, OUTPUT);
#ifdef __AVR__
        port_ = portOutputRegister(digitalPinToPort(pin_));
        mask_ = digitalPinToBitMask(pin_);
#endif
    }
    __attribute__((always_inline)) inline void write(bool high) const {
#ifdef __AVR__
        if (high) *port_ |= mask_;
        else *port_ &= uint8_t(~mask_);
#else
        digitalWrite(pin_, high ? HIGH : LOW);
#endif
    }
private:
    uint8_t pin_;
#ifdef __AVR__
    volatile uint8_t *port_ = nullptr;
    uint8_t mask_ = 0;
#endif
};

// Serial slots describe transmission order, not IC output pin numbers.
class Hv5812Chain {
public:
    Hv5812Chain(uint8_t data, uint8_t clock)
        : data_(data), clock_(clock) {}
    void begin() {
        data_.begin();
        clock_.begin();
    }
    void setClockHalfPeriodUs(uint16_t value) { halfPeriodUs_ = value; }
    void shift(const uint8_t bytes[5]) const {
        const uint16_t clockUs = halfPeriodUs_;
        for (uint8_t i = 0; i < 5; ++i) {
            uint8_t value = bytes[i];
            for (uint8_t bit = 0; bit < 8; ++bit) {
                clock_.write(false);
                data_.write((value & 1) != 0);
                delayMicroseconds(clockUs);
                clock_.write(true);
                delayMicroseconds(clockUs);
                value >>= 1;
            }
        }
        clock_.write(false);
    }
private:
    Hv5812Pin data_, clock_;
    uint16_t halfPeriodUs_ = Board::ClockHalfPeriodUs;
};
#endif
