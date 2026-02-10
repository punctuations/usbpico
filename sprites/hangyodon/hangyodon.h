// Auto-generated from sprites/hangyodon/ -- do not edit by hand
// Re-generate with: python sprites/convert.py sprites/hangyodon
#ifndef SPRITE_HANGYODON_H
#define SPRITE_HANGYODON_H

#include "sprite.h"

// SMALL_1.bmp: 16x16
static const uint8_t sprite_hangyodon_small_1_data[] = {
    0x00, 0x00, 0x02, 0x00, 0x03, 0xf0, 0x07, 0xf8, 0x0f, 0xf8, 0x0f, 0xf8, 0x0d, 0xb8, 0x0f, 0xf8,
    0x1f, 0xf8, 0x07, 0xf8, 0x07, 0xfe, 0x07, 0xf6, 0x07, 0xf0, 0x07, 0xf0, 0x07, 0xf0, 0x0a, 0x58,
};

static const sprite_frame_t sprite_hangyodon_small_frames[] = {
    { 16, 16, sprite_hangyodon_small_1_data },
};

static const sprite_t sprite_hangyodon = {
    .name = "hangyodon",
    .sizes = {
        [SPRITE_SMALL] = { 1, sprite_hangyodon_small_frames },
        [SPRITE_MEDIUM] = { 0, NULL },
        [SPRITE_LARGE] = { 0, NULL },
    },
};

#endif // SPRITE_HANGYODON_H
