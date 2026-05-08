# tele-sketch — Hardware Bindings Handoff

> **Source of truth:** the schematic image (Schematic1, V1.0, updated 2026-04-24).
> **Purpose:** allow another Claude Code instance to verify [main/main.cpp](main/main.cpp) is in sync with the manufactured PCB. The receiving Claude should treat the **Schematic** column as authoritative and update code to match.
>
> Some GPIO numbers below are marked ❓ **VERIFY** because they could not be read from the rendered schematic image with full confidence. The receiving Claude should ask the user to confirm those before editing code.

---

## 1. Schematic bindings (authoritative)

MCU: **ESP32-S3-WROOM-1-N8R2** (U1). Display: 4.0" TFT, ILI9486 over 4-wire SPI (J2). Joystick: analog X/Y + click (J1). 8 momentary buttons (SW1–SW8). USB-C with 5.1 kΩ CC pulldowns; LDO U2 → +3.3 V rail; D1 protects VBUS.

| Net           | Connects to            | ESP32-S3 GPIO | Direction      | Notes                                             |
|---------------|------------------------|---------------|----------------|---------------------------------------------------|
| `JS_X`        | Joystick J1 X-wiper    | **IO4**       | Analog in (ADC1_CH3) | 100 nF decoupling C6                        |
| `JS_Y`        | Joystick J1 Y-wiper    | **IO5**       | Analog in (ADC1_CH4) |                                              |
| `B5`          | Joystick J1 click      | **IO6**       | Digital in, pull-up | Active-low (button to GND)                  |
| `B1`          | SW1                    | **IO7**       | Digital in, pull-up | 100 nF debounce cap C7                      |
| `B2`          | SW2                    | **IO15**      | Digital in, pull-up | C8                                          |
| `B3`          | SW3                    | **IO16**      | Digital in, pull-up | C10                                         |
| `B4`          | SW4                    | **IO17**      | Digital in, pull-up | C11                                         |
| `MENU`        | SW6                    | ❓ VERIFY (likely **IO36**) | Digital in, pull-up | Right-side pin of U1, C9          |
| `SPARE`       | SW7                    | ❓ VERIFY (likely **IO35**) | Digital in, pull-up | Right-side pin of U1, C12         |
| `BOOT`        | SW5                    | **IO0**       | Strapping pin  | Bootloader entry only — do **not** repurpose      |
| `RESET`       | SW8 → EN               | — (EN)        | Hardware reset | 10 kΩ pull-up R3, 100 nF C13 — no firmware action |
| `USB_DM`      | USB-C D-               | **IO19**      | USB peripheral | Reserved for native USB                           |
| `USB_DP`      | USB-C D+               | **IO20**      | USB peripheral | Reserved for native USB                           |
| `LCD_SCK`     | J2 SPI clock           | ❓ VERIFY (currently **IO12** in code) | SPI out | Bottom edge of U1 |
| `LCD_SDI`     | J2 SPI MOSI            | ❓ VERIFY (currently **IO11** in code) | SPI out |                       |
| `LCD_DC/RS`   | J2 data/command select | ❓ VERIFY (currently **IO10** in code) | Digital out |                  |
| `LCD_CS`      | J2 chip select         | ❓ VERIFY (currently **IO9**  in code) | Digital out | 10 kΩ pull-up R4    |
| `LCD_RESET`   | J2 panel reset         | ❓ VERIFY (currently **IO21** in code) | Digital out |                  |
| `+3.3V`       | LDO U2 output          | —             | Power          | 4.7 µF in/out caps; powers MCU and LCD            |
| `VBUS`        | USB-C VBUS             | —             | Power input    | Through D1 to LDO                                 |

---

## 2. Current firmware bindings ([main/main.cpp](main/main.cpp))

| Function         | Code symbol                               | GPIO  | Source line                |
|------------------|-------------------------------------------|-------|----------------------------|
| Joystick X       | `JOY_X_CHAN` = `ADC_CHANNEL_3`            | IO4   | [main.cpp:15](main/main.cpp#L15) |
| Joystick Y       | `JOY_Y_CHAN` = `ADC_CHANNEL_4`            | IO5   | [main.cpp:16](main/main.cpp#L16) |
| Joystick click   | *(not handled)*                           | —     | —                          |
| BTN_DRAW         | `BTN_DRAW_PIN`                            | 15    | [main.cpp:18](main/main.cpp#L18) |
| BTN_COLOR        | `BTN_COLOR_PIN`                           | 16    | [main.cpp:19](main/main.cpp#L19) |
| BTN_UNDO         | `BTN_UNDO_PIN`                            | 17    | [main.cpp:20](main/main.cpp#L20) |
| BTN_TOOL         | `BTN_TOOL_PIN`                            | 18    | [main.cpp:21](main/main.cpp#L21) |
| MENU             | *(not handled)*                           | —     | —                          |
| SPARE            | *(not handled)*                           | —     | —                          |
| LCD SCLK         | `cfg.pin_sclk`                            | 12    | [main.cpp:37](main/main.cpp#L37) |
| LCD MOSI         | `cfg.pin_mosi`                            | 11    | [main.cpp:38](main/main.cpp#L38) |
| LCD MISO         | `cfg.pin_miso`                            | -1    | [main.cpp:39](main/main.cpp#L39) |
| LCD DC           | `cfg.pin_dc`                              | 10    | [main.cpp:40](main/main.cpp#L40) |
| LCD CS           | `cfg.pin_cs`                              | 9     | [main.cpp:46](main/main.cpp#L46) |
| LCD RST          | `cfg.pin_rst`                             | 21    | [main.cpp:47](main/main.cpp#L47) |

---

## 3. Diff vs code — action items

| Schematic net | Schematic GPIO | Code GPIO | Status | Action |
|---------------|----------------|-----------|--------|--------|
| `JS_X`        | IO4            | IO4       | ✅ OK  | none |
| `JS_Y`        | IO5            | IO5       | ✅ OK  | none |
| `B5` (joy click) | IO6        | —         | 🆕 NEW | Add `#define BTN_JOYCLICK_PIN 6` and include in `pin_bit_mask` at [main.cpp:113](main/main.cpp#L113). Behaviour suggestion in §4. |
| `B1`          | IO7            | (was DRAW=15) | ⚠️ MISMATCH | The schematic renames buttons to B1–B4; software still uses the old GPIOs **and** the old function names. Decide mapping (see §4) then update [main.cpp:18-21](main/main.cpp#L18-L21). If `B1`→DRAW: change `BTN_DRAW_PIN` from 15 → 7. |
| `B2`          | IO15           | (was COLOR=16) | ⚠️ MISMATCH | If `B2`→COLOR: `BTN_COLOR_PIN` 16 → 15. |
| `B3`          | IO16           | (was UNDO=17)  | ⚠️ MISMATCH | If `B3`→UNDO:  `BTN_UNDO_PIN`  17 → 16. |
| `B4`          | IO17           | (was TOOL=18)  | ⚠️ MISMATCH | If `B4`→TOOL:  `BTN_TOOL_PIN`  18 → 17. |
| `MENU`        | ❓ ~IO36       | —         | 🆕 NEW + ❓ VERIFY | Confirm pin with user, then add `#define BTN_MENU_PIN <n>` and pull-up config. Behaviour in §4. |
| `SPARE`       | ❓ ~IO35       | —         | 🆕 NEW + ❓ VERIFY | Confirm pin; configure as input + pull-up to avoid floating, no behaviour yet. |
| `BOOT` / `RESET` | IO0 / EN    | —         | ✅ OK  | Hardware-only; no firmware change. |
| `LCD_SCK`     | ❓ VERIFY      | IO12      | ❓ VERIFY | Confirm schematic pin matches IO12 — if not, update [main.cpp:37](main/main.cpp#L37). |
| `LCD_SDI`     | ❓ VERIFY      | IO11      | ❓ VERIFY | Confirm vs [main.cpp:38](main/main.cpp#L38). |
| `LCD_DC/RS`   | ❓ VERIFY      | IO10      | ❓ VERIFY | Confirm vs [main.cpp:40](main/main.cpp#L40). |
| `LCD_CS`      | ❓ VERIFY      | IO9       | ❓ VERIFY | Confirm vs [main.cpp:46](main/main.cpp#L46). |
| `LCD_RESET`   | ❓ VERIFY      | IO21      | ❓ VERIFY | Confirm vs [main.cpp:47](main/main.cpp#L47). |

---

## 4. Suggested behaviour for new inputs

These are **proposals** — the receiving Claude should confirm with the user before wiring them up.

- **`B5` (joystick click, IO6):** quick toggle of `is_eraser` (today this requires a 500 ms hold on `BTN_TOOL` at [main.cpp:303-313](main/main.cpp#L303-L313)). One-tap eraser toggle is a natural fit for a click-while-aiming gesture.
- **`MENU` (SW6):** enter a settings/options overlay — palette mode (light/dark), LCD rotation, brush size table. Until that overlay exists, leave the handler as a no-op `printf("MENU pressed")` so the wiring is exercised.
- **`SPARE` (SW7):** configure as `GPIO_MODE_INPUT` with pull-up at [main.cpp:106-114](main/main.cpp#L106-L114) but bind no behaviour. This prevents a floating input and reserves it for future use.
- **`B1`–`B4`:** straightforward rename of the existing four functions onto the new GPIOs — DRAW=B1, COLOR=B2, UNDO=B3, TOOL=B4. Only the four `#define` values change; all the press/hold logic in `app_main` keeps working.

---

## 5. Verification checklist (for the receiving Claude)

1. Open [main/main.cpp](main/main.cpp).
2. For every row in §3, read the current code value and compare to the **Schematic GPIO** column.
3. For ❓ VERIFY rows: ask the user to confirm the pin number from the schematic before changing code.
4. Apply changes only as listed in §3 / §4. Do not refactor surrounding logic.
5. After edits:
   - `idf.py build` succeeds.
   - No GPIO appears in two roles.
   - Strapping pins not repurposed: **IO0** (BOOT), **IO45**, **IO46** untouched; **IO19/IO20** left for USB.
6. Smoke test on hardware:
   - Joystick moves cursor; B5 toggles eraser.
   - Each of B1–B4 produces the action it was renamed to.
   - LCD initialises and shows the canvas.
   - MENU prints/responds; SPARE press is silently ignored (but pin is not floating).

---

## 6. Open questions for the user

- Confirm exact GPIO numbers for `MENU`, `SPARE`, and the five `LCD_*` nets — these were not legible enough from the rendered schematic image.
- Confirm the B1↔DRAW, B2↔COLOR, B3↔UNDO, B4↔TOOL mapping (the schematic numbers them but doesn't assert function).
- Confirm whether `B5` should be "toggle eraser" or some other action (e.g. "place stamp", "save snapshot now").
