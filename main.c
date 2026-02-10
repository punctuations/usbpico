#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "sd_card.h"
#include "sprite.h"
#include "sprites/djungelskog/djungelskog.h"

// External functions
extern float disk_get_usage(void);

static const sprite_t *current_sprite = &sprite_djungelskog;

// Simple SSD1306 commands (minimal driver)
#define SSD1306_ADDR 0x3C
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define I2C_TIMEOUT_US 10000

static uint8_t framebuf[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

void ssd1306_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking_until(i2c0, SSD1306_ADDR, buf, 2, false,
                             make_timeout_time_us(I2C_TIMEOUT_US));
}

void ssd1306_init(void) {
    ssd1306_cmd(0xAE); // off
    ssd1306_cmd(0xD5); ssd1306_cmd(0x80);
    ssd1306_cmd(0xA8); ssd1306_cmd(0x3F);
    ssd1306_cmd(0xD3); ssd1306_cmd(0x00);
    ssd1306_cmd(0x40);
    ssd1306_cmd(0x8D); ssd1306_cmd(0x14); // charge pump
    ssd1306_cmd(0x20); ssd1306_cmd(0x00); // horizontal mode
    ssd1306_cmd(0xA1); ssd1306_cmd(0xC8);
    ssd1306_cmd(0xDA); ssd1306_cmd(0x12);
    ssd1306_cmd(0x81); ssd1306_cmd(0xCF);
    ssd1306_cmd(0xD9); ssd1306_cmd(0xF1);
    ssd1306_cmd(0xDB); ssd1306_cmd(0x40);
    ssd1306_cmd(0xA4); ssd1306_cmd(0xA6);
    ssd1306_cmd(0xAF); // on
}

void ssd1306_update(void) {
    ssd1306_cmd(0x21); ssd1306_cmd(0); ssd1306_cmd(127);
    ssd1306_cmd(0x22); ssd1306_cmd(0); ssd1306_cmd(7);

    uint8_t buf[SSD1306_WIDTH + 1];
    buf[0] = 0x40; // data mode
    for (int page = 0; page < 8; page++) {
        memcpy(&buf[1], &framebuf[page * SSD1306_WIDTH], SSD1306_WIDTH);
        i2c_write_blocking_until(i2c0, SSD1306_ADDR, buf, SSD1306_WIDTH + 1, false,
                                     make_timeout_time_us(I2C_TIMEOUT_US * 10));
    }
}

void set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    if (on) framebuf[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y % 8));
    else    framebuf[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
}

void blit_sprite(const sprite_frame_t *frame) {
    int ox = (SSD1306_WIDTH - frame->width) / 2;
    int oy = SSD1306_HEIGHT - frame->height; // bottom-aligned
    int row_bytes = (frame->width + 7) / 8;

    for (int y = 0; y < frame->height; y++) {
        for (int x = 0; x < frame->width; x++) {
            int byte_idx = y * row_bytes + x / 8;
            bool on = frame->data[byte_idx] & (0x80 >> (x % 8));
            if (on) set_pixel(ox + x, oy + y, true);
        }
    }
}

static uint8_t anim_frame = 0;

void draw_character(float percent) {
    memset(framebuf, 0, sizeof(framebuf));

    // Pick size based on disk usage
    sprite_size_t size;
    if (percent < 0.33f)      size = SPRITE_SMALL;
    else if (percent < 0.66f) size = SPRITE_MEDIUM;
    else                      size = SPRITE_LARGE;

    const sprite_anim_t *anim = &current_sprite->sizes[size];

    // Fall back to nearest available size if this one has no frames
    if (anim->num_frames == 0) {
        for (int i = 0; i < SPRITE_SIZE_COUNT; i++) {
            if (current_sprite->sizes[i].num_frames > 0) {
                anim = &current_sprite->sizes[i];
                break;
            }
        }
    }

    if (anim->num_frames > 0) {
        uint8_t idx = anim_frame % anim->num_frames;
        blit_sprite(&anim->frames[idx]);
        anim_frame++;
    }

    ssd1306_update();
}

int main(void) {
    stdio_init_all();

    // SPI for SD card (400kHz during init, sd_init() speeds up after)
    spi_init(SD_SPI_PORT, 400 * 1000);
    gpio_set_function(SD_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SD_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SD_PIN_MISO, GPIO_FUNC_SPI);
    gpio_pull_up(SD_PIN_MISO); // Keep MISO defined when SD card releases DO

    // CS pin: manual GPIO output, active low
    gpio_init(SD_PIN_CS);
    gpio_set_dir(SD_PIN_CS, GPIO_OUT);
    gpio_put(SD_PIN_CS, 1);

    // Card detect pin: input with pull-up (active low)
    gpio_init(SD_PIN_CD);
    gpio_set_dir(SD_PIN_CD, GPIO_IN);
    gpio_pull_up(SD_PIN_CD);

    bool sd_ok = sd_init();
    tusb_init();

    // I2C for display
    i2c_init(i2c0, 400000);
    gpio_set_function(0, GPIO_FUNC_I2C); // SDA
    gpio_set_function(1, GPIO_FUNC_I2C); // SCL
    gpio_pull_up(0);
    gpio_pull_up(1);
    ssd1306_init();

    uint32_t last_draw = 0;

    while (1) {
        tud_task(); // TinyUSB task — MUST be called frequently

        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_draw > 2000) {
            float usage = disk_get_usage();
            draw_character(usage);
            last_draw = now;
        }
    }
}