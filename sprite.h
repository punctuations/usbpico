#ifndef SPRITE_H
#define SPRITE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t width;
    uint8_t height;
    const uint8_t *data; // row-major, 1bpp, MSB first, rows padded to byte boundary
} sprite_frame_t;

typedef enum {
    SPRITE_SMALL = 0,
    SPRITE_MEDIUM,
    SPRITE_LARGE,
    SPRITE_SIZE_COUNT
} sprite_size_t;

typedef struct {
    uint8_t num_frames;
    const sprite_frame_t *frames; // array of frames for animation
} sprite_anim_t;

typedef struct {
    const char *name;
    sprite_anim_t sizes[SPRITE_SIZE_COUNT];
} sprite_t;

#endif
