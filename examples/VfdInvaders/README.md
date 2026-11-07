# Optional VFD game

Open `VfdInvaders.ino` with the entire folder present. Upload to an Arduino Uno.
Display wiring and supplies are described in the [main tutorial](../../README.md).

| Control | Pin |
| --- | --- |
| Joystick horizontal output | A0 |
| Second joystick axis; diagnostic only | A2 |
| Fire button; normally open to GND | D12, internal pull-up |
| Active buzzer through a transistor driver | D11; HIGH switches on |

Use a joystick with analog outputs in the Uno's 0–5 V input range and a common
ground. Size the buzzer transistor, bias resistor and supply for the actual load;
do not power the buzzer directly from the GPIO. This is an active buzzer, not a
passive speaker driven with `tone()`.

Keep the joystick centred during the first second after reset. Press FIRE to
start; move left/right and hold FIRE to shoot. Serial Monitor uses 115200 baud.
Send `menu` for numbered settings, `reversex` to reverse steering, or `center`
to recalibrate. `a`, `d` and `f` provide serial movement/fire; `p` pauses.

The menu selects difficulty (3/2/1 hits), compact or full-size sprites, fresh
waves or growing rows, formation alignment and boss encounters. A boss appears
after each third cleared formation and requires multiple hits. `boss test`
starts an immediate encounter; `demo` enables automatic play; `play` starts a
manual round. Send `?` for the remaining commands.

Graphics use a 100x14 logical canvas across the real gaps between cells. Text
and score digits stay aligned to 5x7 cells. Compact sprites are the default;
the larger sprites leave less vertical room. Display hold defaults to 150 us.
