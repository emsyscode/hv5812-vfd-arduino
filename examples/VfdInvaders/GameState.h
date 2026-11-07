#pragma once
#include <stdint.h>

enum Phase : uint8_t { CALIBRATING, READY, PLAYING, PAUSED, WAVE_CLEAR, GAME_OVER, SPRITES, DYING };
struct Shot {
    int8_t x = 0, y = -1; // y=-1 is an unused slot.
};
struct GameState {
    Phase phase = CALIBRATING;
    int8_t playerX = 47, playerY = 11;
    int8_t formationX = 16, formationY = 0, direction = 1;
    uint8_t alive = 0xFF, lives = 3, wave = 1;
    uint16_t score = 0;
    Shot shots[3], bombs[2];
    int8_t blastX = 0, blastY = 0;
    uint32_t blastAt = 0, invulnerableAt = 0;
    bool blast = false, invulnerable = false;
    uint8_t secondRowAlive = 0;
    int8_t deathX = 0, deathY = 0;
    bool bossActive = false;
    uint8_t bossHp = 0, bossPattern = 0;
    int8_t bossX = 44, bossY = 0, bossDirection = 1;
};
