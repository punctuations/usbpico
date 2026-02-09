# usbpico

Raspberry Pi Pico (RP2040) projects using the Pico SDK.

## Prerequisites

- [CMake](https://cmake.org/download/) (3.13+)
- [Ninja](https://ninja-build.org/)
- Python 3
- ARM GCC cross-compiler (`arm-none-eabi-gcc`)

Clone this repo with the SDK submodule, then init TinyUSB (needed for USB serial):

```bash
git submodule update --init --recursive
cd pico-sdk && git submodule update --init lib/tinyusb && cd ..
```

## Building on Windows (Git Bash)

Install the ARM toolchain:

```bash
winget install Arm.ArmGnuToolchain
```

Then build:

```bash
export PATH="/c/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/12.2 MPACBTI-Rel1/bin:$PATH"
mkdir -p build && cd build
PICO_SDK_PATH="$(cd ../pico-sdk && pwd)" cmake -G Ninja -DCMAKE_MAKE_PROGRAM="/c/Strawberry/c/bin/ninja.exe" ..
ninja
```

## Building on WSL / Linux

Install dependencies:

```bash
sudo apt update
sudo apt install -y cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential ninja-build python3
```

Then build (use a separate build directory to avoid conflicts with Windows builds):

```bash
mkdir -p build-wsl && cd build-wsl
PICO_SDK_PATH="$(cd ../pico-sdk && pwd)" cmake -G Ninja ..
ninja
```

## Flashing

1. Hold the **BOOTSEL** button on the Pico
2. Plug it in via USB — a `RPI-RP2` drive will appear
3. Drag a `.uf2` file from the build directory onto the drive

## Targets

| Target | Source | Description |
|---|---|---|
| `usb_character` | `main.c`, `msc_disk.c`, `usb_descriptors.c` | USB mass storage drive with SSD1306 OLED display — shows a pixel art character that changes based on disk usage |
| `blink` | `blink.c` | Blinks the onboard LED every 500ms |

### usb_character wiring

The `usb_character` target drives an SSD1306 128x64 OLED over I2C:

| Pico Pin | Function |
|---|---|
| GPIO 0 | SDA |
| GPIO 1 | SCL |

## Sprites

Character sprites live in `sprites/<character_name>/` as BMP files.

### File naming

Files are named `<SIZE>_<FRAME>.bmp`:

```
sprites/djungelskog/
  SMALL_1.bmp       # small state, frame 1
  SMALL_2.bmp       # small state, frame 2 (animation)
  MEDIUM_1.bmp
  LARGE_1.bmp
  LARGE_2.bmp
```

Sizes map to disk usage: **SMALL** (<33%), **MEDIUM** (33-66%), **LARGE** (>66%).
Multiple frames per size will animate, cycling every 2 seconds.
Missing sizes fall back to the nearest available one.

### Converting BMPs to C headers

```bash
pip install Pillow
python sprites/convert.py sprites/djungelskog
```

This generates `sprites/djungelskog/djungelskog.h` which gets `#include`d by `main.c`.

### Bitmap format

1-bit monochrome, row-major, MSB = leftmost pixel, each row padded to a byte boundary.
White pixels (`1`) are drawn on the display; black (`0`) is transparent.
Sprites are centered horizontally and aligned to the bottom of the 128x64 display.

### Adding a new character

1. Create `sprites/newname/`
2. Add your BMP files (`SMALL_1.bmp`, `MEDIUM_1.bmp`, `LARGE_1.bmp`, etc.)
3. Run `python sprites/convert.py sprites/newname`
4. In `main.c`, change the `#include` and `current_sprite` pointer