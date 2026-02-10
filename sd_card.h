#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdint.h>
#include <stdbool.h>

// SPI0 pin assignments (Adafruit MicroSD breakout board+)
#define SD_SPI_PORT   spi0
#define SD_PIN_SCK    2   // CLK
#define SD_PIN_MOSI   3   // DI
#define SD_PIN_MISO   4   // DO
#define SD_PIN_CS     5   // CS (manual)
#define SD_PIN_CD     6   // Card Detect (active low, optional)

#define SD_BLOCK_SIZE 512

// Initialize SD card (SPI must already be initialized at 400kHz).
// Returns true on success.
bool sd_init(void);

// Read a single 512-byte block. Returns true on success.
bool sd_read_block(uint32_t lba, uint8_t *buf);

// Write a single 512-byte block. Returns true on success.
bool sd_write_block(uint32_t lba, const uint8_t *buf);

// Get total number of 512-byte blocks on the card. Returns 0 on error.
uint32_t sd_get_block_count(void);

#endif
