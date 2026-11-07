# HV5812 / UCN5812 VFD control with Arduino

Control a 2x20-character vacuum fluorescent display using four serial
drivers and an Arduino Uno. Each character has a 5x7 dot matrix. Examples cover
raw grid selection, ASCII text and an optional Space Invaders-style game.

## How the drivers work

The HV5812 has a 20-bit shift register, output latches and 20 high-voltage
outputs. Connect **DATA OUT** to the next chip's **DATA IN** to cascade chips;
share their CLOCK, STROBE and BLANK signals. A two-chip chain needs 40 clock
pulses for a complete output pattern. Data enters on CLOCK's rising edge.
STROBE transfers the shift-register contents to the outputs; shifting while
STROBE is low leaves the displayed pattern unchanged.

This panel has two separate chains:

```text
D7 DATA  -> grid driver 1 -> grid driver 2     D8 CLOCK
D4 DATA  -> dot driver 1  -> dot driver 2      D5 CLOCK
              all four: D9 STROBE, D10 BLANK
```

To light one character, send a 40-bit grid word with one selected bit and a
40-bit dot word, then pulse the shared STROBE. Hold the grid briefly, shift
40 zero grid bits and latch again to turn it off. Repeat for the other cells.
The default hold is 150 microseconds. BLANK starts HIGH for initialization,
then stays LOW; runtime blanking is done with zero grid data.

Skipping empty cells saves display time, not serial bits. Every transmitted
word still contains all 40 bits. With a full display, active and all-cell scans
visit the same 40 positions. Hold time is per grid, not the screen refresh rate;
long holds produce a visible sweep, while short holds reduce duty cycle.

## Wiring

| Uno pin | Panel signal |
| --- | --- |
| D4 | Dot-chain DATA IN |
| D5 | Dot-chain CLOCK |
| D7 | Grid-chain DATA IN |
| D8 | Grid-chain CLOCK |
| D9 | Shared STROBE |
| D10 | Shared BLANK |
| GND | Logic and driver ground |

D6 is unused. These are signal connections, not physical IC pin numbers.
Check the package pinout and trace the panel connections before wiring.

The HV5812 uses 4.5–5.5 V logic and a separate 20–80 V high-voltage rail.
These are driver operating ranges, not a panel or filament specification.
Do not connect the high-voltage rail to Arduino pins. A 3.3 V controller needs
a compatible logic interface; the examples target the 5 V Uno.

Power-up order: ground, logic supply, defined input levels, then high voltage;
power down in reverse. Do not let VPP float or fall below VDD during operation.
Keep BLANK HIGH in hardware during reset: firmware cannot control floating
inputs before setup runs or enforce external power sequencing.

The panel supply uses three isolated B0524S 5-to-24 V converters: common 5 V
inputs, isolated outputs connected in series for nominal 72 V. Series stacking
requires isolated outputs: the bottom negative terminal connects to driver ground,
and the top positive terminal supplies VPP. The filament has a separate 2x2.2 V AC transformer,
with its centre tap tied to +5 V DC. Supply ratings, loading and insulation must
suit the panel; this connection summary is not a complete power-supply schematic.

## Run an example

Download the repository and open one complete example folder in Arduino IDE.
Select **Arduino Uno** and upload. Required headers are already beside each
sketch; no custom library installation is needed.

1. [RawGrid](examples/RawGrid/RawGrid.ino): displays A in the top-left cell.
   Change the logical row/column or five column bytes to check the mapping.
2. [TextDisplay](examples/TextDisplay/TextDisplay.ino): displays two lines and
   accepts commands at **115200 baud**, with Newline or Both NL & CR selected.
3. [VfdInvaders](examples/VfdInvaders/README.md): optional joystick game.

Text commands:

```text
row1 HELLO VFD
row2 Arduino text
hold 150
scan active
clear
?
```

`hold` accepts 100–10000 microseconds; `scan all` also visits empty cells.
Each row accepts up to 20 printable ASCII characters. Call `refresh()` frequently:
it sends one grid pulse per call. Long blocking operations interrupt the scan.
Single-cell examples insert a 5 ms dark interval to limit duty cycle.

## Adapt the mapping

`Hv5812Board.h` defines the signal pins and `GridSlot[2][20]`. Entries are serial
bit positions, not numbered IC output pins. This panel's top row maps to slots
39..20, and its bottom row to 19..0, both left to right.

Each cell uses five bytes, transmitted left to right, least-significant bit
first. Within a column, **bit 7 is the top dot and bit 1 the bottom dot**.
Bit 0 is reserved for auxiliary electrodes; it is clear in the font. Dot,
colon and triangle connections must be mapped separately before using
`setExtras(row, column, mask)`; mask bits select bit 0 of the five column bytes.

`VfdAscii.h` contains ASCII 32–126. `ascii_5x7_column(character, column)` reads
one column from flash on AVR. Changing boards or panels requires checking
pin levels, slot order, column order, polarity and timing.

## Driver API

| Call | Action |
| --- | --- |
| `panel.begin()` | Clear both chains; hold BLANK HIGH for 50 ms, then LOW |
| `panel.enable(true/false)` | Allow grid pulses or clear grid selection |
| `panel.setHoldUs(150)` | Set per-grid hold |
| `panel.setClockHalfPeriodUs(1)` | Set software clock half-period |
| `panel.scanRaw(slot, columns)` | Pulse slot 0–39, then clear all grids |
| `display.write(row, column, text)` | Put text into the two-row buffer |
| `display.refresh()` | Scan one cell; return true at the end of a sweep |

Call `enable(true)` after initialization. Changing enable state does not toggle
BLANK. The transport uses cached AVR port writes; do not modify those same port
bits from an interrupt handler. Other architectures need separate validation.

### Troubleshooting

- **Nothing lights:** check supplies, filament power, BLANK LOW, full words and
  the shared STROBE; start with RawGrid.
- **Wrong cell or mirrored text:** correct `GridSlot` or column/bit mapping.
- **Visible sweep or flicker:** reduce hold time and keep loop work short.
- **Dim sparse text:** use active scanning; do not add waits for empty cells.
- **Ghosting:** check that grids clear between pulses, then inspect filament
  supply/reference and panel wiring; scan timing alone does not diagnose it.

Shared source is in `libraries/Hv5812Vfd/src`. After editing it, run
`python tools/sync_headers.py`, then `python tools/sync_headers.py --check` to
update all bundled copies.

HV5812 electrical limits and timing: [Microchip DS20005629A](https://ww1.microchip.com/downloads/en/DeviceDoc/20005629A.pdf).
For UCN5812 variants, verify the exact part's pinout and electrical ratings
against its own datasheet before substitution.
