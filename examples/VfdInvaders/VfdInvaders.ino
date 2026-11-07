// Space Invaders-style game; see README.md for controls.
#include "Hv5812Panel.h"
#include "VfdAscii.h"
#include "GameState.h"
#include "Sprites.h"
#include <string.h>

constexpr uint8_t XPin = A0, FirePin = 12, BuzzerPin = 11;
// Increasing X voltage moves left; reversex changes this direction.
bool reverseX = false;
constexpr int Engage = 30, Release = 18;
constexpr uint16_t ShotStepMs = 45, FireRepeatMs = 260;
bool compactSprites = true;
bool growingRows = false;
bool staggeredRows = true, bossesEnabled = true;
int8_t bombDx[2] = {};
uint32_t bossStageAt = 0;
uint8_t startingLives = 3;
uint8_t menuPage = 0; // 0 closed, 1 main, 2 difficulty, 3 size, 4 levels, 5 formation, 6 boss.

Hv5812Panel panel;
GameState game;
uint16_t holdUs = Board::GridOnTimeUs;
bool activeScan = true, demoMode = false, soundEnabled = true;
uint8_t frame[40][5] = {}, activeCells[40] = {}, activeCount = 0, scanIndex = 0;
bool frameDirty = true;
bool singleCellWaiting = false;
uint32_t lastDraw = 0, lastInput = 0, lastXMove = 0;
uint32_t phaseAt = 0, lastEnemyMove = 0, lastShotMove = 0, lastBomb = 0, lastFire = 0;
uint32_t lastScan = 0, scanPeriodUs = 0, singleCellOffUntil = 0;
uint32_t calibrationX = 0;
uint16_t calibrationSamples = 0;
int rawX = 0, centreX = 512, filteredX = 512;
int8_t stickX = 0;
bool fireRaw = HIGH, fireStable = HIGH, fireArmed = false;
uint32_t fireChangedAt = 0, buzzerAt = 0;
uint16_t buzzerMs = 0;
char command[32] = {};
uint8_t commandLength = 0;
bool commandOverflow = false;
uint32_t lastByteAt = 0;

void setup() {
    digitalWrite(BuzzerPin, LOW);
    pinMode(BuzzerPin, OUTPUT);
    pinMode(FirePin, INPUT_PULLUP);
    pinMode(XPin, INPUT);
    panel.begin();
    panel.setHoldUs(holdUs);
    panel.enable(true);
    Serial.begin(115200);
    Serial.println(F("VFD INVADERS | compact sprites | A0=left/right | hold=150us"));
    printHelp();
    beginCalibration();
    printMenu();
}

void loop() {
    readSerial();
    updateInputs(millis());
    const uint32_t now = millis();
    updateBuzzer(now);
    updateGame(now);
    // Rebuild only between sweeps: one displayed frame never mixes two canvases.
    if (!scanIndex && frameDirty) renderFrame(now);
    if (singleCellWaiting && int32_t(micros() - singleCellOffUntil) >= 0) singleCellWaiting = false;
    if (!singleCellWaiting) scanDisplay();
}
