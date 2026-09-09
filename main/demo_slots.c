// main/demo_slots.c —— 老虎机：JIE / P3 / MARI 三主题（resource 立绘 + PCM 循环 BGM）。
#include "demo.h"
#include "slot_logic.h"
#include "slots_jie_hero.h"
#include "slots_p3_hero.h"
#include "slots_mari_hero.h"
#include "slots_bgm_bank.h"
#include "slots_ima_wav.h"
#include "bsp_battery.h"
#include "bsp_audio.h"

#include "esp_random.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

/* Fallback; each theme overrides via theme_style_t.sym_text */
static const char *SYM_TEXT_FALLBACK[SLOT_SYM_COUNT] = { "CH", "LM", "BL", "ST", "7" };
static const char *SYM_JIE[SLOT_SYM_COUNT]  = { "ORNG", "STAFF", "WING", "STAR", "777" };
static const char *SYM_P3[SLOT_SYM_COUNT]   = { "MOON", "CARD", "SEES", "STAR", "777" };
static const char *SYM_MARI[SLOT_SYM_COUNT] = { "CHRY", "BAR", "EVA", "UNIT", "777" };
static const int BETS[] = { 1, 5, 10, 25 };
#define BET_COUNT ((int)(sizeof(BETS) / sizeof(BETS[0])))
#define PAT_TILES 20


#define SAMPLE_RATE     16000
#define BGM_SAMPLE_RATE 8000
#define CHUNK_SAMPLES   256
#define FREE_SPIN_AWARD 3
#define NVS_NS        "slots"
#define NVS_KEY_BEST  "best_win"
#define NVS_KEY_PEAK  "peak_cr"
#define NVS_KEY_VOL   "vol"
#define NVS_KEY_THEME "theme"

typedef enum {
    SLOT_IDLE = 0,
    SLOT_SPINNING,
    SLOT_CELEBRATE,
    SLOT_RESULT,
} slot_phase_t;

typedef enum {
    THEME_JIE = 0,
    THEME_P3,
    THEME_MARI,
    THEME_COUNT
} theme_id_t;

typedef enum {
    CFG_BET = 0,
    CFG_VOL,
    CFG_BGM,
    CFG_COUNT
} cfg_focus_t;

typedef enum {
    SFX_NONE = 0,
    SFX_TICK,
    SFX_STOP,
    SFX_WIN,
    SFX_JACKPOT,
    SFX_FREE,
} sfx_t;

typedef struct {
    const lv_image_dsc_t *const *heroes;
    int hero_count;
    const char *title;
    const char *tag;
    const char *ribbon;                 /* 二次元角标，如 EVA / ORANGE / P3R */
    const char *const *sym_text;        /* 花哨符号文案 */
    uint32_t accent;
    uint32_t glow;                      /* 外圈霓虹 */
    uint32_t panel;
    uint32_t reel;
    uint32_t reel_hi;                   /* 转轴高光底 */
    uint32_t border;
    uint32_t ink;
    uint32_t status;
    uint32_t title_bg;
    uint32_t title_fg;
    uint32_t pat_a;                     /* 主题色块图案 A/B（代替人物立绘） */
    uint32_t pat_b;
    uint32_t flash_win;
    uint32_t flash_jackpot;
    uint32_t flash_free;
    uint32_t sym[SLOT_SYM_COUNT];
} theme_style_t;

static const lv_image_dsc_t *const JIE_HEROES[] = { &slots_jie_hero, &slots_jie_hero2, &slots_jie_hero3 };
static const lv_image_dsc_t *const P3_HEROES[] = { &slots_p3_hero, &slots_p3_hero2, &slots_p3_hero3 };
static const lv_image_dsc_t *const MARI_HEROES[] = { &slots_mari_hero, &slots_mari_hero2, &slots_mari_hero3 };

static const theme_style_t THEMES[THEME_COUNT] = {
    [THEME_JIE] = {
        .heroes = JIE_HEROES,
        .hero_count = 3,
        .title = "JIE",
        .tag = "JIE",
        .ribbon = "ORANGE",
        .sym_text = SYM_JIE,
        .accent = 0xFF3B30,
        .glow = 0xFF8A65,
        .panel = 0xFFF3E0,
        .reel = 0x3E1410,
        .reel_hi = 0xFFE0B2,
        .border = 0x212121,
        .ink = 0x212121,
        .status = 0xD32F2F,
        .title_bg = 0xE53935,
        .title_fg = 0xFFFFFF,
        .pat_a = 0xFF7043,
        .pat_b = 0xFFCC80,
        .flash_win = 0xE53935,
        .flash_jackpot = 0xFFD54F,
        .flash_free = 0x4FC3F7,
        .sym = { 0xFF7043, 0xFFD54F, 0xFFFFFF, 0x4FC3F7, 0xFF1744 },
    },
    [THEME_P3] = {
        .heroes = P3_HEROES,
        .hero_count = 3,
        .title = "P3R",
        .tag = "P3",
        .ribbon = "MOON",
        .sym_text = SYM_P3,
        .accent = 0x00B0FF,
        .glow = 0x4FC3F7,
        .panel = 0x0B1C3A,
        .reel = 0x081428,
        .reel_hi = 0x163A66,
        .border = 0xE8F4FF,
        .ink = 0xE8F4FF,
        .status = 0x4FC3F7,
        .title_bg = 0x003D7A,
        .title_fg = 0xFFFFFF,
        .pat_a = 0x1565C0,
        .pat_b = 0x00E5FF,
        .flash_win = 0x00AEEF,
        .flash_jackpot = 0xFFFFFF,
        .flash_free = 0xD70000,
        .sym = { 0x4FC3F7, 0xFFFFFF, 0x82B1FF, 0xFF5252, 0x00E5FF },
    },
    [THEME_MARI] = {
        .heroes = MARI_HEROES,
        .hero_count = 3,
        .title = "MARI",
        .tag = "MARI",
        .ribbon = "EVA",
        .sym_text = SYM_MARI,
        .accent = 0xFF2D95,
        .glow = 0xFF80AB,
        .panel = 0x2A0A1C,
        .reel = 0x1A0410,
        .reel_hi = 0xFFD6E8,
        .border = 0xFFFFFF,
        .ink = 0xFFE6F2,
        .status = 0xFF80C0,
        .title_bg = 0xFF1493,
        .title_fg = 0xFFFFFF,
        .pat_a = 0xC2185B,
        .pat_b = 0xF8BBD0,
        .flash_win = 0xFF2D95,
        .flash_jackpot = 0xFFFFFF,
        .flash_free = 0xE53935,
        .sym = { 0xFF5252, 0xFFFFFF, 0xFF2D95, 0xCE93D8, 0xFF1744 },
    },
};

static lv_obj_t *s_scr;
static lv_obj_t *s_hero;
static lv_obj_t *s_vol_hint;
static lv_obj_t *s_ui_layer;
static lv_obj_t *s_title_shadow;
static lv_obj_t *s_title_plate;
static lv_obj_t *s_title_lbl;
static lv_obj_t *s_ribbon;
static lv_obj_t *s_ribbon_lbl;
static lv_obj_t *s_panel_shadow;
static lv_obj_t *s_panel_glow;
static lv_obj_t *s_panel_outline;
static lv_obj_t *s_panel;
static lv_obj_t *s_reel_shadow[3];
static lv_obj_t *s_reel_cell[3];
static lv_obj_t *s_reel_lbl[3];
static lv_obj_t *s_status_bar;
static lv_obj_t *s_status;
static lv_obj_t *s_banner;
static lv_obj_t *s_cfg_shadow;
static lv_obj_t *s_cfg_panel;
static lv_obj_t *s_cfg_label;
static lv_obj_t *s_batt;
static lv_obj_t *s_pat[PAT_TILES];
static lv_timer_t *s_timer;
static lv_timer_t *s_batt_timer;
static lv_timer_t *s_vol_timer;

static slot_phase_t s_phase;
static bool s_ui_shown;
static theme_id_t s_theme = THEME_JIE;
static int s_hero_idx;
static int s_credits = 100;
static int s_bet_idx;
static int s_volume = 80;
static int s_cfg_focus = CFG_BET;
static int s_reels[3];
static int s_target[3];
static int s_spin_tick;
static int s_stop_at[3];
static int s_last_win;
static int s_best_win;
static int s_peak_credits = 100;
static int s_free_left;
static int s_cele_tick;
static bool s_cele_jackpot;
static bool s_cele_free;
static bool s_audio_ok;
static TaskHandle_t s_sfx_task;
static TaskHandle_t s_bgm_task;
static SemaphoreHandle_t s_audio_mu;
static volatile sfx_t s_sfx_req;
static volatile bool s_bgm_on;
static volatile theme_id_t s_bgm_theme;

static void refresh_status(void);
static void refresh_config(void);
static void set_reel(int i, int sym);
static void queue_sfx(sfx_t s);
static void maybe_save_records(void);
static void load_records(void);
static void apply_volume(void);
static void apply_theme(void);
static void save_theme(void);
static void save_volume(void);
static void start_spin(void);
static void hide_to_splash(void);
static void show_volume_hint(void);
static void toggle_bgm(void);
static void cycle_bgm(void);
static void show_bgm_hint(void);

static const theme_style_t *cur_theme(void)
{
    return &THEMES[s_theme];
}

static lv_obj_t *ui_block(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    return obj;
}

static lv_obj_t *make_panel(lv_obj_t *parent, int x, int y, int w, int h,
                            uint32_t fill, uint32_t border, uint32_t accent,
                            lv_obj_t **shadow_out)
{
    lv_obj_t *shadow = ui_block(parent, x + 4, y + 5, w, h, accent);
    lv_obj_t *panel = ui_block(parent, x, y, w, h, fill);
    lv_obj_set_style_border_color(panel, lv_color_hex(border), 0);
    lv_obj_set_style_border_width(panel, 3, 0);
    lv_obj_set_style_pad_all(panel, 4, 0);
    if (shadow_out) *shadow_out = shadow;
    return panel;
}

/* 像素风粗描边块：黑底 + 白边 + 主题填充（二次元贴纸感） */
static lv_obj_t *make_pixel_chip(lv_obj_t *parent, int x, int y, int w, int h,
                                 uint32_t fill, uint32_t edge)
{
    lv_obj_t *blk = ui_block(parent, x, y, w, h, 0x000000);
    lv_obj_set_style_border_color(blk, lv_color_hex(edge), 0);
    lv_obj_set_style_border_width(blk, 2, 0);
    lv_obj_set_style_bg_color(blk, lv_color_hex(fill), 0);
    return blk;
}

static void set_reel(int i, int sym)
{
    if (sym < 0 || sym >= SLOT_SYM_COUNT) sym = 0;
    s_reels[i] = sym;
    if (s_reel_lbl[i]) {
        const theme_style_t *t = cur_theme();
        const char *const *txt = t->sym_text ? t->sym_text : SYM_TEXT_FALLBACK;
        lv_label_set_text(s_reel_lbl[i], txt[sym]);
        lv_obj_set_style_text_color(s_reel_lbl[i], lv_color_hex(t->sym[sym]), 0);
    }
}

static void set_cells_flash(bool on, uint32_t color)
{
    for (int i = 0; i < 3; i++) {
        if (!s_reel_cell[i]) continue;
        lv_obj_set_style_bg_color(s_reel_cell[i],
                                  lv_color_hex(on ? color : cur_theme()->reel_hi), 0);
    }
}

static void apply_theme(void)
{
    const theme_style_t *t = cur_theme();
    if (s_hero_idx < 0 || s_hero_idx >= t->hero_count) s_hero_idx = 0;
    if (s_hero && t->heroes && t->hero_count > 0) {
        lv_image_set_src(s_hero, NULL);
        lv_image_set_src(s_hero, t->heroes[s_hero_idx]);
        lv_obj_invalidate(s_hero);
        if (s_scr) lv_obj_invalidate(s_scr);
    }
    if (s_title_shadow) lv_obj_set_style_bg_color(s_title_shadow, lv_color_hex(0x000000), 0);
    if (s_title_plate) {
        lv_obj_set_style_bg_color(s_title_plate, lv_color_hex(t->title_bg), 0);
        lv_obj_set_style_border_color(s_title_plate, lv_color_hex(t->border), 0);
    }
    if (s_title_lbl) {
        lv_label_set_text(s_title_lbl, t->title);
        lv_obj_set_style_text_color(s_title_lbl, lv_color_hex(t->title_fg), 0);
    }
    if (s_ribbon) {
        lv_obj_set_style_bg_color(s_ribbon, lv_color_hex(t->accent), 0);
        lv_obj_set_style_border_color(s_ribbon, lv_color_hex(t->border), 0);
    }
    if (s_ribbon_lbl) {
        lv_label_set_text(s_ribbon_lbl, t->ribbon);
        lv_obj_set_style_text_color(s_ribbon_lbl, lv_color_hex(0xFFFFFF), 0);
    }
    if (s_panel_glow) lv_obj_set_style_bg_color(s_panel_glow, lv_color_hex(t->glow), 0);
    if (s_panel_outline) lv_obj_set_style_bg_color(s_panel_outline, lv_color_hex(0x000000), 0);
    if (s_panel_shadow) lv_obj_set_style_bg_color(s_panel_shadow, lv_color_hex(t->accent), 0);
    if (s_panel) {
        lv_obj_set_style_bg_color(s_panel, lv_color_hex(t->panel), 0);
        lv_obj_set_style_border_color(s_panel, lv_color_hex(t->accent), 0);
    }
    if (s_status_bar) {
        lv_obj_set_style_bg_color(s_status_bar, lv_color_hex(t->reel), 0);
        lv_obj_set_style_border_color(s_status_bar, lv_color_hex(t->accent), 0);
    }
    if (s_status) lv_obj_set_style_text_color(s_status, lv_color_hex(t->status), 0);
    if (s_banner) lv_obj_set_style_text_color(s_banner, lv_color_hex(t->flash_jackpot), 0);
    if (s_batt) lv_obj_set_style_text_color(s_batt, lv_color_hex(t->ink), 0);
    if (s_cfg_shadow) lv_obj_set_style_bg_color(s_cfg_shadow, lv_color_hex(t->accent), 0);
    if (s_cfg_panel) {
        lv_obj_set_style_bg_color(s_cfg_panel, lv_color_hex(t->panel), 0);
        lv_obj_set_style_border_color(s_cfg_panel, lv_color_hex(t->accent), 0);
    }
    if (s_cfg_label) lv_obj_set_style_text_color(s_cfg_label, lv_color_hex(t->ink), 0);
    if (s_vol_hint) lv_obj_set_style_text_color(s_vol_hint, lv_color_hex(t->accent), 0);
    for (int i = 0; i < PAT_TILES; i++) {
        if (!s_pat[i]) continue;
        lv_obj_set_style_bg_color(s_pat[i],
                                  lv_color_hex((i & 1) ? t->pat_b : t->pat_a), 0);
        lv_obj_set_style_border_color(s_pat[i], lv_color_hex(t->border), 0);
    }
    for (int i = 0; i < 3; i++) {
        if (s_reel_shadow[i]) {
            lv_obj_set_style_bg_color(s_reel_shadow[i], lv_color_hex(0x000000), 0);
        }
        if (s_reel_cell[i]) {
            lv_obj_set_style_bg_color(s_reel_cell[i], lv_color_hex(t->reel_hi), 0);
            lv_obj_set_style_border_color(s_reel_cell[i], lv_color_hex(t->accent), 0);
        }
        set_reel(i, s_reels[i]);
    }
    refresh_config();
    s_bgm_theme = s_theme;
}

static void hide_to_splash(void)
{
    s_ui_shown = false;
    s_phase = SLOT_IDLE;
    if (s_banner) lv_obj_add_flag(s_banner, LV_OBJ_FLAG_HIDDEN);
    set_cells_flash(false, 0);
    if (s_ui_layer) lv_obj_add_flag(s_ui_layer, LV_OBJ_FLAG_HIDDEN);
}

static void cycle_theme(int dir)
{
    /* 主页上下：先翻同一主题立绘，翻完再切下一主题 */
    const theme_style_t *t = cur_theme();
    int next = s_hero_idx + dir;
    if (next >= 0 && next < t->hero_count) {
        s_hero_idx = next;
        apply_theme();
        return;
    }
    s_theme = (theme_id_t)((s_theme + dir + THEME_COUNT) % THEME_COUNT);
    t = cur_theme();
    s_hero_idx = (dir > 0) ? 0 : (t->hero_count - 1);
    if (s_hero_idx < 0) s_hero_idx = 0;
    apply_theme();
    save_theme();
}

static void refresh_status(void)
{
    if (!s_status) return;
    const int bet = BETS[s_bet_idx];
    if (s_phase == SLOT_SPINNING) {
        if (s_free_left > 0) {
            lv_label_set_text_fmt(s_status,
                                  "Credits %d  FREE x%d\nSPINNING...  Best %d",
                                  s_credits, s_free_left, s_best_win);
        } else {
            lv_label_set_text_fmt(s_status,
                                  "Credits %d  Bet %d\nSPINNING...  Best %d",
                                  s_credits, bet, s_best_win);
        }
    } else if (s_phase == SLOT_CELEBRATE) {
        if (s_cele_free) {
            lv_label_set_text_fmt(s_status, "Credits %d\nFREE SPINS +%d!",
                                  s_credits, FREE_SPIN_AWARD);
        } else if (s_cele_jackpot) {
            lv_label_set_text_fmt(s_status, "Credits %d\nJACKPOT +%d!",
                                  s_credits, s_last_win);
        } else {
            lv_label_set_text_fmt(s_status, "Credits %d\nWIN +%d!",
                                  s_credits, s_last_win);
        }
    } else if (s_phase == SLOT_RESULT && s_last_win > 0) {
        lv_label_set_text_fmt(s_status,
                              "Credits %d  Bet %d\nWIN +%d   Best %d",
                              s_credits, bet, s_last_win, s_best_win);
    } else if (s_phase == SLOT_RESULT) {
        lv_label_set_text_fmt(s_status,
                              "Credits %d  Bet %d\nNo win   Best %d",
                              s_credits, bet, s_best_win);
    } else if (s_free_left > 0) {
        lv_label_set_text_fmt(s_status,
                              "Credits %d  FREE x%d\nBest win %d",
                              s_credits, s_free_left, s_best_win);
    } else {
        lv_label_set_text_fmt(s_status,
                              "Credits %d  Bet %d\nBest win %d",
                              s_credits, bet, s_best_win);
    }
}

static void refresh_config(void)
{
    if (!s_cfg_label || !s_cfg_panel) return;
    const char *bmark = (s_cfg_focus == CFG_BET) ? ">" : " ";
    const char *vmark = (s_cfg_focus == CFG_VOL) ? ">" : " ";
    const char *gmark = (s_cfg_focus == CFG_BGM) ? ">" : " ";
    lv_label_set_text_fmt(s_cfg_label,
                          "CONFIG %s\n"
                          "%s Bet %d\n"
                          "%s Vol %d%%\n"
                          "%s BGM %s",
                          cur_theme()->tag,
                          bmark, BETS[s_bet_idx],
                          vmark, s_volume,
                          gmark, s_bgm_on ? "ON" : "OFF");
    /* Keep focused row visible when content exceeds panel height. */
    lv_obj_update_layout(s_cfg_panel);
    const int line_h = 16;
    const int title_h = 16;
    int focus_y = title_h + (int)s_cfg_focus * line_h;
    int view_h = (int)lv_obj_get_content_height(s_cfg_panel);
    int target = focus_y - (view_h / 2) + (line_h / 2);
    if (target < 0) target = 0;
    lv_obj_scroll_to_y(s_cfg_panel, target, LV_ANIM_OFF);
}

static void apply_volume(void)
{
    if (s_volume < 0) s_volume = 0;
    if (s_volume > 100) s_volume = 100;
    if (s_audio_ok) bsp_audio_set_volume((uint8_t)s_volume);
}

static void vol_hint_hide(lv_timer_t *t)
{
    (void)t;
    if (s_vol_hint) lv_obj_add_flag(s_vol_hint, LV_OBJ_FLAG_HIDDEN);
    if (s_vol_timer) {
        lv_timer_delete(s_vol_timer);
        s_vol_timer = NULL;
    }
}

static void show_volume_hint(void)
{
    if (!s_vol_hint) return;
    lv_label_set_text_fmt(s_vol_hint, "VOL %d%%", s_volume);
    lv_obj_set_style_text_color(s_vol_hint, lv_color_hex(cur_theme()->accent), 0);
    lv_obj_remove_flag(s_vol_hint, LV_OBJ_FLAG_HIDDEN);
    if (s_vol_timer) lv_timer_delete(s_vol_timer);
    s_vol_timer = lv_timer_create(vol_hint_hide, 1200, NULL);
    lv_timer_set_repeat_count(s_vol_timer, 1);
}

static void show_bgm_hint(void)
{
    if (!s_vol_hint) return;
    lv_label_set_text_fmt(s_vol_hint, "BGM %s", s_bgm_on ? "ON" : "OFF");
    lv_obj_set_style_text_color(s_vol_hint, lv_color_hex(cur_theme()->accent), 0);
    lv_obj_remove_flag(s_vol_hint, LV_OBJ_FLAG_HIDDEN);
    if (s_vol_timer) lv_timer_delete(s_vol_timer);
    s_vol_timer = lv_timer_create(vol_hint_hide, 1500, NULL);
    lv_timer_set_repeat_count(s_vol_timer, 1);
}

static void adjust_volume(int dir)
{
    s_volume += dir * 10;
    if (s_volume < 0) s_volume = 0;
    if (s_volume > 100) s_volume = 100;
    apply_volume();
    save_volume();
    show_volume_hint();
    refresh_config();
}

static void refresh_batt(void)
{
    if (!s_batt) return;
    int soc = bsp_battery_soc();
    if (soc < 0) lv_label_set_text(s_batt, "");
    else lv_label_set_text_fmt(s_batt, "%d%%", soc);
}

static void batt_tick(lv_timer_t *t)
{
    (void)t;
    refresh_batt();
}

static void load_records(void)
{
    s_best_win = 0;
    s_peak_credits = 100;
    s_volume = 80;
    s_theme = THEME_JIE;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return;
    int32_t best = 0, peak = 100, vol = 80, theme = 0;
    if (nvs_get_i32(h, NVS_KEY_BEST, &best) == ESP_OK && best > 0) s_best_win = (int)best;
    if (nvs_get_i32(h, NVS_KEY_PEAK, &peak) == ESP_OK && peak > 0) s_peak_credits = (int)peak;
    if (nvs_get_i32(h, NVS_KEY_VOL, &vol) == ESP_OK) {
        if (vol < 0) vol = 0;
        if (vol > 100) vol = 100;
        s_volume = (int)vol;
    }
    if (nvs_get_i32(h, NVS_KEY_THEME, &theme) == ESP_OK) {
        if (theme >= 0 && theme < THEME_COUNT) s_theme = (theme_id_t)theme;
    }
    nvs_close(h);
}

static void maybe_save_records(void)
{
    bool dirty = false;
    if (s_last_win > s_best_win) {
        s_best_win = s_last_win;
        dirty = true;
    }
    if (s_credits > s_peak_credits) {
        s_peak_credits = s_credits;
        dirty = true;
    }
    if (!dirty) return;

    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_i32(h, NVS_KEY_BEST, (int32_t)s_best_win);
    nvs_set_i32(h, NVS_KEY_PEAK, (int32_t)s_peak_credits);
    nvs_commit(h);
    nvs_close(h);
}

static void save_volume(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_i32(h, NVS_KEY_VOL, (int32_t)s_volume);
    nvs_commit(h);
    nvs_close(h);
}

static void save_theme(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_i32(h, NVS_KEY_THEME, (int32_t)s_theme);
    nvs_commit(h);
    nvs_close(h);
}

static void queue_sfx(sfx_t s)
{
    if (!s_audio_ok) return;
    s_sfx_req = s;
}

static void play_tone_ms(int hz, int ms, int amp)
{
    if (bsp_audio_set_format(SAMPLE_RATE, 16, 1) != ESP_OK) return;
    apply_volume();
    int scaled = (amp * s_volume) / 100;
    if (scaled <= 0) return;
    int16_t buf[CHUNK_SAMPLES];
    const int period = hz > 0 ? (SAMPLE_RATE / hz) : 1;
    int total = SAMPLE_RATE * ms / 1000;
    int phase = 0;
    while (total > 0) {
        int n = total < CHUNK_SAMPLES ? total : CHUNK_SAMPLES;
        for (int i = 0; i < n; i++) {
            buf[i] = (phase < period / 2) ? (int16_t)scaled : (int16_t)(-scaled);
            if (++phase >= period) phase = 0;
        }
        bsp_audio_write(buf, (size_t)n * sizeof(int16_t));
        total -= n;
    }
}

static void sfx_task(void *arg)
{
    (void)arg;
    for (;;) {
        sfx_t req = s_sfx_req;
        if (req == SFX_NONE) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        s_sfx_req = SFX_NONE;
        if (s_audio_mu) xSemaphoreTake(s_audio_mu, portMAX_DELAY);
        switch (req) {
        case SFX_TICK:
            play_tone_ms(880, 25, 2500);
            break;
        case SFX_STOP:
            play_tone_ms(440, 50, 3500);
            break;
        case SFX_WIN:
            play_tone_ms(660, 80, 4500);
            play_tone_ms(880, 100, 4500);
            break;
        case SFX_JACKPOT:
            play_tone_ms(523, 90, 5000);
            play_tone_ms(659, 90, 5000);
            play_tone_ms(784, 120, 5500);
            play_tone_ms(1046, 160, 6000);
            break;
        case SFX_FREE:
            play_tone_ms(784, 70, 4500);
            play_tone_ms(988, 70, 4500);
            play_tone_ms(1174, 120, 5000);
            break;
        default:
            break;
        }
        if (s_audio_mu) xSemaphoreGive(s_audio_mu);
    }
}

static void bgm_task(void *arg)
{
    (void)arg;
    int16_t pcm[CHUNK_SAMPLES];
    slots_ima_wav_t dec;
    for (;;) {
        if (!s_bgm_on || !s_audio_ok) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        theme_id_t th = s_bgm_theme;
        if (th < 0 || th >= THEME_COUNT) th = THEME_JIE;
        size_t wav_sz = 0;
        const uint8_t *wav = slots_bgm_wav((int)th, &wav_sz);
        if (!wav || !slots_ima_wav_open(&dec, wav, wav_sz)) {
            ESP_LOGW("slots", "BGM open failed theme=%d", (int)th);
            s_bgm_on = false;
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        if (s_audio_mu && xSemaphoreTake(s_audio_mu, pdMS_TO_TICKS(50)) != pdTRUE) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        int rate = dec.sample_rate > 0 ? dec.sample_rate : BGM_SAMPLE_RATE;
        if (bsp_audio_set_format((uint32_t)rate, 16, 1) != ESP_OK) {
            if (s_audio_mu) xSemaphoreGive(s_audio_mu);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        apply_volume();
        while (s_bgm_on && s_bgm_theme == th) {
            int got = slots_ima_wav_read(&dec, pcm, CHUNK_SAMPLES);
            if (got <= 0) {
                slots_ima_wav_rewind(&dec); /* 整首循环 */
                continue;
            }
            /* 音量缩放 */
            if (s_volume < 100) {
                for (int i = 0; i < got; i++) {
                    pcm[i] = (int16_t)((pcm[i] * s_volume) / 100);
                }
            }
            if (s_volume <= 0) {
                memset(pcm, 0, (size_t)got * sizeof(int16_t));
            }
            bsp_audio_write(pcm, (size_t)got * sizeof(int16_t));
            if (s_sfx_req != SFX_NONE) break;
        }
        if (s_audio_mu) xSemaphoreGive(s_audio_mu);
        if (s_sfx_req != SFX_NONE) vTaskDelay(pdMS_TO_TICKS(30));
    }
}

static void toggle_bgm(void)
{
    s_bgm_on = !s_bgm_on;
    s_bgm_theme = s_theme;
    refresh_config();
    show_bgm_hint();
}

static void cycle_bgm(void)
{
    /* 主页 OK 双击：切换 BGM 开/关（当前每主题一条 PCM 循环） */
    toggle_bgm();
}

static void begin_celebrate(bool jackpot, bool free_trig)
{
    s_phase = SLOT_CELEBRATE;
    s_cele_tick = 0;
    s_cele_jackpot = jackpot;
    s_cele_free = free_trig;
    if (s_banner) {
        if (free_trig) lv_label_set_text(s_banner, "FREE!");
        else if (jackpot) lv_label_set_text(s_banner, "JACKPOT");
        else lv_label_set_text(s_banner, "WIN!");
        lv_obj_remove_flag(s_banner, LV_OBJ_FLAG_HIDDEN);
    }
    if (free_trig) queue_sfx(SFX_FREE);
    else if (jackpot) queue_sfx(SFX_JACKPOT);
    else queue_sfx(SFX_WIN);
    refresh_status();
}

static void finish_spin(void)
{
    int bet = BETS[s_bet_idx];
    int mult = slot_payout_mult(s_target[0], s_target[1], s_target[2]);
    s_last_win = bet * mult;
    s_credits += s_last_win;

    bool free_trig = slot_is_free_trigger(s_target[0], s_target[1], s_target[2]);
    bool jackpot = slot_is_jackpot(s_target[0], s_target[1], s_target[2]);
    if (free_trig) s_free_left += FREE_SPIN_AWARD;

    maybe_save_records();

    if (s_last_win > 0 || free_trig) {
        begin_celebrate(jackpot, free_trig);
    } else {
        s_phase = SLOT_RESULT;
        if (s_banner) lv_obj_add_flag(s_banner, LV_OBJ_FLAG_HIDDEN);
        set_cells_flash(false, 0);
        refresh_status();
    }
}

static void spin_tick(lv_timer_t *t)
{
    (void)t;

    if (s_phase == SLOT_CELEBRATE) {
        s_cele_tick++;
        bool on = (s_cele_tick / 3) % 2 == 0;
        uint32_t col = s_cele_jackpot ? cur_theme()->flash_jackpot
                     : (s_cele_free ? cur_theme()->flash_free : cur_theme()->flash_win);
        set_cells_flash(on, col);
        if (s_cele_tick >= 24) {
            set_cells_flash(false, 0);
            if (s_banner) lv_obj_add_flag(s_banner, LV_OBJ_FLAG_HIDDEN);
            s_phase = SLOT_RESULT;
            refresh_status();
            if (s_free_left > 0) start_spin();
        }
        return;
    }

    if (s_phase != SLOT_SPINNING) return;

    s_spin_tick++;
    for (int i = 0; i < 3; i++) {
        if (s_spin_tick < s_stop_at[i]) {
            set_reel(i, slot_symbol_from_rand(esp_random()));
            if ((s_spin_tick % 2) == 0) queue_sfx(SFX_TICK);
        } else if (s_spin_tick == s_stop_at[i]) {
            set_reel(i, s_target[i]);
            queue_sfx(SFX_STOP);
        }
    }

    if (s_spin_tick >= s_stop_at[2]) finish_spin();
}

static void start_spin(void)
{
    if (s_phase == SLOT_SPINNING || s_phase == SLOT_CELEBRATE) return;
    int bet = BETS[s_bet_idx];
    bool using_free = (s_free_left > 0);

    if (!using_free && s_credits < bet) {
        lv_label_set_text_fmt(s_status, "Credits %d  Bet %d\nNot enough!",
                              s_credits, bet);
        s_phase = SLOT_IDLE;
        return;
    }

    if (using_free) s_free_left--;
    else s_credits -= bet;

    s_last_win = 0;
    s_spin_tick = 0;
    s_stop_at[0] = 12;
    s_stop_at[1] = 18;
    s_stop_at[2] = 24;

    // 约 1/40 强制三同 ST，方便演示免费局
    if ((esp_random() % 40) == 0) {
        s_target[0] = s_target[1] = s_target[2] = SLOT_SYM_STAR;
    } else {
        for (int i = 0; i < 3; i++) s_target[i] = slot_symbol_from_rand(esp_random());
    }

    s_phase = SLOT_SPINNING;
    if (s_banner) lv_obj_add_flag(s_banner, LV_OBJ_FLAG_HIDDEN);
    refresh_status();
}

void demo_slots_reset_credits(void)
{
    s_credits = 100;
    s_bet_idx = 0;
    s_phase = SLOT_IDLE;
    s_last_win = 0;
    s_free_left = 0;
    set_cells_flash(false, 0);
    if (s_banner) lv_obj_add_flag(s_banner, LV_OBJ_FLAG_HIDDEN);
    set_reel(0, SLOT_SYM_SEVEN);
    set_reel(1, SLOT_SYM_SEVEN);
    set_reel(2, SLOT_SYM_SEVEN);
    refresh_status();
    refresh_config();
}

void demo_slots_enter(void)
{
    esp_err_t nvs_err = nvs_flash_init();
    (void)nvs_err;
    load_records();

    s_audio_ok = (bsp_audio_init() == ESP_OK);
    if (!s_audio_mu) s_audio_mu = xSemaphoreCreateMutex();
    if (s_audio_ok && !s_sfx_task) {
        xTaskCreate(sfx_task, "slot_sfx", 3072, NULL, 4, &s_sfx_task);
    }
    if (s_audio_ok && !s_bgm_task) {
        xTaskCreate(bgm_task, "slot_bgm", 3072, NULL, 3, &s_bgm_task);
    }
    s_bgm_on = false;
    s_bgm_theme = s_theme;
    if (!slots_bgm_bank_ready()) {
        ESP_LOGW("slots", "BGM bank not flashed; OKx2 will not play full songs");
    }

    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    s_hero = lv_image_create(s_scr);
    if (cur_theme()->heroes && cur_theme()->hero_count > 0) {
        lv_image_set_src(s_hero, cur_theme()->heroes[s_hero_idx]);
    }
    lv_obj_set_pos(s_hero, 0, 0);
    lv_obj_set_size(s_hero, 240, 320);
    lv_obj_remove_flag(s_hero, LV_OBJ_FLAG_SCROLLABLE);

    s_vol_hint = lv_label_create(s_scr);
    lv_obj_set_style_text_font(s_vol_hint, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_vol_hint, lv_color_hex(cur_theme()->accent), 0);
    lv_obj_align(s_vol_hint, LV_ALIGN_BOTTOM_MID, 0, -28);
    lv_label_set_text(s_vol_hint, "");
    lv_obj_add_flag(s_vol_hint, LV_OBJ_FLAG_HIDDEN);

    s_ui_layer = lv_obj_create(s_scr);
    lv_obj_remove_flag(s_ui_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_ui_layer, 0, 0);
    lv_obj_set_size(s_ui_layer, 240, 320);
    lv_obj_set_style_bg_opa(s_ui_layer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_ui_layer, 0, 0);
    lv_obj_set_style_pad_all(s_ui_layer, 0, 0);
    lv_obj_add_flag(s_ui_layer, LV_OBJ_FLAG_HIDDEN);
    s_ui_shown = false;

    const theme_style_t *t0 = cur_theme();

    /* 标题贴纸：黑阴影 + 白描边色块 */
    s_title_shadow = ui_block(s_ui_layer, 8, 8, 128, 28, 0x000000);
    s_title_plate = make_pixel_chip(s_ui_layer, 4, 4, 128, 28, t0->title_bg, t0->border);
    s_title_lbl = lv_label_create(s_title_plate);
    lv_label_set_text(s_title_lbl, t0->title);
    lv_obj_set_style_text_font(s_title_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_hex(t0->title_fg), 0);
    lv_obj_center(s_title_lbl);

    /* 主题角标丝带（EVA / ORANGE / MOON） */
    s_ribbon = make_pixel_chip(s_ui_layer, 4, 36, 92, 20, t0->accent, t0->border);
    s_ribbon_lbl = lv_label_create(s_ribbon);
    lv_label_set_text(s_ribbon_lbl, t0->ribbon);
    lv_obj_set_style_text_font(s_ribbon_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ribbon_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(s_ribbon_lbl);

    s_batt = lv_label_create(s_ui_layer);
    lv_obj_set_style_text_font(s_batt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_batt, lv_color_hex(t0->ink), 0);
    lv_obj_align(s_batt, LV_ALIGN_TOP_RIGHT, -8, 8);
    refresh_batt();

    /* 状态条 */
    s_status_bar = make_pixel_chip(s_ui_layer, 8, 60, 224, 30, t0->reel, t0->accent);
    lv_obj_set_style_bg_opa(s_status_bar, LV_OPA_90, 0);
    s_status = lv_label_create(s_status_bar);
    lv_obj_set_width(s_status, 210);
    lv_obj_set_style_text_font(s_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_status, lv_color_hex(t0->status), 0);
    lv_obj_align(s_status, LV_ALIGN_LEFT_MID, 6, 0);

    /* 霓虹多层转轴窗：glow → black → accent 面板 */
    s_panel_glow = ui_block(s_ui_layer, 2, 94, 236, 118, t0->glow);
    s_panel_outline = ui_block(s_ui_layer, 6, 98, 228, 110, 0x000000);
    s_panel = make_panel(s_ui_layer, 10, 102, 220, 102, t0->panel, t0->accent, t0->accent,
                         &s_panel_shadow);
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_95, 0);
    lv_obj_set_style_border_width(s_panel, 4, 0);

    /* 主题色块图案（代替人物头像，左右棋盘） */
    for (int i = 0; i < PAT_TILES; i++) {
        int col = i % 2;
        int row = i / 2;
        int side = (row < 5) ? 0 : 1; /* 左 10 + 右 10 */
        int r = row % 5;
        int x = side ? 198 : 14;
        int y = 108 + r * 16;
        s_pat[i] = make_pixel_chip(s_ui_layer, x + col * 12, y, 10, 10,
                                   (i & 1) ? t0->pat_b : t0->pat_a, t0->border);
    }

    for (int i = 0; i < 3; i++) {
        int x = 42 + i * 56;
        s_reel_cell[i] = make_panel(s_panel, x - 10, 12, 50, 72,
                                    t0->reel_hi, t0->accent, 0x000000, &s_reel_shadow[i]);
        lv_obj_set_style_border_width(s_reel_cell[i], 3, 0);
        lv_obj_set_style_pad_all(s_reel_cell[i], 2, 0);
        s_reel_lbl[i] = lv_label_create(s_reel_cell[i]);
        lv_obj_set_style_text_font(s_reel_lbl[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_align(s_reel_lbl[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(s_reel_lbl[i]);
    }

    s_banner = lv_label_create(s_ui_layer);
    lv_obj_set_style_text_font(s_banner, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_banner, lv_color_hex(t0->flash_jackpot), 0);
    lv_obj_align(s_banner, LV_ALIGN_TOP_MID, 0, 130);
    lv_label_set_text(s_banner, "");
    lv_obj_add_flag(s_banner, LV_OBJ_FLAG_HIDDEN);

    s_cfg_panel = make_panel(s_ui_layer, 8, 222, 224, 78, t0->panel, t0->accent, t0->accent,
                             &s_cfg_shadow);
    lv_obj_set_style_bg_opa(s_cfg_panel, LV_OPA_95, 0);
    lv_obj_set_style_pad_all(s_cfg_panel, 4, 0);
    lv_obj_set_scroll_dir(s_cfg_panel, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(s_cfg_panel, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(s_cfg_panel, LV_OBJ_FLAG_SCROLLABLE);
    s_cfg_label = lv_label_create(s_cfg_panel);
    lv_obj_set_width(s_cfg_label, 200);
    lv_obj_set_style_text_font(s_cfg_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_cfg_label, lv_color_hex(t0->ink), 0);
    lv_obj_set_style_text_line_space(s_cfg_label, 2, 0);
    lv_obj_align(s_cfg_label, LV_ALIGN_TOP_LEFT, 0, 0);

    s_timer = lv_timer_create(spin_tick, 50, NULL);
    s_batt_timer = lv_timer_create(batt_tick, 5000, NULL);

    demo_slots_reset_credits();
    apply_volume();
    apply_theme();
    refresh_status();
    refresh_config();
    lv_screen_load(s_scr);
}

void demo_slots_exit(void)
{
    s_bgm_on = false;
    s_sfx_req = SFX_NONE;
    if (s_timer) { lv_timer_delete(s_timer); s_timer = NULL; }
    if (s_batt_timer) { lv_timer_delete(s_batt_timer); s_batt_timer = NULL; }
    if (s_vol_timer) { lv_timer_delete(s_vol_timer); s_vol_timer = NULL; }
    if (s_scr) {
        lv_obj_delete(s_scr);
        s_scr = NULL;
        s_hero = s_vol_hint = s_ui_layer = NULL;
        s_title_shadow = s_title_plate = s_title_lbl = NULL;
        s_ribbon = s_ribbon_lbl = NULL;
        s_panel_glow = s_panel_outline = NULL;
        s_panel_shadow = s_panel = s_status_bar = s_status = s_banner = NULL;
        s_cfg_shadow = s_cfg_panel = s_cfg_label = s_batt = NULL;
        s_reel_lbl[0] = s_reel_lbl[1] = s_reel_lbl[2] = NULL;
        s_reel_cell[0] = s_reel_cell[1] = s_reel_cell[2] = NULL;
        s_reel_shadow[0] = s_reel_shadow[1] = s_reel_shadow[2] = NULL;
        for (int i = 0; i < PAT_TILES; i++) s_pat[i] = NULL;
    }
    s_phase = SLOT_IDLE;
    s_ui_shown = false;
}

void demo_slots_key(bsp_btn_t btn, bsp_btn_ev_t ev)
{
    // 主页：上下切图/主题；上下双击调音量；OK 进局；OK 双击切换 BGM（不显示操作提示）
    if (!s_ui_shown) {
        if (btn == BSP_BTN_OK && ev == BSP_BTN_DOUBLE) {
            cycle_bgm();
            return;
        }
        if (btn == BSP_BTN_OK && ev == BSP_BTN_CLICK && s_ui_layer) {
            s_ui_shown = true;
            lv_obj_remove_flag(s_ui_layer, LV_OBJ_FLAG_HIDDEN);
            refresh_config();
            return;
        }
        if ((btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) && ev == BSP_BTN_DOUBLE) {
            adjust_volume(btn == BSP_BTN_UP ? 1 : -1);
            return;
        }
        if ((btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) && ev == BSP_BTN_CLICK) {
            cycle_theme(btn == BSP_BTN_UP ? 1 : -1);
        }
        return;
    }

    if (s_phase == SLOT_SPINNING || s_phase == SLOT_CELEBRATE) return;

    // 局内：上双击回主页
    if (btn == BSP_BTN_UP && ev == BSP_BTN_DOUBLE) {
        hide_to_splash();
        return;
    }

    // 局内：OK 双击切换 CONFIG 选中项（Bet / Vol / BGM）
    if (btn == BSP_BTN_OK && ev == BSP_BTN_DOUBLE) {
        s_cfg_focus = (s_cfg_focus + 1) % CFG_COUNT;
        refresh_config();
        return;
    }

    if (ev != BSP_BTN_CLICK) return;

    // 局内：OK 单击开转
    if (btn == BSP_BTN_OK) {
        if (s_phase == SLOT_RESULT) s_phase = SLOT_IDLE;
        start_spin();
        return;
    }

    // 局内：上下调节当前选中的 CONFIG 项
    if (btn == BSP_BTN_UP || btn == BSP_BTN_DOWN) {
        int dir = (btn == BSP_BTN_UP) ? 1 : -1;
        if (s_cfg_focus == CFG_BET) {
            if (s_free_left > 0) return;
            s_bet_idx = (s_bet_idx + dir + BET_COUNT) % BET_COUNT;
            refresh_status();
            refresh_config();
            return;
        }
        if (s_cfg_focus == CFG_BGM) {
            bool want_on = (dir > 0);
            if (want_on != s_bgm_on) toggle_bgm();
            else refresh_config();
            return;
        }
        adjust_volume(dir);
    }
}
