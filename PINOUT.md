# Tomogatchi USB — Pinout

## Hardware

- **MCU:** Raspberry Pi Pico (RP2040)
- **Display:** Adafruit #3533 — 0.96" 160×80 ST7735R TFT
- **Storage:** MicroSD card module (SPI)

All SPI peripherals share **SPI0** on the same bus. CS pins are used to select
between them.

---

## Wiring Table

| Signal    | GPIO | Physical Pin | Connected To       |
| --------- | ---- | ------------ | ------------------ |
| SPI0 SCK  | GP18 | 24           | TFT SCK + SD SCK   |
| SPI0 MOSI | GP19 | 25           | TFT MOSI + SD MOSI |
| SPI0 MISO | GP16 | 21           | SD MISO            |
| TFT CS    | GP17 | 22           | TFT CS             |
| SD CS     | GP15 | 20           | SD CS              |
| TFT DC    | GP20 | 26           | TFT DC             |
| TFT RST   | GP21 | 27           | TFT RST            |
| TFT BL    | GP22 | 29           | TFT Backlight      |
| 3.3V      | —    | 36           | TFT VIN + SD VCC   |
| GND       | —    | 38           | TFT GND + SD GND   |

---

## Notes

- **MISO** is only connected to the SD card — the ST7735R is write-only.
- **Backlight (BL)** is driven high by firmware on init. Wire directly to GP22
  or to 3.3V if you want it always on.
- **BOOTSEL** is reserved for future character switching — do not connect
  anything to it.
- All logic is **3.3V**. Do not connect 5V signals to any GPIO.

---

## Pico Pinout Reference

```
                ┌─────────┐
           GP0  │ 1    40 │  VBUS
           GP1  │ 2    39 │  VSYS
           GND  │ 3    38 │  GND  ←── TFT GND / SD GND
           GP2  │ 4    37 │  3V3_EN
           GP3  │ 5    36 │  3V3   ←── TFT VIN / SD VCC
           GP4  │ 6    35 │  ADC_VREF
           GP5  │ 7    34 │  GP28
           GND  │ 8    33 │  GND
           GP6  │ 9    32 │  GP27
           GP7  │ 10   31 │  GP26
           GP8  │ 11   30 │  RUN
           GP9  │ 12   29 │  GP22  ←── TFT BL
           GND  │ 13   28 │  GND
          GP10  │ 14   27 │  GP21  ←── TFT RST
          GP11  │ 15   26 │  GP20  ←── TFT DC
          GP12  │ 16   25 │  GP19  ←── SPI0 MOSI
          GP13  │ 17   24 │  GP18  ←── SPI0 SCK
           GND  │ 18   23 │  GND
          GP14  │ 19   22 │  GP17  ←── TFT CS
SD CS ──► GP15  │ 20   21 │  GP16  ←── SPI0 MISO
                │         │
                └─────────┘
```
