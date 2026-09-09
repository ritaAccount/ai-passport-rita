#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Decode IMA-ADPCM WAV (mono) to PCM s16 into out; returns samples written, 0 on end/error. */
typedef struct {
    const uint8_t *wav;
    size_t wav_size;
    const uint8_t *data;
    size_t data_size;
    size_t data_pos;
    int sample_rate;
    int samples_per_block;
    int block_align;
    int predictor;
    int step_index;
    int samples_in_block;
    int sample_pos_in_block;
    const uint8_t *block;
} slots_ima_wav_t;

bool slots_ima_wav_open(slots_ima_wav_t *st, const uint8_t *wav, size_t wav_size);
/* Fill pcm[] with up to max_samples; returns count (0 = finished). */
int slots_ima_wav_read(slots_ima_wav_t *st, int16_t *pcm, int max_samples);
void slots_ima_wav_rewind(slots_ima_wav_t *st);
