void printMenu() {
    if (game.phase==PLAYING) togglePause();
    menuPage=1;
    Serial.println(F("=== INVADERS MENU ==="));
    Serial.print(F("1 Difficulty: ")); Serial.println(startingLives==3?F("Easy (3 hits)"):startingLives==2?F("Normal (2 hits)"):F("Hard (1 hit)"));
    Serial.print(F("2 Sprites: ")); Serial.println(compactSprites?F("Compact 5x4 / 3x2"):F("Classic 5x7 / 5x3"));
    Serial.print(F("3 Levels: ")); Serial.println(growingRows?F("Growing rows"):F("Fresh waves"));
    Serial.println(F("4 Play | 5 Sprite preview | 0 Close menu"));
    Serial.print(F("6 Formation: "));Serial.println(staggeredRows?F("Staggered"):F("Aligned"));
    Serial.print(F("7 Boss every 3 levels: "));Serial.println(bossesEnabled?F("On"):F("Off"));
    Serial.println(F("8 Test boss now"));
    Serial.println(F("9 Test two rows now"));
    Serial.println(F("Type a number then Enter. Direct commands also work."));
}

void settingsChanged() {
    demoMode=false;
    if (game.phase!=CALIBRATING) {
        clearShots();
        setPhase(READY,millis());
    }
    Serial.println(F("Settings ready. Press FIRE or send play for a new round."));
    printMenu();
}

bool handleMenuCommand(const char *line) {
    if (!strcmp(line,"menu")) { printMenu(); return true; }
    if(!strcmp(line,"formation staggered") || !strcmp(line,"formation aligned")) {
        staggeredRows=!strcmp(line,"formation staggered");settingsChanged();return true;
    }
    if(!strcmp(line,"boss on") || !strcmp(line,"boss off")) {
        bossesEnabled=!strcmp(line,"boss on");settingsChanged();return true;
    }
    if(!strcmp(line,"boss test")) {
        if(game.phase==CALIBRATING) {Serial.println(F("Wait for READY"));return true;}
        startGame(false);game.wave=3;beginBoss(millis());return true;
    }
    if(!strcmp(line,"rows test")) {
        if(game.phase==CALIBRATING) {Serial.println(F("Wait for READY"));return true;}
        compactSprites=true;growingRows=true;
        startGame(false);game.wave=2;beginWave(millis());return true;
    }
    if (!strcmp(line,"difficulty easy") || !strcmp(line,"difficulty normal") || !strcmp(line,"difficulty hard")) {
        startingLives=!strcmp(line,"difficulty easy")?3:!strcmp(line,"difficulty normal")?2:1;
        settingsChanged(); return true;
    }
    if (!strcmp(line,"size compact") || !strcmp(line,"size classic")) {
        compactSprites=!strcmp(line,"size compact");
        if (!compactSprites && growingRows) {
            growingRows=false;
            Serial.println(F("Classic art fits one row: switched levels to waves."));
        }
        settingsChanged(); return true;
    }
    if (!strcmp(line,"mode waves") || !strcmp(line,"mode rows")) {
        growingRows=!strcmp(line,"mode rows");
        if (growingRows && !compactSprites) {
            compactSprites=true;
            Serial.println(F("Two rows require compact art: switched to compact."));
        }
        settingsChanged(); return true;
    }
    if (!menuPage || !line[0] || line[1] || line[0]<'0' || line[0]>'9') return false;
    const uint8_t choice=line[0]-'0';
    if (!choice) { menuPage=0; Serial.println(F("Menu closed")); return true; }
    if (menuPage==1) {
        if (choice==1) { menuPage=2; Serial.println(F("Difficulty: 1 Easy / 2 Normal / 3 Hard / 0 Close")); }
        else if (choice==2) { menuPage=3; Serial.println(F("Sprites: 1 Compact / 2 Classic / 0 Close")); }
        else if (choice==3) { menuPage=4; Serial.println(F("Levels: 1 Fresh waves / 2 Growing rows / 0 Close")); }
        else if (choice==4) { menuPage=0; startGame(false); }
        else if(choice==5) { menuPage=0; demoMode=false; clearShots(); setPhase(SPRITES,millis()); }
        else if(choice==6) {menuPage=5;Serial.println(F("Formation: 1 Staggered / 2 Aligned / 0 Close"));}
        else if(choice==7) {menuPage=6;Serial.println(F("Boss: 1 On / 2 Off / 0 Close"));}
        else return handleMenuCommand(choice==8?"boss test":"rows test");
        return true;
    }
    if (menuPage==2 && choice<=3) return handleMenuCommand(choice==1?"difficulty easy":choice==2?"difficulty normal":"difficulty hard");
    if (menuPage==3 && choice<=2) return handleMenuCommand(choice==1?"size compact":"size classic");
    if (menuPage==4 && choice<=2) return handleMenuCommand(choice==1?"mode waves":"mode rows");
    if (menuPage==5 && choice<=2) return handleMenuCommand(choice==1?"formation staggered":"formation aligned");
    if (menuPage==6 && choice<=2) return handleMenuCommand(choice==1?"boss on":"boss off");
    Serial.println(F("ERR invalid menu choice")); return true;
}
