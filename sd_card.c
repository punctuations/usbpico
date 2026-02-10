#include "sd_card.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <string.h>

// SD SPI commands
#define CMD0   0   // GO_IDLE_STATE
#define CMD8   8   // SEND_IF_COND
#define CMD9   9   // SEND_CSD
#define CMD17  17  // READ_SINGLE_BLOCK
#define CMD24  24  // WRITE_BLOCK
#define CMD55  55  // APP_CMD prefix
#define CMD58  58  // READ_OCR
#define ACMD41 41  // SD_SEND_OP_COND (after CMD55)

// R1 response bits
#define R1_IDLE 0x01

static bool sd_is_sdhc = false;
static uint32_t sd_block_count = 0;

static inline void cs_select(void) {
    gpio_put(SD_PIN_CS, 0);
}

static inline void cs_deselect(void) {
    gpio_put(SD_PIN_CS, 1);
    // Send a dummy byte to release DO
    uint8_t dummy = 0xFF;
    spi_write_blocking(SD_SPI_PORT, &dummy, 1);
}

static uint8_t spi_xfer(uint8_t tx) {
    uint8_t rx;
    spi_write_read_blocking(SD_SPI_PORT, &tx, &rx, 1);
    return rx;
}

// Wait until card is not busy (DO goes high)
static bool sd_wait_ready(uint32_t timeout_ms) {
    absolute_time_t deadline = make_timeout_time_ms(timeout_ms);
    while (spi_xfer(0xFF) != 0xFF) {
        if (absolute_time_diff_us(get_absolute_time(), deadline) < 0)
            return false;
    }
    return true;
}

// Send a command and return R1 response byte
static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg) {
    // For ACMD, send CMD55 first
    if (cmd == ACMD41) {
        uint8_t r = sd_send_cmd(CMD55, 0);
        if (r > R1_IDLE) return r;
    }

    cs_deselect();
    cs_select();

    if (!sd_wait_ready(500)) return 0xFF;

    // Command frame: start bit + cmd, 4 bytes arg, CRC + stop bit
    uint8_t frame[6];
    frame[0] = 0x40 | cmd;
    frame[1] = (arg >> 24) & 0xFF;
    frame[2] = (arg >> 16) & 0xFF;
    frame[3] = (arg >> 8) & 0xFF;
    frame[4] = arg & 0xFF;

    // Pre-computed CRCs for the commands that need valid CRC
    if (cmd == CMD0)       frame[5] = 0x95;
    else if (cmd == CMD8)  frame[5] = 0x87;
    else                   frame[5] = 0x01; // dummy CRC, CRC off after init

    spi_write_blocking(SD_SPI_PORT, frame, 6);

    // Wait for response (bit 7 = 0)
    uint8_t r1;
    for (int i = 0; i < 10; i++) {
        r1 = spi_xfer(0xFF);
        if (!(r1 & 0x80)) return r1;
    }
    return 0xFF;
}

// Read CSD register to determine card capacity
static bool sd_read_csd(void) {
    uint8_t r1 = sd_send_cmd(CMD9, 0);
    if (r1 != 0x00) {
        cs_deselect();
        return false;
    }

    // Wait for data token 0xFE
    absolute_time_t deadline = make_timeout_time_ms(500);
    uint8_t token;
    do {
        token = spi_xfer(0xFF);
        if (absolute_time_diff_us(get_absolute_time(), deadline) < 0) {
            cs_deselect();
            return false;
        }
    } while (token == 0xFF);

    if (token != 0xFE) {
        cs_deselect();
        return false;
    }

    // Read 16 bytes of CSD
    uint8_t csd[16];
    for (int i = 0; i < 16; i++)
        csd[i] = spi_xfer(0xFF);

    // Discard 2-byte CRC
    spi_xfer(0xFF);
    spi_xfer(0xFF);

    cs_deselect();

    // Parse capacity
    uint8_t csd_ver = csd[0] >> 6;
    if (csd_ver == 1) {
        // CSD v2 (SDHC/SDXC): C_SIZE is bits [69:48] = csd[7..9]
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) |
                          ((uint32_t)csd[8] << 8) |
                          (uint32_t)csd[9];
        sd_block_count = (c_size + 1) * 1024; // each unit = 512KB
    } else {
        // CSD v1 (SDSC)
        uint8_t read_bl_len = csd[5] & 0x0F;
        uint16_t c_size_v1 = ((uint16_t)(csd[6] & 0x03) << 10) |
                             ((uint16_t)csd[7] << 2) |
                             (csd[8] >> 6);
        uint8_t c_size_mult = ((csd[9] & 0x03) << 1) | (csd[10] >> 7);
        uint32_t capacity_bytes = (uint32_t)(c_size_v1 + 1) *
                                  (1u << (c_size_mult + 2)) *
                                  (1u << read_bl_len);
        sd_block_count = capacity_bytes / SD_BLOCK_SIZE;
    }

    return true;
}

bool sd_init(void) {
    // CS high during power-up
    gpio_put(SD_PIN_CS, 1);

    // SD spec: wait >=1ms after VCC stabilizes
    sleep_ms(2);

    // Send >=74 clock cycles with CS high (card power-up)
    uint8_t dummy[10];
    memset(dummy, 0xFF, sizeof(dummy));
    spi_write_blocking(SD_SPI_PORT, dummy, 10); // 80 clocks

    // CMD0: GO_IDLE_STATE → expect R1_IDLE (0x01)
    uint8_t r1;
    for (int retry = 0; retry < 10; retry++) {
        r1 = sd_send_cmd(CMD0, 0);
        if (r1 == R1_IDLE) break;
    }
    if (r1 != R1_IDLE) {
        cs_deselect();
        return false;
    }

    // CMD8: check for SDv2 (voltage check, 0x1AA pattern)
    r1 = sd_send_cmd(CMD8, 0x000001AA);
    bool sd_v2 = false;
    if (r1 == R1_IDLE) {
        // Read 4-byte R7 response
        uint8_t r7[4];
        for (int i = 0; i < 4; i++) r7[i] = spi_xfer(0xFF);
        // Check echo pattern: should be 0x01AA
        if ((r7[2] & 0x0F) == 0x01 && r7[3] == 0xAA)
            sd_v2 = true;
    }

    cs_deselect();

    // ACMD41: wait for card to leave idle state
    // For SDv2, set HCS bit (bit 30) to indicate we support SDHC
    uint32_t acmd41_arg = sd_v2 ? 0x40000000 : 0;
    absolute_time_t deadline = make_timeout_time_ms(2000);
    do {
        r1 = sd_send_cmd(ACMD41, acmd41_arg);
        cs_deselect();
        if (absolute_time_diff_us(get_absolute_time(), deadline) < 0) {
            return false;
        }
        sleep_ms(10);
    } while (r1 != 0x00);

    // CMD58: read OCR to check SDHC flag
    sd_is_sdhc = false;
    if (sd_v2) {
        r1 = sd_send_cmd(CMD58, 0);
        if (r1 == 0x00) {
            uint8_t ocr[4];
            for (int i = 0; i < 4; i++) ocr[i] = spi_xfer(0xFF);
            // CCS bit (bit 30 of OCR) indicates SDHC/SDXC
            if (ocr[0] & 0x40)
                sd_is_sdhc = true;
        }
        cs_deselect();
    }

    // Speed up SPI now that init is done
    spi_set_baudrate(SD_SPI_PORT, 12 * 1000 * 1000); // 12 MHz

    // Read CSD to get block count
    if (!sd_read_csd())
        return false;

    return true;
}

bool sd_read_block(uint32_t lba, uint8_t *buf) {
    // SDSC uses byte address; SDHC uses block address
    uint32_t addr = sd_is_sdhc ? lba : lba * SD_BLOCK_SIZE;

    uint8_t r1 = sd_send_cmd(CMD17, addr);
    if (r1 != 0x00) {
        cs_deselect();
        return false;
    }

    // Wait for data token 0xFE
    absolute_time_t deadline = make_timeout_time_ms(500);
    uint8_t token;
    do {
        token = spi_xfer(0xFF);
        if (absolute_time_diff_us(get_absolute_time(), deadline) < 0) {
            cs_deselect();
            return false;
        }
    } while (token == 0xFF);

    if (token != 0xFE) {
        cs_deselect();
        return false;
    }

    // Read 512 bytes of data
    spi_read_blocking(SD_SPI_PORT, 0xFF, buf, SD_BLOCK_SIZE);

    // Discard 2-byte CRC
    spi_xfer(0xFF);
    spi_xfer(0xFF);

    cs_deselect();
    return true;
}

bool sd_write_block(uint32_t lba, const uint8_t *buf) {
    uint32_t addr = sd_is_sdhc ? lba : lba * SD_BLOCK_SIZE;

    uint8_t r1 = sd_send_cmd(CMD24, addr);
    if (r1 != 0x00) {
        cs_deselect();
        return false;
    }

    if (!sd_wait_ready(500)) {
        cs_deselect();
        return false;
    }

    // Send data token
    uint8_t token = 0xFE;
    spi_write_blocking(SD_SPI_PORT, &token, 1);

    // Send 512 bytes of data
    spi_write_blocking(SD_SPI_PORT, buf, SD_BLOCK_SIZE);

    // Send dummy CRC
    spi_xfer(0xFF);
    spi_xfer(0xFF);

    // Read data response token: xxx0sss1
    // sss: 010 = accepted, 101 = CRC error, 110 = write error
    uint8_t resp = spi_xfer(0xFF);
    if ((resp & 0x1F) != 0x05) {
        cs_deselect();
        return false;
    }

    // Wait for card to finish programming
    if (!sd_wait_ready(1000)) {
        cs_deselect();
        return false;
    }

    cs_deselect();
    return true;
}

uint32_t sd_get_block_count(void) {
    return sd_block_count;
}
