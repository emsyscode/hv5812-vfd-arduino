void putPixel(int x, int y) {
    if (x < 0 || x >= 100 || y < 0 || y >= 14) return;
    // Each physical cell is five columns. Bit 7 is its top dot; bit 0 is reserved.
    frame[(y / 7) * 20 + x / 5][x % 5] |= uint8_t(0x80 >> (y % 7));
}

void drawSprite(int x, int y, const uint8_t *rows, uint8_t height) {
    for (uint8_t r = 0; r < height; ++r) {
        const uint8_t bits = pgm_read_byte(rows + r);
        for (uint8_t c = 0; c < 5; ++c) if (bits & (0x10 >> c)) putPixel(x + c, y + r);
    }
}

// Text/HUD uses complete character cells; actors still use the pixel canvas.
void drawCellText(uint8_t row, uint8_t column, const char *text) {
    if (row >= 2) return;
    while (*text && column < 20) {
        for (uint8_t c = 0; c < 5; ++c) frame[row * 20 + column][c] = ascii_5x7_column(*text,c);
        ++text; ++column;
    }
}

void drawCellNumber(uint8_t row, uint8_t column, uint16_t value) {
    char digits[6];
    utoa(value,digits,10);
    drawCellText(row,column,digits);
}

void renderFrame(uint32_t now) {
    memset(frame, 0, sizeof(frame));
    if (game.phase == SPRITES) {
        drawSprite(40,0,enemyArt(0),enemyHeight());
        drawSprite(50,0,enemyArt(1),enemyHeight());
        drawSprite(40,14-shipHeight(),shipArt(),shipHeight());
        drawSprite(50,8,Explosion,5);
    } else if (game.phase == DYING) {
        // Keep the complete X on-screen, centred horizontally on the last ship.
        const int x = constrain(game.deathX + int(shipWidth()/2) - 2,0,95);
        const int y = constrain(game.deathY + int(shipHeight()/2) - 2,0,9);
        if (((now-phaseAt)/100)%2 == 0) drawSprite(x,y,Explosion,5);
    } else if (game.phase == GAME_OVER || game.phase == PAUSED) {
        drawCellText(0,0,"Score:"); drawCellNumber(0,7,game.score);
        if (game.phase == GAME_OVER) drawCellText(1,0,"FIRE TO RESTART");
        else { drawCellText(1,0,"Hits:"); drawCellNumber(1,6,game.lives); }
    } else if (game.phase == WAVE_CLEAR) {
        if(bossesEnabled && game.wave%3==0 && !game.bossActive) drawCellText(0,0,"BOSS INCOMING");
        else {drawCellText(0,0,"Level:"); drawCellNumber(0,7,game.wave < 255 ? game.wave+1 : 255);}
        drawCellText(1,0,"Score:"); drawCellNumber(1,7,game.score);
    } else if (game.phase == READY || game.phase == CALIBRATING) {
        drawCellText(0,0,"VFD INVADERS");
        if (game.phase == CALIBRATING) drawCellText(1,0,"CENTRE JOYSTICK");
        else {
            drawCellText(1,0,startingLives==3 ? "Easy" : startingLives==2 ? "Normal" : "Hard");
            drawCellText(1,7,compactSprites ? "Small" : "Large");
            drawCellText(1,13,growingRows ? "Rows" : "Waves");
        }
    } else {
        const uint8_t pose = (now/400)&1;
        if(game.bossActive && game.bossHp) drawBoss();
        for (uint8_t e=0;e<16;++e) if (enemyAlive(e))
            drawSprite(enemyX(e),enemyY(e),enemyArt(pose),enemyHeight());
        if (!game.invulnerable || (now/100)%2) drawSprite(game.playerX,game.playerY,shipArt(),shipHeight());
        for (uint8_t i=0;i<3;++i) if (game.shots[i].y>=0) {
            putPixel(game.shots[i].x,game.shots[i].y); putPixel(game.shots[i].x,game.shots[i].y+1);
        }
        for (uint8_t i=0;i<2;++i) if (game.bombs[i].y>=0) {
            putPixel(game.bombs[i].x,game.bombs[i].y); putPixel(game.bombs[i].x,game.bombs[i].y+1);
        }
        if (game.blast) drawSprite(game.blastX,game.blastY,Explosion,5);
    }
    activeCount=0;
    for (uint8_t cell=0;cell<40;++cell) {
        bool lit=false;
        for (uint8_t c=0;c<5;++c) if (frame[cell][c]) lit=true;
        if (lit) activeCells[activeCount++]=cell;
    }
    frameDirty=false;
}

void scanDisplay() {
    const uint8_t count = activeScan ? activeCount : 40;
    if (!count) return; // The last pulse already cleared the grid chain.
    const uint8_t cell = activeScan ? activeCells[scanIndex] : scanIndex;
    panel.scanRaw(Board::GridSlot[cell / 20][cell % 20], frame[cell]);
    if (++scanIndex >= count) {
        scanIndex = 0;
        const uint32_t now = micros();
        if (lastScan) scanPeriodUs = now - lastScan;
        lastScan = now;
        if (count == 1) {
            singleCellOffUntil = now + 5000; // Avoid near-continuous single-cell drive.
            singleCellWaiting = true;
        }
    }
}
