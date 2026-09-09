#include "slots_bgm_bank.h"
#include "esp_partition.h"
#include "esp_log.h"

static const char *TAG = "bgm_bank";

#define SLBM_MAGIC 0x4D424C53u /* 'SLBM' little-endian */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t count;
} slbm_hdr_t;

typedef struct {
    uint32_t offset;
    uint32_t size;
} slbm_ent_t;

static const esp_partition_t *s_part;
static const uint8_t *s_map;
static esp_partition_mmap_handle_t s_mmap;
static bool s_ok;

bool slots_bgm_bank_ready(void)
{
    if (s_ok) return true;
    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, "music");
    if (!s_part) {
        ESP_LOGW(TAG, "music partition missing — flash assets/music/slots_music_bank.bin @ 0x35A000");
        return false;
    }
    esp_err_t err = esp_partition_mmap(s_part, 0, s_part->size, ESP_PARTITION_MMAP_DATA,
                                       (const void **)&s_map, &s_mmap);
    if (err != ESP_OK || !s_map) {
        ESP_LOGW(TAG, "mmap music failed (%s)", esp_err_to_name(err));
        return false;
    }
    const slbm_hdr_t *h = (const slbm_hdr_t *)s_map;
    if (h->magic != SLBM_MAGIC || h->version != 1 || h->count < 1) {
        ESP_LOGW(TAG, "bad SLBM header magic=%08x", (unsigned)h->magic);
        return false;
    }
    s_ok = true;
    ESP_LOGI(TAG, "music bank ready, %u tracks in %u KB",
             (unsigned)h->count, (unsigned)(s_part->size / 1024));
    return true;
}

const uint8_t *slots_bgm_wav(int theme_idx, size_t *out_size)
{
    if (out_size) *out_size = 0;
    if (!slots_bgm_bank_ready() || theme_idx < 0) return NULL;
    const slbm_hdr_t *h = (const slbm_hdr_t *)s_map;
    if ((uint32_t)theme_idx >= h->count) return NULL;
    const slbm_ent_t *e = (const slbm_ent_t *)(s_map + sizeof(slbm_hdr_t));
    uint32_t off = e[theme_idx].offset;
    uint32_t sz = e[theme_idx].size;
    if ((size_t)off + (size_t)sz > (size_t)s_part->size) return NULL;
    if (out_size) *out_size = sz;
    return s_map + off;
}
