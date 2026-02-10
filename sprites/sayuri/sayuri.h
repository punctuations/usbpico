// Auto-generated from sprites/sayuri/ -- do not edit by hand
// Re-generate with: python sprites/convert.py sprites/sayuri
#ifndef SPRITE_SAYURI_H
#define SPRITE_SAYURI_H

#include "sprite.h"

// SMALL_1.bmp: 16x16
static const uint8_t sprite_sayuri_small_1_data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x0d, 0xf0, 0x0a, 0x90, 0x0f, 0xf0,
    0x0f, 0xf0, 0x03, 0xc0, 0x07, 0xe0, 0x0d, 0x50, 0x09, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const sprite_frame_t sprite_sayuri_small_frames[] = {
    { 16, 16, sprite_sayuri_small_1_data },
};

static const sprite_t sprite_sayuri = {
    .name = "sayuri",
    .sizes = {
        [SPRITE_SMALL] = { 1, sprite_sayuri_small_frames },
        [SPRITE_MEDIUM] = { 0, NULL },
        [SPRITE_LARGE] = { 0, NULL },
    },
};

#endif // SPRITE_SAYURI_H
