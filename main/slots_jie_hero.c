#include "slots_jie_hero.h"

extern const uint8_t slots_jie_hero_rgb565_start[] asm("_binary_slots_jie_hero_rgb565_start");
extern const uint8_t slots_jie_hero2_rgb565_start[] asm("_binary_slots_jie_hero2_rgb565_start");
extern const uint8_t slots_jie_hero3_rgb565_start[] asm("_binary_slots_jie_hero3_rgb565_start");
extern const uint8_t slots_jie_hero4_rgb565_start[] asm("_binary_slots_jie_hero4_rgb565_start");

#define HERO_IMG(name, sym) \
const lv_image_dsc_t name = { \
    .header = { \
        .magic = LV_IMAGE_HEADER_MAGIC, \
        .cf = LV_COLOR_FORMAT_RGB565, \
        .flags = 0, \
        .w = 240, \
        .h = 320, \
        .stride = 240 * 2, \
        .reserved_2 = 0, \
    }, \
    .data_size = 240 * 320 * 2, \
    .data = sym, \
    .reserved = NULL, \
    .reserved_2 = NULL, \
}

HERO_IMG(slots_jie_hero, slots_jie_hero_rgb565_start);
HERO_IMG(slots_jie_hero2, slots_jie_hero2_rgb565_start);
HERO_IMG(slots_jie_hero3, slots_jie_hero3_rgb565_start);
HERO_IMG(slots_jie_hero4, slots_jie_hero4_rgb565_start);
