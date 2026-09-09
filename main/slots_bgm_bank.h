#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Theme BGM index: 0=jie 1=p3 2=mari — WAV IMA-ADPCM in music partition. */
bool slots_bgm_bank_ready(void);
const uint8_t *slots_bgm_wav(int theme_idx, size_t *out_size);
