#include <Arduino.h>

constexpr uint8_t XPin = A0, YPin = A2, FirePin = 12;
uint32_t lastReport = 0;

void setup() {
    pinMode(XPin, INPUT);
    pinMode(YPin, INPUT);
    pinMode(FirePin, INPUT_PULLUP);
    Serial.begin(115200);
    Serial.println(F("Move each axis and press the button. X/Y range: 0..1023."));
}

void loop() {
    const uint32_t now = millis();
    if (uint32_t(now - lastReport) < 100) return;
    lastReport = now;

    // Discard the first conversion after switching ADC channels.
    analogRead(XPin);
    const int x = analogRead(XPin);
    analogRead(YPin);
    const int y = analogRead(YPin);

    Serial.print(F("X=")); Serial.print(x);
    Serial.print(F(" Y=")); Serial.print(y);
    Serial.print(F(" FIRE="));
    Serial.println(digitalRead(FirePin) == LOW ? F("DOWN") : F("UP"));
}
