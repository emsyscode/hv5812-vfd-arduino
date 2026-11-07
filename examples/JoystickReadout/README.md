# Joystick readout

Connect the joystick's 5 V and GND to the Uno, its horizontal output to A0,
and its vertical output to A2. Connect a normally open button between D12 and GND.
No display is needed.

Upload `JoystickReadout.ino` and open Serial Monitor at **115200 baud**.
Move each axis and press the button. The sketch prints raw X/Y readings
(0–1023) and the button state every 100 ms. A centred joystick usually reads
near 512; the game calibrates its horizontal centre automatically.

The game uses only A0 and D12. This example reads both axes independently.
