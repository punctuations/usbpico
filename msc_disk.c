#include "tusb.h"
#include "sd_card.h"
#include <string.h>

static bool ejected = false;

// Read FAT32 FSInfo sector to estimate disk usage, or return 0 if unavailable.
float disk_get_usage(void) {
    uint32_t total_blocks = sd_get_block_count();
    if (total_blocks == 0) return 0.0f;

    // Read boot sector — extract all needed fields in one pass
    uint8_t buf[SD_BLOCK_SIZE];
    if (!sd_read_block(0, buf)) return 0.0f;

    // FAT32 has sectors_per_FAT16 == 0
    uint16_t sectors_per_fat16 = buf[22] | (buf[23] << 8);
    if (sectors_per_fat16 != 0) return 0.0f; // Not FAT32

    // Save boot sector fields before buf is reused
    uint8_t sectors_per_cluster = buf[13];
    uint16_t reserved = buf[14] | (buf[15] << 8);
    uint8_t num_fats = buf[16];
    uint32_t total_sectors_32 = buf[32] | (buf[33] << 8) |
                                (buf[34] << 16) | (buf[35] << 24);
    uint32_t fat_size_32 = buf[36] | (buf[37] << 8) |
                           (buf[38] << 16) | (buf[39] << 24);
    uint16_t fsinfo_sector = buf[48] | (buf[49] << 8);

    if (fsinfo_sector == 0 || fsinfo_sector == 0xFFFF) return 0.0f;
    if (sectors_per_cluster == 0) return 0.0f;

    // Read FSInfo sector
    if (!sd_read_block(fsinfo_sector, buf)) return 0.0f;

    // Validate FSInfo signatures
    uint32_t sig1 = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    uint32_t sig2 = buf[484] | (buf[485] << 8) | (buf[486] << 16) | (buf[487] << 24);
    if (sig1 != 0x41615252 || sig2 != 0x61417272) return 0.0f;

    // Free cluster count at offset 488
    uint32_t free_clusters = buf[488] | (buf[489] << 8) |
                             (buf[490] << 16) | (buf[491] << 24);
    if (free_clusters == 0xFFFFFFFF) return 0.0f; // Unknown

    // Compute total data clusters from saved boot sector fields
    uint32_t data_sectors = total_sectors_32 - reserved - (num_fats * fat_size_32);
    uint32_t total_clusters = data_sectors / sectors_per_cluster;

    if (total_clusters == 0 || free_clusters > total_clusters) return 0.0f;
    uint32_t used_clusters = total_clusters - free_clusters;
    return (float)used_clusters / (float)total_clusters;
}

// TinyUSB MSC callbacks
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8],
                         uint8_t product_id[16], uint8_t product_rev[4]) {
    memcpy(vendor_id, "PICO    ", 8);
    memcpy(product_id, "Character Drive ", 16);
    memcpy(product_rev, "1.0 ", 4);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    if (sd_get_block_count() == 0) return false; // No card / init failed
    return !ejected;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    *block_count = sd_get_block_count();
    *block_size = SD_BLOCK_SIZE;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                           void *buffer, uint32_t bufsize) {
    // TinyUSB may request partial blocks with offset
    if (offset == 0 && bufsize == SD_BLOCK_SIZE) {
        // Fast path: full block read
        if (!sd_read_block(lba, buffer)) return -1;
        return (int32_t)bufsize;
    }

    // Partial block: read into temp buffer, then copy the requested portion
    uint8_t tmp[SD_BLOCK_SIZE];
    if (!sd_read_block(lba, tmp)) return -1;
    memcpy(buffer, tmp + offset, bufsize);
    return (int32_t)bufsize;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset,
                            uint8_t *buffer, uint32_t bufsize) {
    if (offset == 0 && bufsize == SD_BLOCK_SIZE) {
        if (!sd_write_block(lba, buffer)) return -1;
        return (int32_t)bufsize;
    }

    // Partial block: read-modify-write
    uint8_t tmp[SD_BLOCK_SIZE];
    if (!sd_read_block(lba, tmp)) return -1;
    memcpy(tmp + offset, buffer, bufsize);
    if (!sd_write_block(lba, tmp)) return -1;
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
