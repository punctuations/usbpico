#include "tusb.h"
#include <string.h>

// 64KB RAM disk — RP2040 only has 264KB SRAM total
#define DISK_BLOCK_NUM  128
#define DISK_BLOCK_SIZE 512

static uint8_t disk[DISK_BLOCK_NUM][DISK_BLOCK_SIZE];
static bool ejected = false;

// Pre-format with FAT12 on startup
void disk_init(void) {
    memset(disk, 0, sizeof(disk));

    // Boot sector
    uint8_t *boot = disk[0];
    boot[0] = 0xEB; boot[1] = 0x3C; boot[2] = 0x90;
    memcpy(&boot[3], "MSDOS5.0", 8);
    boot[11] = 0x00; boot[12] = 0x02; // 512 bytes/sector
    boot[13] = 1;                       // 1 sector/cluster
    boot[14] = 1; boot[15] = 0;        // 1 reserved sector
    boot[16] = 1;                       // 1 FAT
    boot[17] = 16; boot[18] = 0;       // 16 root entries
    boot[19] = (DISK_BLOCK_NUM & 0xFF);
    boot[20] = (DISK_BLOCK_NUM >> 8);
    boot[21] = 0xF8;                    // media type
    boot[22] = 1; boot[23] = 0;        // sectors per FAT
    boot[510] = 0x55; boot[511] = 0xAA;

    // FAT
    uint8_t *fat = disk[1];
    fat[0] = 0xF8; fat[1] = 0xFF; fat[2] = 0xFF;

    // Root directory — volume label
    uint8_t *root = disk[2];
    memcpy(root, "USB DRIVE  ", 11);
    root[11] = 0x08; // volume label attribute
}

// How much space is used (for the character!)
float disk_get_usage(void) {
    // Count non-zero clusters in FAT
    uint8_t *fat = disk[1];
    int used = 0;
    int total = DISK_BLOCK_NUM - 3; // minus boot, FAT, root
    for (int i = 2; i < total; i++) {
        // FAT12: each entry is 1.5 bytes
        int off = i + (i / 2);
        if (off + 1 >= DISK_BLOCK_SIZE) break; // stay within FAT sector
        uint16_t val;
        if (i & 1)
            val = (fat[off] >> 4) | (fat[off + 1] << 4);
        else
            val = fat[off] | ((fat[off + 1] & 0x0F) << 8);
        if (val != 0) used++;
    }
    return (total > 0) ? (float)used / (float)total : 0.0f;
}

// TinyUSB MSC callbacks
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8],
                         uint8_t product_id[16], uint8_t product_rev[4]) {
    memcpy(vendor_id, "PICO    ", 8);
    memcpy(product_id, "Character Drive  ", 16);
    memcpy(product_rev, "1.0 ", 4);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    return !ejected;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    *block_count = DISK_BLOCK_NUM;
    *block_size = DISK_BLOCK_SIZE;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           void *buffer, uint32_t bufsize) {
    if (lba >= DISK_BLOCK_NUM) return -1;
    memcpy(buffer, disk[lba] + offset, bufsize);
    return (int32_t)bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                            uint8_t *buffer, uint32_t bufsize) {
    if (lba >= DISK_BLOCK_NUM) return -1;
    memcpy(disk[lba] + offset, buffer, bufsize);
    return (int32_t)bufsize;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16],
                         void *buffer, uint16_t bufsize) {
    int32_t resplen = 0;
    switch (scsi_cmd[0]) {
        case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
            resplen = 0;
            break;
        default:
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            resplen = -1;
            break;
    }
    return resplen;
}