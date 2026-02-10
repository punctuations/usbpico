// Auto-generated from sprites/djungelskog/ -- do not edit by hand
// Re-generate with: python sprites/convert.py sprites/djungelskog
#ifndef SPRITE_DJUNGELSKOG_H
#define SPRITE_DJUNGELSKOG_H

#include "sprite.h"

// SMALL_1.bmp: 16x16
static const uint8_t sprite_djungelskog_small_1_data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const sprite_frame_t sprite_djungelskog_small_frames[] = {
    { 16, 16, sprite_djungelskog_small_1_data },
};

// LARGE_1.bmp: 16x16
static const uint8_t sprite_djungelskog_large_1_data[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const sprite_frame_t sprite_djungelskog_large_frames[] = {
    { 16, 16, sprite_djungelskog_large_1_data },
};

static const sprite_t sprite_djungelskog = {
    .name = "djungelskog",
    .sizes = {
        [SPRITE_SMALL] = { 1, sprite_djungelskog_small_frames },
        [SPRITE_MEDIUM] = { 0, NULL },
        [SPRITE_LARGE] = { 1, sprite_djungelskog_large_frames },
    },
};

#endif // SPRITE_DJUNGELSKOG_H
