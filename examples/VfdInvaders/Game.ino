uint8_t enemyHeight() { return compactSprites ? 4 : 7; }
uint8_t shipHeight() { return compactSprites ? 2 : 3; }
uint8_t shipWidth() { return compactSprites ? 3 : 5; }
const uint8_t *enemyArt(uint8_t pose) { return compactSprites ? CompactInvader[pose] : Invader[pose]; }
const uint8_t *shipArt() { return compactSprites ? CompactShip : Ship; }
bool enemyAlive(uint8_t enemy) {
    return enemy < 8 ? (game.alive & (1U << enemy)) : (game.secondRowAlive & (1U << (enemy - 8)));
}
int enemyX(uint8_t enemy) { return game.formationX + (enemy % 8) * 9 + (staggeredRows && enemy>=8 ? 4 : 0); }
int enemyY(uint8_t enemy) { return game.formationY + (enemy / 8) * (enemyHeight() + 1); }

void setPhase(Phase phase, uint32_t now) {
    game.phase = phase;
    phaseAt = now;
    frameDirty = true;
}

void clearShots() {
    for (uint8_t i = 0; i < 3; ++i) game.shots[i].y = -1;
    for (uint8_t i = 0; i < 2; ++i) game.bombs[i].y = -1;
    bombDx[0]=bombDx[1]=0;
}

void beginWave(uint32_t now) {
    game.bossActive=false;
    game.alive = 0xFF;
    game.secondRowAlive = growingRows && game.wave >= 2 ? 0xFF : 0;
    game.formationX = 16;
    game.formationY = 0;
    game.direction = 1;
    clearShots();
    lastEnemyMove = lastShotMove = lastBomb = now;
    setPhase(PLAYING, now);
    Serial.print(F("LEVEL ")); Serial.print(game.wave);
    Serial.print(F(" rows=")); Serial.println(game.secondRowAlive?2:1);
}

void startGame(bool automatic) {
    if (game.phase == CALIBRATING) { Serial.println(F("Wait for READY; joystick is calibrating.")); return; }
    demoMode = automatic;
    menuPage = 0;
    game = GameState();
    game.lives = startingLives;
    game.playerY = 14 - shipHeight();
    const uint32_t now = millis();
    lastFire = now - FireRepeatMs;
    beginWave(now);
    beep(60);
    Serial.println(automatic ? F("DEMO PLAY") : F("PLAY"));
}

uint8_t enemyCount() {
    if(game.bossActive) return game.bossHp?1:0;
    uint8_t count = 0;
    for (uint8_t i = 0; i < 16; ++i) if (enemyAlive(i)) ++count;
    return count;
}

void movePlayer(int8_t dx) {
    if (game.phase != PLAYING) return;
    const int x = game.playerX + dx;
    game.playerX = constrain(x, 0, 100 - shipWidth());
    game.playerY = 14 - shipHeight(); // Horizontal-only control in both sizes.
    frameDirty = true;
}

void fireShot(uint32_t now) {
    if (game.phase != PLAYING || uint32_t(now - lastFire) < FireRepeatMs) return;
    for (uint8_t i = 0; i < 3; ++i) if (game.shots[i].y < 0) {
        game.shots[i].x = game.playerX + shipWidth() / 2;
        game.shots[i].y = game.playerY - 1;
        lastFire = now;
        frameDirty = true;
        beep(12);
        return;
    }
}

bool enemyPixel(uint8_t enemy, int8_t x, int8_t y) {
    if (!enemyAlive(enemy)) return false;
    const int dx = x - enemyX(enemy);
    const int dy = y - enemyY(enemy);
    if (dx < 0 || dx >= 5 || dy < 0 || dy >= enemyHeight()) return false;
    // Both animation frames share a collision outline for the moving legs.
    const uint8_t row = pgm_read_byte(enemyArt(0) + dy) | pgm_read_byte(enemyArt(1) + dy);
    return row & (0x10 >> dx);
}

void endGame(uint32_t now) {
    clearShots();
    game.deathX = game.playerX;
    game.deathY = game.playerY;
    setPhase(DYING, now);
    beep(250);
    Serial.print(F("GAME OVER score=")); Serial.println(game.score);
    Serial.println(F("Release and press FIRE for a new round."));
}

void hurtPlayer(uint32_t now) {
    if (game.invulnerable) return;
    --game.lives;
    game.invulnerable = true;
    game.invulnerableAt = now;
    clearShots();
    game.playerY = 14 - shipHeight();
    beep(120);
    frameDirty = true;
    Serial.print(F("HIT lives=")); Serial.println(game.lives);
    if (!game.lives) endGame(now);
}

void checkCollisions(uint32_t now) {
    for (uint8_t i = 0; i < 3; ++i) {
        Shot &shot = game.shots[i];
        if (shot.y < 0) continue;
        if(game.bossActive) {
            hitBoss(shot,now);
            continue;
        }
        for (uint8_t e = 0; e < 16; ++e) {
            if (!enemyPixel(e, shot.x, shot.y) && !enemyPixel(e, shot.x, shot.y + 1)) continue;
            if (e < 8) game.alive &= uint8_t(~(1U << e));
            else game.secondRowAlive &= uint8_t(~(1U << (e - 8)));
            game.blastX = enemyX(e);
            game.blastY = enemyY(e);
            game.blast = true; game.blastAt = now;
            game.score = game.score <= 65525 ? game.score + 10 : 65535;
            shot.y = -1;
            frameDirty = true;
            beep(35);
            break;
        }
    }
    for (uint8_t i = 0; i < 2; ++i) {
        const Shot &bomb = game.bombs[i];
        if (bomb.y < 0) continue;
        for (uint8_t dy = 0; dy < 2; ++dy) {
            const int x = bomb.x - game.playerX, y = bomb.y + dy - game.playerY;
            if (x >= 0 && x < shipWidth() && y >= 0 && y < shipHeight() &&
                (pgm_read_byte(shipArt() + y) & (0x10 >> x))) { hurtPlayer(now); break; }
        }
    }
    if (game.phase == PLAYING && (game.bossActive ? !game.bossHp : !game.alive && !game.secondRowAlive)) {
        clearShots();
        setPhase(WAVE_CLEAR, now);
        beep(180);
        Serial.print(F("WAVE CLEAR score=")); Serial.println(game.score);
    }
}

void moveEnemies(uint32_t now) {
    if(game.bossActive) {moveBoss(now);return;}
    const uint8_t killed = (growingRows && game.wave>=2 ? 16 : 8) - enemyCount();
    const uint16_t difficulty = uint16_t(game.wave - 1) * 20 + killed * 25;
    const uint16_t interval = difficulty < 360 ? 450 - difficulty : 90;
    if (uint32_t(now - lastEnemyMove) < interval) return;
    lastEnemyMove = now;
    int left=100,right=-1;
    uint8_t bottom = 0;
    for (uint8_t e = 0; e < 16; ++e) if (enemyAlive(e)) {
        left=min(left,enemyX(e));right=max(right,enemyX(e)+4);
        if (enemyY(e) + enemyHeight() > bottom) bottom = enemyY(e) + enemyHeight();
    }
    const int nextLeft = left + game.direction;
    const int nextRight = right + game.direction;
    if (nextLeft < 0 || nextRight >= 100) {
        game.direction = -game.direction;
        ++game.formationY;
        if (bottom + 1 >= game.playerY + 1) endGame(now); // Breaching the defence line loses the round.
    } else game.formationX += game.direction;
    frameDirty = true;
}

void dropBomb(uint32_t now) {
    if(game.bossActive) {bossAttack(now);return;}
    const uint16_t interval = game.wave < 10 ? 1100 - game.wave * 60 : 500;
    if (uint32_t(now - lastBomb) < interval) return;
    lastBomb = now;
    uint8_t enemy = random(16);
    for (uint8_t i = 0; i < 16; ++i, enemy = (enemy + 1) % 16) {
        if (!enemyAlive(enemy)) continue;
        // With staggered rows, only a lower sprite under this launch column blocks it.
        bool blocked=false;
        if(enemy<8) for(uint8_t lower=8;lower<16;++lower)
            if(enemyAlive(lower) && enemyX(enemy)+2>=enemyX(lower) && enemyX(enemy)+2<enemyX(lower)+5) blocked=true;
        if(blocked) continue;
        for (uint8_t b = 0; b < 2; ++b) if (game.bombs[b].y < 0) {
            game.bombs[b].x = enemyX(enemy) + 2;
            game.bombs[b].y = enemyY(enemy) + enemyHeight();
            bombDx[b]=0;
            frameDirty = true;
            return;
        }
        return;
    }
}

void updateGame(uint32_t now) {
    if (uint32_t(now - lastDraw) >= 100) { lastDraw = now; frameDirty = true; }
    if (game.phase == DYING && uint32_t(now - phaseAt) >= 600) setPhase(GAME_OVER, now);
    if (game.phase == WAVE_CLEAR && uint32_t(now - phaseAt) >= 1000) {
        if(bossesEnabled && game.wave%3==0 && !game.bossActive) beginBoss(now);
        else {
            if (game.wave < 255) ++game.wave;
            beginWave(now);
        }
    }
    if (game.phase == GAME_OVER && demoMode && uint32_t(now - phaseAt) >= 2000) startGame(true);
    if (game.phase != PLAYING) return;
    if (game.invulnerable && uint32_t(now - game.invulnerableAt) >= 1200) game.invulnerable = false;
    if (game.blast && uint32_t(now - game.blastAt) >= 120) { game.blast = false; frameDirty = true; }
    if (demoMode) {
        uint8_t target = game.secondRowAlive ? 8 : 0;
        while (target < 15 && !enemyAlive(target)) ++target;
        const int desired = game.bossActive ? game.bossX+5-shipWidth()/2 : enemyX(target) + 2 - shipWidth() / 2 + game.direction;
        if (uint32_t(now - lastXMove) >= 12) {
            lastXMove = now;
            movePlayer(desired > game.playerX ? 1 : desired < game.playerX ? -1 : 0);
        }
        fireShot(now);
    }
    moveEnemies(now);
    if (game.phase != PLAYING) return;
    // Check before and after bullet steps so a shot meeting a moving enemy is not skipped.
    checkCollisions(now);
    if (game.phase != PLAYING) return;
    if (uint32_t(now - lastShotMove) >= ShotStepMs) {
        lastShotMove = now;
        for (uint8_t i = 0; i < 3; ++i) if (game.shots[i].y >= 0) --game.shots[i].y;
        for (uint8_t i = 0; i < 2; ++i) if (game.bombs[i].y >= 0) {
            game.bombs[i].x+=bombDx[i];
            if (++game.bombs[i].y >= 14 || game.bombs[i].x<0 || game.bombs[i].x>=100) game.bombs[i].y = -1;
        }
        frameDirty = true;
    }
    checkCollisions(now);
    if (game.phase == PLAYING) dropBomb(now);
}
