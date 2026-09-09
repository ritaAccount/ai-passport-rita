#include "slots_ima_wav.h"
#include <string.h>

static const int IMA_STEP[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
};
static const int IMA_INDEX[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8
};

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int ima_decode_nibble(slots_ima_wav_t *st, int nibble)
{
    int step = IMA_STEP[st->step_index];
    int diff = step >> 3;
    if (nibble & 1) diff += step >> 2;
    if (nibble & 2) diff += step >> 1;
    if (nibble & 4) diff += step;
    if (nibble & 8) st->predictor -= diff;
    else st->predictor += diff;
    if (st->predictor > 32767) st->predictor = 32767;
    if (st->predictor < -32768) st->predictor = -32768;
    st->step_index += IMA_INDEX[nibble];
    if (st->step_index < 0) st->step_index = 0;
    if (st->step_index > 88) st->step_index = 88;
    return st->predictor;
}

bool slots_ima_wav_open(slots_ima_wav_t *st, const uint8_t *wav, size_t wav_size)
{
    memset(st, 0, sizeof(*st));
    if (!wav || wav_size < 44) return false;
    if (memcmp(wav, "RIFF", 4) != 0 || memcmp(wav + 8, "WAVE", 4) != 0) return false;
    st->wav = wav;
    st->wav_size = wav_size;

    size_t pos = 12;
    int audio_format = 0, channels = 0, bits = 0;
    while (pos + 8 <= wav_size) {
        const uint8_t *ch = wav + pos;
        uint32_t id = rd32(ch);
        uint32_t sz = rd32(ch + 4);
        pos += 8;
        if (pos + sz > wav_size) break;
        if (id == 0x20746d66u) { /* 'fmt ' */
            if (sz < 16) return false;
            audio_format = rd16(ch + 8);
            channels = rd16(ch + 10);
            st->sample_rate = (int)rd32(ch + 12);
            st->block_align = rd16(ch + 20);
            bits = rd16(ch + 22);
            if (sz >= 20) st->samples_per_block = rd16(ch + 26);
        } else if (id == 0x61746164u) { /* 'data' */
            st->data = ch + 8;
            st->data_size = sz;
        }
        pos += (sz + 1u) & ~1u;
    }
    /* 0x0011 = IMA ADPCM */
    if (audio_format != 0x0011 || channels != 1 || !st->data || st->block_align < 4) return false;
    if (st->samples_per_block <= 0) {
        /* mono IMA: samples_per_block = (block_align - 4) * 2 + 1 */
        st->samples_per_block = (st->block_align - 4) * 2 + 1;
    }
    slots_ima_wav_rewind(st);
    return true;
}

void slots_ima_wav_rewind(slots_ima_wav_t *st)
{
    st->data_pos = 0;
    st->samples_in_block = 0;
    st->sample_pos_in_block = 0;
    st->block = NULL;
    st->predictor = 0;
    st->step_index = 0;
}

static bool load_block(slots_ima_wav_t *st)
{
    if (st->data_pos + (size_t)st->block_align > st->data_size) return false;
    st->block = st->data + st->data_pos;
    st->data_pos += (size_t)st->block_align;
    st->predictor = (int16_t)rd16(st->block);
    st->step_index = st->block[2];
    if (st->step_index > 88) st->step_index = 88;
    st->samples_in_block = st->samples_per_block;
    st->sample_pos_in_block = 0;
    return true;
}

int slots_ima_wav_read(slots_ima_wav_t *st, int16_t *pcm, int max_samples)
{
    int n = 0;
    while (n < max_samples) {
        if (st->sample_pos_in_block >= st->samples_in_block) {
            if (!load_block(st)) break;
            /* first sample is predictor */
            pcm[n++] = (int16_t)st->predictor;
            st->sample_pos_in_block = 1;
            continue;
        }
        int nib_index = st->sample_pos_in_block - 1;
        const uint8_t *nibbles = st->block + 4;
        int byte_i = nib_index / 2;
        int nibble = nibbles[byte_i];
        if ((nib_index & 1) == 0) nibble &= 0x0f;
        else nibble >>= 4;
        pcm[n++] = (int16_t)ima_decode_nibble(st, nibble);
        st->sample_pos_in_block++;
    }
    return n;
}
