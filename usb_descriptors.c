#include "tusb.h"
#include <string.h>

// Device descriptor — looks like a generic USB drive
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x1209,   // pid.codes open-source VID
    .idProduct = 0x0001,  // pid.codes test PID — register at pid.codes before distributing
    .bcdDevice = 0x0100,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

// String descriptors
char const *string_desc_arr[] = {
    [0] = (const char[]){0x09, 0x04},  // English
    [1] = "USB Drive",
    [2] = "Character Drive",
    [3] = "123456",
};

// Configuration descriptor — MSC only
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_MSC_DESC_LEN)

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_MSC_DESCRIPTOR(0, 0, 0x01, 0x81, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    return desc_configuration;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    static uint16_t desc_str[32];
    uint8_t chr_count;

    if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;

    if (index == 0) {
        memcpy(&desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        const char *str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++)
            desc_str[1 + i] = str[i];
    }

    desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return desc_str;
}