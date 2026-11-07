# HV5812 / UCN5812 VFD with Arduino

Drive a 2x20-character VFD with four serial drivers and an Arduino Uno.
Each cell contains 5x7 dots. Start with one character, then text; the game is optional.

## Connect the panel

| Uno | Signal |
| --- | --- |
| D4 | Dot-chain DATA IN |
| D5 | Dot-chain CLOCK |
| D7 | Grid-chain DATA IN |
| D8 | Grid-chain CLOCK |
| D9 | Shared STROBE |
| D10 | Shared BLANK |
| GND | Logic and driver ground |

D6 is unused. These are signal connections, not IC pin numbers.
Check the chip package and trace the panel connections before wiring.

```text
D7 DATA -> grid driver 1 -> grid driver 2    D8 CLOCK
D4 DATA -> dot driver 1  -> dot driver 2     D5 CLOCK
             all four: D9 STROBE, D10 BLANK
```

The HV5812 requires 4.5–5.5 V logic and a separate 20–80 V VPP rail.
These are driver limits: use the panel's own anode/grid and filament ratings.
Never connect VPP to Arduino pins. Other boards require logic-level and timing checks.

Power up in this order: ground, VDD, defined logic inputs, then VPP; reverse
the order for shutdown. VPP must not float or fall below VDD during operation.
Keep BLANK HIGH during reset with an external pull-up resistor.

To supply the VFD, this build uses three isolated B0524S converters with
common 5 V inputs and series-connected 24 V outputs, providing nominal 72 V. Connect the bottom negative
terminal to driver ground and the top positive terminal to VPP. Only isolated
outputs can be stacked this way. The separate filament supply is 2x2.2 V AC,
with the transformer's centre tap tied to +5 V DC. Choose supplies that match the tube's voltage and current requirements.

## Upload and test

Open the example's main `.ino` in Arduino IDE, select
**Arduino Uno** and upload. Required headers are included; no library installation is needed.

1. [RawGrid](examples/RawGrid/RawGrid.ino): A in the top-left cell.
2. [TextDisplay](examples/TextDisplay/TextDisplay.ino): two lines, controlled through Serial Monitor.
3. [VfdInvaders](examples/VfdInvaders/README.md): optional joystick game.

For TextDisplay, select **115200 baud** and **Newline** or **Both NL & CR**:

```text
row1 HELLO VFD
row2 Arduino text
hold 150
scan active
clear
?
```

Each row accepts up to 20 printable ASCII characters. `hold` sets the per-grid
on-time (100–10000 microseconds); `scan all` also visits empty cells.

## How control works

Each HV5812 has a 20-bit shift register, output latches and 20 high-voltage
outputs. Connect DATA OUT to the next chip's DATA IN; share CLOCK, STROBE and
BLANK within a chain. Data shifts on CLOCK's rising edge. Keep STROBE LOW while
shifting, then pulse it HIGH to update the outputs. BLANK HIGH disables output;
LOW permits the latched pattern.

For this four-driver panel, each character pulse is:

1. Send a complete **40-bit grid word**, selecting one grid.
2. Send a complete **40-bit dot word** through the other chain.
3. Pulse the shared STROBE and hold the character; the default is **150 us**.
4. Shift 40 zero grid bits and latch again to turn the character off.

Repeat across the display. BLANK is HIGH during the 50 ms startup interval,
then stays LOW. Empty cells can be skipped in display time; transmitted words
always remain 40 bits. Active scanning is the default. With all cells occupied,
it visits the same positions as an all-cell scan.

Call `refresh()` frequently: it scans one cell per call. Long holds or blocking
code produce a visible sweep. A single active cell gets an extra 5 ms dark
interval in these examples to limit duty cycle.

## Mapping and API

`Hv5812Board.h` contains the pins and `GridSlot[2][20]`: top row 39..20,
bottom row 19..0, both left to right. Slots mean serial bit positions, not
IC output pin numbers. This mapping and the fixed 40-bit chains are panel-specific.

Five column bytes describe a character, sent left to right and LSB first.
**Bit 7 is the top dot; bit 1 is the bottom.** The font leaves bit 0 clear.
Map auxiliary dot/colon/triangle electrodes separately before using
`setExtras(row, column, mask)`; mask bits 0..4 select bit 0 of each column.
`VfdAscii.h` contains ASCII 32–126, stored in AVR flash.

| Call | Purpose |
| --- | --- |
| `panel.begin()` | Clear both chains and initialize BLANK |
| `panel.enable(true)` | Enable grid pulses; `false` clears selection |
| `panel.setHoldUs(150)` | Set grid hold time |
| `panel.setClockHalfPeriodUs(1)` | Set software clock timing |
| `panel.scanRaw(slot, columns)` | Pulse slot 0–39, then clear grids |
| `display.write(row, column, text)` | Write text; rows 0–1, columns 0–19 |
| `display.resetScan()` | Restart the sweep after changing text or scan mode |
| `display.refresh()` | Scan one cell; true means a sweep finished |

Enable the panel after `begin()`. Runtime enable/disable does not change BLANK.
Do not write to the same AVR ports from interrupt handlers.
After editing shared headers in `libraries/Hv5812Vfd/src`,
run `python tools/sync_headers.py` and `python tools/sync_headers.py --check`.

## Troubleshooting

- **Dark:** check supplies, filament, BLANK LOW, full words and STROBE; try RawGrid.
- **Wrong cell or mirrored glyph:** correct grid, column or bit mapping.
- **Flicker:** shorten hold time and avoid blocking the scan.
- **Dim sparse text:** use active scanning instead of waiting on empty cells.
- **Ghosting:** check grid clearing, filament reference and panel wiring.

[HV5812 datasheet: Microchip DS20005629A](https://ww1.microchip.com/downloads/en/DeviceDoc/20005629A.pdf).
Verify the exact UCN5812 variant's pinout and ratings before substitution.
