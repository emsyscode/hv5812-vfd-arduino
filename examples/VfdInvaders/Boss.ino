// Boss encounters share the game's bullets, damage allowance and pause handling.
void beginBoss(uint32_t now) {
    clearShots();game.alive=game.secondRowAlive=0;
    game.bossActive=true;
    game.bossHp=8+min(uint8_t(8),uint8_t((game.wave/3-1)*2));
    game.bossX=44;game.bossY=0;game.bossDirection=1;game.bossPattern=0;
    bossStageAt=lastEnemyMove=lastShotMove=lastBomb=now;
    setPhase(PLAYING,now);
    Serial.print(F("BOSS level="));Serial.print(game.wave);
    Serial.print(F(" hp="));Serial.println(game.bossHp);
}

bool bossPixel(int x,int y) {
    const int dx=x-game.bossX,dy=y-game.bossY;
    return dx>=0 && dx<11 && dy>=0 && dy<7 &&
        (pgm_read_word(BossArt+dy)&(uint16_t(1)<<(10-dx)));
}

void hitBoss(Shot &shot,uint32_t now) {
    if(!game.bossHp || (!bossPixel(shot.x,shot.y) && !bossPixel(shot.x,shot.y+1))) return;
    shot.y=-1;--game.bossHp;
    game.blast=true;game.blastAt=now;game.blastX=game.bossX+3;game.blastY=game.bossY+1;
    if(!game.bossHp) game.score=game.score<=65435?game.score+100:65535;
    frameDirty=true;beep(game.bossHp?25:180);
    Serial.print(F("BOSS hp="));Serial.println(game.bossHp);
}

void moveBoss(uint32_t now) {
    if(uint32_t(now-bossStageAt)>=4000) {
        game.bossPattern=(game.bossPattern+1)%3;bossStageAt=now;
        Serial.print(F("BOSS pattern="));Serial.println(game.bossPattern+1);
    }
    if(uint32_t(now-lastEnemyMove)<90) return;
    lastEnemyMove=now;
    if(game.bossPattern==2) { // Track the ship, leaving a two-dot firing gap.
        const int target=constrain(game.playerX+int(shipWidth()/2)-5,0,89);
        game.bossX+=target>game.bossX?1:target<game.bossX?-1:0;
    } else {
        const int next=game.bossX+game.bossDirection;
        if(next<0 || next>89) game.bossDirection=-game.bossDirection;
        game.bossX+=game.bossDirection;
    }
    // Zigzag adds vertical motion; all patterns leave room above either ship.
    const uint8_t step=((now-bossStageAt)/360)%4;
    game.bossY=game.bossPattern==1?(step<=2?step:1):0;
    frameDirty=true;
}

void bossAttack(uint32_t now) {
    const uint16_t intervals[3]={900,700,600};
    if(uint32_t(now-lastBomb)<intervals[game.bossPattern]) return;
    lastBomb=now;
    for(uint8_t b=0;b<2;++b) if(game.bombs[b].y<0) {
        game.bombs[b].x=game.bossX+(b?8:2);
        game.bombs[b].y=game.bossY+7;
        const int target=game.playerX+shipWidth()/2;
        // Patterns: straight shots, aimed diagonals, outward two-shot fan.
        bombDx[b]=game.bossPattern==0?0:game.bossPattern==2?(b?1:-1):
            target>game.bombs[b].x?1:target<game.bombs[b].x?-1:0;
        frameDirty=true;
    }
}

void drawBoss() {
    for(uint8_t y=0;y<7;++y) {
        const uint16_t row=pgm_read_word(BossArt+y);
        for(uint8_t x=0;x<11;++x) if(row&(uint16_t(1)<<(10-x))) putPixel(game.bossX+x,game.bossY+y);
    }
}
