void beep(uint16_t duration) {
    if (!soundEnabled) return;
    digitalWrite(BuzzerPin, HIGH); // Active buzzer: its driver supplies the pitch.
    buzzerAt = millis();
    buzzerMs = duration;
}

void updateBuzzer(uint32_t now) {
    if (buzzerMs && uint32_t(now - buzzerAt) >= buzzerMs) {
        digitalWrite(BuzzerPin, LOW);
        buzzerMs = 0;
    }
}

void beginCalibration() {
    demoMode = false;
    clearShots();
    calibrationX = calibrationY = 0;
    calibrationSamples = 0;
    stickX = 0;
    setPhase(CALIBRATING, millis());
    Serial.println(F("CENTRE joystick for one second; wait for READY, then press FIRE."));
}

int8_t axisDirection(int delta, int8_t previous) {
    if (delta > Engage) return -1;
    if (delta < -Engage) return 1;
    if (abs(delta) < Release) return 0;
    return previous;
}

void updateInputs(uint32_t now) {
    if (uint32_t(now - lastInput) < 5) return;
    lastInput = now;
    // Discard the first conversion after switching ADC channels.
    analogRead(XPin); rawX = analogRead(XPin);
    analogRead(YPin); rawY = analogRead(YPin);
    const bool pressedLevel = digitalRead(FirePin);
    if (pressedLevel != fireRaw) { fireRaw = pressedLevel; fireChangedAt = now; }
    if (uint32_t(now - fireChangedAt) >= 30 && fireStable != fireRaw) {
        fireStable = fireRaw;
        if (fireStable == HIGH) fireArmed = true;
        else if (fireArmed) {
            fireArmed = false;
            if (game.phase == READY || game.phase == GAME_OVER || game.phase == SPRITES) startGame(false);
            else fireShot(now);
        }
    }
    if (fireStable == HIGH) fireArmed = true;
    if (game.phase == CALIBRATING) {
        calibrationX += rawX; calibrationY += rawY; ++calibrationSamples;
        if (uint32_t(now - phaseAt) >= 1000 && calibrationSamples) {
            centreX = filteredX = calibrationX / calibrationSamples;
            centreY = calibrationY / calibrationSamples;
            randomSeed(uint32_t(centreX) * 1024 + centreY + micros());
            fireArmed = fireStable == HIGH;
            setPhase(READY, now);
            Serial.print(F("READY centre_x=")); Serial.print(centreX);
            Serial.print(F(" centre_y=")); Serial.println(centreY);
        }
        return;
    }
    filteredX = (3 * filteredX + rawX) / 4;
    stickX = axisDirection(reverseX ? centreX - filteredX : filteredX - centreX, stickX);
    if (game.phase != PLAYING || demoMode) return;
    if (stickX && uint32_t(now - lastXMove) >= 30) {
        lastXMove = now; movePlayer(stickX);
    }
    if (fireStable == LOW) fireShot(now);
}

void togglePause() {
    const uint32_t now = millis();
    if (game.phase == PLAYING) {
        setPhase(PAUSED, now);
        digitalWrite(BuzzerPin, LOW); buzzerMs = 0;
    } else if (game.phase == PAUSED) {
        const uint32_t stopped = now - phaseAt;
        lastEnemyMove += stopped; lastShotMove += stopped; lastBomb += stopped; lastFire += stopped;
        game.invulnerableAt += stopped; game.blastAt += stopped;
        bossStageAt += stopped;
        setPhase(PLAYING, now);
    }
}

void printHelp() {
    Serial.println(F("menu | difficulty easy/normal/hard | size compact/classic | mode waves/rows"));
    Serial.println(F("FIRE: start / hold to shoot. Joystick: A0=left/right; ship height fixed. A2=diagnostic only."));
    Serial.println(F("play | demo (autopilot) | p pause | sprite | status | input | center"));
    Serial.println(F("a/d left/right | f fire | reversex | sound on/off"));
    Serial.println(F("hold/dwell 100..10000 us | scan active/all | ?"));
}

void printStatus() {
    Serial.print(F("GAME formation="));Serial.print(staggeredRows?F("staggered"):F("aligned"));
    Serial.print(F(" boss="));Serial.print(game.bossActive);
    Serial.print(F(" hp="));Serial.print(game.bossHp);
    Serial.print(F(" pattern="));Serial.println(game.bossPattern+1);
    Serial.print(F("STATE phase=")); Serial.print(game.phase);
    Serial.print(F(" x=")); Serial.print(game.playerX);
    Serial.print(F(" y=")); Serial.print(game.playerY);
    Serial.print(F(" lives=")); Serial.print(game.lives);
    Serial.print(F(" score=")); Serial.print(game.score);
    Serial.print(F(" wave=")); Serial.print(game.wave);
    Serial.print(F(" enemies=")); Serial.print(enemyCount());
    Serial.print(F(" rows=")); Serial.print(game.bossActive?0:game.secondRowAlive?2:1);
    Serial.print(F(" size=")); Serial.print(compactSprites?F("compact"):F("classic"));
    Serial.print(F(" mode=")); Serial.print(growingRows?F("rows"):F("waves"));
    Serial.print(F(" starting_hits=")); Serial.print(startingLives);
    Serial.print(F(" formation_x=")); Serial.print(game.formationX);
    Serial.print(F(" formation_y=")); Serial.print(game.formationY);
    uint8_t shots = 0, bombs = 0;
    for (uint8_t i = 0; i < 3; ++i) if (game.shots[i].y >= 0) ++shots;
    for (uint8_t i = 0; i < 2; ++i) if (game.bombs[i].y >= 0) ++bombs;
    Serial.print(F(" shots=")); Serial.print(shots);
    Serial.print(F(" bombs=")); Serial.print(bombs);
    Serial.print(F(" hold_us=")); Serial.print(holdUs);
    Serial.print(F(" scan=")); Serial.print(activeScan ? F("active") : F("all"));
    Serial.print(F(" cells=")); Serial.print(activeScan ? activeCount : 40);
    Serial.print(F(" sweep_hz="));
    if (scanPeriodUs) Serial.print(1000000.0f / scanPeriodUs, 1); else Serial.print(0);
    Serial.println(F(" BLANK=LOW"));
}

void printInput() {
    Serial.print(F("INPUT x=")); Serial.print(rawX);
    Serial.print(F(" y=")); Serial.print(rawY);
    Serial.print(F(" centre_x=")); Serial.print(centreX);
    Serial.print(F(" centre_y=")); Serial.print(centreY);
    Serial.print(F(" fire=")); Serial.print(fireStable == LOW ? F("DOWN") : F("UP"));
    Serial.print(F(" reverse_x=")); Serial.print(reverseX);
    Serial.println(F(" y_control=OFF"));
}

void handleCommand(char *line) {
    while (*line == ' ') ++line;
    char *end = line + strlen(line);
    while (end > line && end[-1] == ' ') *--end = 0;
    for (char *c = line; *c; ++c) if (*c >= 'A' && *c <= 'Z') *c += 'a' - 'A';
    if (!*line) return;
    if (handleMenuCommand(line)) return;
    if (!strcmp(line,"?")) printHelp();
    else if (!strcmp(line,"status")) printStatus();
    else if (!strcmp(line,"input")) printInput();
    else if (!strcmp(line,"play")) startGame(false);
    else if (!strcmp(line,"demo")) startGame(true);
    else if (!strcmp(line,"center")) beginCalibration();
    else if (!strcmp(line,"p")) togglePause();
    else if (!strcmp(line,"sprite")) { demoMode = false; clearShots(); setPhase(SPRITES, millis()); }
    else if (!strcmp(line,"reversex")) { reverseX = !reverseX; stickX = 0; printInput(); }
    else if (!strcmp(line,"sound on")) soundEnabled = true;
    else if (!strcmp(line,"sound off")) { soundEnabled = false; digitalWrite(BuzzerPin, LOW); buzzerMs = 0; }
    else if (!strcmp(line,"scan active") || !strcmp(line,"scan all")) {
        activeScan = !strcmp(line,"scan active"); scanIndex = 0; lastScan = scanPeriodUs = 0;
    } else if (!strncmp(line,"hold ",5) || !strncmp(line,"dwell ",6)) {
        const char *value = strchr(line,' ') + 1;
        uint16_t parsed = 0;
        bool valid = *value != 0;
        for (; *value && valid; ++value) {
            if (*value < '0' || *value > '9' || parsed > 1000) valid = false;
            else { parsed = parsed * 10 + (*value - '0'); if (parsed > 10000) valid = false; }
        }
        if (!valid || parsed < 100) { Serial.println(F("ERR hold range: 100..10000 us")); return; }
        holdUs = parsed; panel.setHoldUs(holdUs); lastScan = scanPeriodUs = 0;
    } else if (line[0] && !line[1]) {
        if (line[0] == 'a') movePlayer(-1);
        else if (line[0] == 'd') movePlayer(1);
        else if (line[0] == 'f') fireShot(millis());
        else { Serial.println(F("ERR unknown command; send ?")); return; }
    } else { Serial.println(F("ERR unknown command; send ?")); return; }
    Serial.println(F("OK"));
}

void finishCommand() {
    if (commandOverflow) Serial.println(F("ERR line too long; discarded"));
    else { command[commandLength] = 0; handleCommand(command); }
    commandLength = 0; commandOverflow = false;
}

void readSerial() {
    while (Serial.available()) {
        const char c = Serial.read(); lastByteAt = millis();
        if (c == '\r' || c == '\n') { if (commandLength || commandOverflow) finishCommand(); }
        else if (!commandOverflow) {
            if (commandLength < sizeof(command) - 1) command[commandLength++] = c;
            else commandOverflow = true;
        }
    }
    if ((commandLength || commandOverflow) && uint32_t(millis() - lastByteAt) >= 100) finishCommand();
}
