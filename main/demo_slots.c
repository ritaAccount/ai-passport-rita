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

#define REEL_PIECES 7
#define SAMPLE_RATE     16000
#define BGM_SAMPLE_RATE 8000
#define CHUNK_SAMPLES   256
#define NVS_NS        "slots"
#define NVS_KEY_BEST  "best_win"
#define NVS_KEY_PEAK  "peak_cr"
#define NVS_KEY_VOL   "vol"
#define NVS_KEY_THEME "theme"
#define NVS_KEY_HERO  "hero"
#define NVS_KEY_LVL   "level"
#define NVS_KEY_LIFE  "life_won"
#define NVS_KEY_TIER  "tier"
#define NVS_KEY_NWIN  "pity_nw"
#define NVS_KEY_NHI   "pity_nh"
#define NVS_KEY_NSP   "pity_ns"
#define NVS_KEY_NJP   "pity_nj"

typedef enum {
    SLOT_IDLE = 0,
    SLOT_SPINNING,
    SLOT_BONUS_ANIM,   /* Bonus 独立小动画页 */
    SLOT_CELEBRATE,
    SLOT_LEVEL_UP,     /* 升级弹框 */
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
    SFX_WIN,        /* 普通中奖 */
    SFX_JACKPOT,    /* Jackpot */
    SFX_MEGA,       /* Mega 大奖 */
    SFX_BONUS,      /* Bonus 入场 / Bonus 结算 */
    SFX_LEVEL,      /* 升级横幅 */
} sfx_t;

typedef struct {
    const lv_image_dsc_t *const *heroes;
    int hero_count;
    const char *title;
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
    uint32_t flash_win;
    uint32_t flash_jackpot;
    uint32_t flash_bonus;
    uint32_t flash_mega;
    uint32_t flash_level;
    uint32_t sym[SLOT_SYM_COUNT];
} theme_style_t;

static const lv_image_dsc_t *const JIE_HEROES[] = {
    &slots_jie_hero, &slots_jie_hero2, &slots_jie_hero3, &slots_jie_hero4,
};
static const lv_image_dsc_t *const P3_HEROES[] = { &slots_p3_hero, &slots_p3_hero2, &slots_p3_hero3 };
static const lv_image_dsc_t *const MARI_HEROES[] = { &slots_mari_hero, &slots_mari_hero2, &slots_mari_hero3 };

static const theme_style_t THEMES[THEME_COUNT] = {
    [THEME_JIE] = {
        .heroes = JIE_HEROES,
        .hero_count = 3,
        .title = "JIE",
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
        .flash_win = 0xE53935,
        .flash_jackpot = 0xFFD54F,
        .flash_bonus = 0xFF6D00,
        .flash_mega = 0xFFD700,
        .flash_level = 0xFFB74D, /* 琥珀，区别于中奖红/金 */
        /* CHERRY LEMON BELL STAR DIAMOND WILD BONUS MEGA */
        .sym = { 0xFF7043, 0xFFD54F, 0xFFFFFF, 0x4FC3F7, 0xFF1744, 0x7C4DFF, 0xFF6D00, 0xFFD700 },
    },
    [THEME_P3] = {
        .heroes = P3_HEROES,
        .hero_count = 3,
        .title = "P3R",
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
        .flash_win = 0x00AEEF,
        .flash_jackpot = 0xFFFFFF,
        .flash_bonus = 0xFF9100,
        .flash_mega = 0xFFD600,
        .flash_level = 0xFFB74D,
        .sym = { 0x4FC3F7, 0xFFFFFF, 0x82B1FF, 0xFF5252, 0x00E5FF, 0xB388FF, 0xFF9100, 0xFFD600 },
    },
    [THEME_MARI] = {
        .heroes = MARI_HEROES,
        .hero_count = 3,
        .title = "MARI",
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
        .flash_win = 0xFF2D95,
        .flash_jackpot = 0xFFFFFF,
        .flash_bonus = 0xFFAB40,
        .flash_mega = 0xFFE57F,
        .flash_level = 0xFFB74D,
        .sym = { 0xFF5252, 0xFFFFFF, 0xFF2D95, 0xCE93D8, 0xFF1744, 0xEA80FC, 0xFFAB40, 0xFFE57F },
    },
};

static lv_obj_t *s_scr;
static lv_obj_t *s_hero;
static lv_obj_t *s_vol_hint;
static lv_obj_t *s_ui_layer;
static lv_obj_t *s_title_shadow;
static lv_obj_t *s_title_plate;
static lv_obj_t *s_title_lbl;
static lv_obj_t *s_panel_glow;
static lv_obj_t *s_panel_outline;
static lv_obj_t *s_panel;
static lv_obj_t *s_reel_cell[3];
static lv_obj_t *s_reel_icon[3];
static lv_obj_t *s_reel_piece[3][REEL_PIECES];
static lv_obj_t *s_status_bar;
static lv_obj_t *s_status_left;
static lv_obj_t *s_status_right;
static lv_obj_t *s_banner_bg;
static lv_obj_t *s_banner;
static lv_obj_t *s_bonus_layer;
static lv_obj_t *s_bonus_lbl;
static lv_obj_t *s_cfg_panel;
static lv_obj_t *s_cfg_left;
static lv_obj_t *s_cfg_right;
static lv_obj_t *s_batt;
static lv_timer_t *s_timer;
static lv_timer_t *s_batt_timer;
static lv_timer_t *s_vol_timer;

static slot_phase_t s_phase;
static bool s_ui_shown;
static theme_id_t s_theme = THEME_JIE;
static int s_hero_idx;
static int s_credits = 500;
static int s_volume = 80;
static int s_cfg_focus = CFG_BET;
static int s_reels[3];
static int s_target[3];
static int s_spin_tick;
static int s_stop_at[3];
static int s_last_win;
static int s_best_win;
static int s_peak_credits = 500;
static int s_lifetime_won;          /* 累计赢分，驱动升级 */
static int s_pending_level;         /* 庆祝后要展示的新等级；0=无 */
static int s_cele_tick;
static bool s_cele_jackpot;
static bool s_cele_mega;
static bool s_cele_bonus;
static slot_player_t s_player;
static slot_spin_result_t s_spin_res;
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
static void save_player_progress(void);
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
static void apply_spin_payout(void);
static void begin_celebrate_win(void);
static void begin_level_up(int new_lv);
static void start_bonus_anim(void);
static void hide_bonus_layer(void);
static void show_banner(const char *text, uint32_t bg_color);
static void hide_banner(void);

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

/* 无右下偏移阴影的描边块 */
static lv_obj_t *make_flat_panel(lv_obj_t *parent, int x, int y, int w, int h,
                                 uint32_t fill, uint32_t border)
{
    lv_obj_t *panel = ui_block(parent, x, y, w, h, fill);
    lv_obj_set_style_border_color(panel, lv_color_hex(border), 0);
    lv_obj_set_style_border_width(panel, 3, 0);
    lv_obj_set_style_pad_all(panel, 2, 0);
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

static void paint_reel_sym(int reel, int sym)
{
    if (reel < 0 || reel >= 3 || !s_reel_icon[reel]) return;
    const theme_style_t *t = cur_theme();
    uint32_t c = t->sym[sym];
    for (int p = 0; p < REEL_PIECES; p++) {
        if (s_reel_piece[reel][p]) lv_obj_add_flag(s_reel_piece[reel][p], LV_OBJ_FLAG_HIDDEN);
    }

    /* 固定块拼出可爱像素符号；只改位置/颜色，避免转轴时频繁创建对象 */
    #define PIECE(p, x, y, w, h, col, rad) do { \
        lv_obj_t *_o = s_reel_piece[reel][p]; \
        if (!_o) break; \
        lv_obj_set_pos(_o, (x), (y)); \
        lv_obj_set_size(_o, (w), (h)); \
        lv_obj_set_style_bg_color(_o, lv_color_hex(col), 0); \
        lv_obj_set_style_radius(_o, (rad), 0); \
        lv_obj_remove_flag(_o, LV_OBJ_FLAG_HIDDEN); \
    } while (0)

    switch (sym) {
    case SLOT_SYM_CHERRY: /* 双樱桃 */
        PIECE(0, 18, 4, 4, 16, 0x6D4C41, 0);
        PIECE(1, 20, 2, 14, 8, 0x66BB6A, 4);
        PIECE(2, 2, 18, 18, 18, c, LV_RADIUS_CIRCLE);
        PIECE(3, 18, 26, 18, 18, c, LV_RADIUS_CIRCLE);
        PIECE(4, 6, 22, 5, 5, 0xFFFFFF, LV_RADIUS_CIRCLE);
        PIECE(5, 22, 30, 5, 5, 0xFFFFFF, LV_RADIUS_CIRCLE);
        break;
    case SLOT_SYM_LEMON: /* 柠檬 */
        PIECE(0, 4, 12, 32, 30, c, 14);
        PIECE(1, 17, 6, 6, 10, 0x8BC34A, 3);
        PIECE(2, 12, 20, 10, 6, 0xFFFFFF, 3);
        break;
    case SLOT_SYM_BELL: /* 铃铛 */
        PIECE(0, 14, 2, 12, 8, c, 4);
        PIECE(1, 8, 10, 24, 28, c, 6);
        PIECE(2, 4, 34, 32, 8, c, 2);
        PIECE(3, 16, 40, 8, 8, 0xFFD54F, LV_RADIUS_CIRCLE);
        PIECE(4, 12, 16, 8, 6, 0xFFFFFF, 3);
        break;
    case SLOT_SYM_STAR: /* 星星 */
        PIECE(0, 16, 2, 8, 48, c, 0);
        PIECE(1, 0, 20, 40, 8, c, 0);
        PIECE(2, 6, 10, 12, 12, c, 2);
        PIECE(3, 22, 10, 12, 12, c, 2);
        PIECE(4, 6, 30, 12, 12, c, 2);
        PIECE(5, 22, 30, 12, 12, c, 2);
        PIECE(6, 16, 20, 8, 8, 0xFFFFFF, LV_RADIUS_CIRCLE);
        break;
    case SLOT_SYM_DIAMOND: /* 钻石 */
        PIECE(0, 12, 2, 16, 14, c, 2);
        PIECE(1, 4, 14, 32, 22, c, 4);
        PIECE(2, 12, 34, 16, 14, c, 2);
        PIECE(3, 16, 8, 8, 8, 0xFFFFFF, LV_RADIUS_CIRCLE);
        PIECE(4, 10, 20, 6, 6, 0xFFFFFF, LV_RADIUS_CIRCLE);
        PIECE(5, 24, 24, 5, 5, 0xFFF59D, LV_RADIUS_CIRCLE);
        break;
    case SLOT_SYM_WILD: /* 扑克牌 / Wild */
        PIECE(0, 8, 4, 24, 48, c, 4);
        PIECE(1, 12, 8, 16, 40, 0xFFFFFF, 2);
        PIECE(2, 16, 14, 8, 8, c, LV_RADIUS_CIRCLE);
        PIECE(3, 16, 28, 8, 12, c, 2);
        PIECE(4, 10, 6, 6, 6, 0xFF1744, 0);
        PIECE(5, 24, 44, 6, 6, 0xFF1744, 0);
        break;
    case SLOT_SYM_BONUS: /* 礼物盒 */
        PIECE(0, 6, 18, 28, 28, c, 4);
        PIECE(1, 4, 14, 32, 10, 0xFFFFFF, 2);
        PIECE(2, 16, 8, 8, 40, 0xFFF59D, 2);
        PIECE(3, 10, 4, 20, 8, 0xFF5252, 4);
        PIECE(4, 18, 2, 4, 6, 0xFF5252, 2);
        break;
    case SLOT_SYM_MEGA: /* 皇冠 */
        PIECE(0, 4, 22, 32, 18, c, 4);
        PIECE(1, 6, 10, 8, 16, c, 2);
        PIECE(2, 16, 4, 8, 22, c, 2);
        PIECE(3, 26, 10, 8, 16, c, 2);
        PIECE(4, 8, 8, 4, 4, 0xFFFFFF, LV_RADIUS_CIRCLE);
        PIECE(5, 18, 2, 4, 4, 0xFFFFFF, LV_RADIUS_CIRCLE);
        PIECE(6, 28, 8, 4, 4, 0xFFFFFF, LV_RADIUS_CIRCLE);
        break;
    default:
        PIECE(0, 12, 2, 16, 14, c, 2);
        PIECE(1, 4, 14, 32, 22, c, 4);
        PIECE(2, 12, 34, 16, 14, c, 2);
        break;
    }
    #undef PIECE
}

static void set_reel(int i, int sym)
{
    if (sym < 0 || sym >= SLOT_SYM_COUNT) sym = 0;
    s_reels[i] = sym;
    paint_reel_sym(i, sym);
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
    if (s_panel_glow) lv_obj_set_style_bg_color(s_panel_glow, lv_color_hex(t->glow), 0);
    if (s_panel_outline) lv_obj_set_style_bg_color(s_panel_outline, lv_color_hex(0x000000), 0);
    if (s_panel) {
        lv_obj_set_style_bg_color(s_panel, lv_color_hex(t->panel), 0);
        lv_obj_set_style_border_color(s_panel, lv_color_hex(t->accent), 0);
    }
    if (s_status_bar) {
        lv_obj_set_style_bg_color(s_status_bar, lv_color_hex(t->reel), 0);
        lv_obj_set_style_border_color(s_status_bar, lv_color_hex(t->accent), 0);
    }
    if (s_status_left) lv_obj_set_style_text_color(s_status_left, lv_color_hex(t->status), 0);
    if (s_status_right) lv_obj_set_style_text_color(s_status_right, lv_color_hex(t->status), 0);
    if (s_banner_bg) {
        lv_obj_set_style_bg_color(s_banner_bg, lv_color_hex(t->flash_jackpot), 0);
        lv_obj_set_style_border_color(s_banner_bg, lv_color_hex(t->border), 0);
    }
    if (s_banner) lv_obj_set_style_text_color(s_banner, lv_color_hex(0x212121), 0);
    if (s_batt) lv_obj_set_style_text_color(s_batt, lv_color_hex(t->ink), 0);
    if (s_cfg_panel) {
        lv_obj_set_style_bg_color(s_cfg_panel, lv_color_hex(t->panel), 0);
        lv_obj_set_style_border_color(s_cfg_panel, lv_color_hex(t->accent), 0);
    }
    if (s_cfg_left) lv_obj_set_style_text_color(s_cfg_left, lv_color_hex(t->ink), 0);
    if (s_cfg_right) lv_obj_set_style_text_color(s_cfg_right, lv_color_hex(t->ink), 0);
    if (s_vol_hint) lv_obj_set_style_text_color(s_vol_hint, lv_color_hex(t->accent), 0);
    for (int i = 0; i < 3; i++) {
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
    hide_banner();
    hide_bonus_layer();
    set_cells_flash(false, 0);
    if (s_ui_layer) lv_obj_add_flag(s_ui_layer, LV_OBJ_FLAG_HIDDEN);
}

static void cycle_theme(int dir)
{
    /* 主页上下：先翻同一主题立绘，翻完再切下一主题；主题+立绘索引都写入 NVS，关机后恢复 */
    const theme_style_t *t = cur_theme();
    int next = s_hero_idx + dir;
    if (next >= 0 && next < t->hero_count) {
        s_hero_idx = next;
        apply_theme();
        save_theme();
        return;
    }
    s_theme = (theme_id_t)((s_theme + dir + THEME_COUNT) % THEME_COUNT);
    t = cur_theme();
    s_hero_idx = (dir > 0) ? 0 : (t->hero_count - 1);
    if (s_hero_idx < 0) s_hero_idx = 0;
    apply_theme();
    save_theme();
}

static void refresh_status_right(void)
{
    if (!s_status_right) return;
    int next_lv = s_player.level + 1;
    int rem = slot_lifetime_needed_for_level(next_lv) - s_lifetime_won;
    if (rem < 0) rem = 0;
    lv_label_set_text_fmt(s_status_right, "Lv %d\nNeed %d", s_player.level, rem);
}

static void refresh_status(void)
{
    if (!s_status_left || !s_status_right) return;
    const int bet = slot_tier_bet(s_player.tier);
    if (s_phase == SLOT_SPINNING || s_phase == SLOT_BONUS_ANIM) {
        lv_label_set_text_fmt(s_status_left, "Credits %d\nBet %d", s_credits, bet);
    } else if (s_phase == SLOT_LEVEL_UP) {
        lv_label_set_text_fmt(s_status_left, "Credits %d\nLEVEL UP!", s_credits);
    } else if (s_phase == SLOT_CELEBRATE) {
        if (s_cele_mega) {
            lv_label_set_text_fmt(s_status_left, "Credits %d\nMEGA +%d!", s_credits, s_last_win);
        } else if (s_cele_bonus) {
            lv_label_set_text_fmt(s_status_left, "Credits %d\nBONUS +%d!", s_credits, s_last_win);
        } else if (s_cele_jackpot) {
            lv_label_set_text_fmt(s_status_left, "Credits %d\nJP +%d!", s_credits, s_last_win);
        } else {
            lv_label_set_text_fmt(s_status_left, "Credits %d\nWIN +%d!", s_credits, s_last_win);
        }
    } else if (s_phase == SLOT_RESULT && s_last_win > 0) {
        lv_label_set_text_fmt(s_status_left, "Credits %d\nWIN +%d", s_credits, s_last_win);
    } else {
        lv_label_set_text_fmt(s_status_left, "Credits %d\nBet %d", s_credits, bet);
    }
    refresh_status_right();
}

static void show_banner(const char *text, uint32_t bg_color)
{
    if (s_banner_bg) {
        lv_obj_set_style_bg_color(s_banner_bg, lv_color_hex(bg_color), 0);
        lv_obj_remove_flag(s_banner_bg, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_banner_bg);
    }
    if (s_banner) {
        lv_label_set_text(s_banner, text);
        lv_obj_remove_flag(s_banner, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_banner);
    }
}

static void hide_banner(void)
{
    if (s_banner_bg) lv_obj_add_flag(s_banner_bg, LV_OBJ_FLAG_HIDDEN);
    if (s_banner) lv_obj_add_flag(s_banner, LV_OBJ_FLAG_HIDDEN);
}

static void refresh_config(void)
{
    if (!s_cfg_left || !s_cfg_right) return;
    const char *bmark = (s_cfg_focus == CFG_BET) ? ">" : " ";
    const char *vmark = (s_cfg_focus == CFG_VOL) ? ">" : " ";
    const char *gmark = (s_cfg_focus == CFG_BGM) ? ">" : " ";
    /* 两列等宽 × 两行：左 Bet/BGM，右 Vol / 空 */
    lv_label_set_text_fmt(s_cfg_left,
                          "%sBet %d\n"
                          "%sBGM %s",
                          bmark, slot_tier_bet(s_player.tier),
                          gmark, s_bgm_on ? "ON" : "OFF");
    lv_label_set_text_fmt(s_cfg_right,
                          "%sVol %d%%\n"
                          " ",
                          vmark, s_volume);
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
    s_peak_credits = 500;
    s_volume = 80;
    s_theme = THEME_JIE;
    s_hero_idx = 0;
    s_lifetime_won = 0;
    memset(&s_player, 0, sizeof(s_player));
    s_player.level = 1;
    s_player.tier = SLOT_TIER_BRONZE;

    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return;
    int32_t best = 0, peak = 500, vol = 80, theme = 0, hero = 0;
    int32_t life = 0, tier = 0, nw = 0, nh = 0, ns = 0, nj = 0;
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
    if (nvs_get_i32(h, NVS_KEY_HERO, &hero) == ESP_OK) s_hero_idx = (int)hero;
    if (nvs_get_i32(h, NVS_KEY_LIFE, &life) == ESP_OK && life >= 0) s_lifetime_won = (int)life;
    /* 等级始终由累计赢分推导，曲线调整后进度一致 */
    s_player.level = slot_level_from_lifetime_won(s_lifetime_won);
    if (nvs_get_i32(h, NVS_KEY_TIER, &tier) == ESP_OK
        && tier >= 0 && tier < SLOT_TIER_COUNT
        && slot_tier_unlocked((slot_tier_t)tier, s_player.level)) {
        s_player.tier = (slot_tier_t)tier;
    } else {
        s_player.tier = slot_highest_unlocked_tier(s_player.level);
    }
    if (nvs_get_i32(h, NVS_KEY_NWIN, &nw) == ESP_OK) s_player.pity.no_win = (int)nw;
    if (nvs_get_i32(h, NVS_KEY_NHI, &nh) == ESP_OK) s_player.pity.no_high = (int)nh;
    if (nvs_get_i32(h, NVS_KEY_NSP, &ns) == ESP_OK) s_player.pity.no_special = (int)ns;
    if (nvs_get_i32(h, NVS_KEY_NJP, &nj) == ESP_OK) s_player.pity.no_jackpot = (int)nj;
    nvs_close(h);
    if (s_hero_idx < 0 || s_hero_idx >= THEMES[s_theme].hero_count) s_hero_idx = 0;
}

static void save_player_progress(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_i32(h, NVS_KEY_LVL, (int32_t)s_player.level);
    nvs_set_i32(h, NVS_KEY_LIFE, (int32_t)s_lifetime_won);
    nvs_set_i32(h, NVS_KEY_TIER, (int32_t)s_player.tier);
    nvs_set_i32(h, NVS_KEY_NWIN, (int32_t)s_player.pity.no_win);
    nvs_set_i32(h, NVS_KEY_NHI, (int32_t)s_player.pity.no_high);
    nvs_set_i32(h, NVS_KEY_NSP, (int32_t)s_player.pity.no_special);
    nvs_set_i32(h, NVS_KEY_NJP, (int32_t)s_player.pity.no_jackpot);
    nvs_commit(h);
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
    if (!dirty) {
        save_player_progress();
        return;
    }

    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_i32(h, NVS_KEY_BEST, (int32_t)s_best_win);
    nvs_set_i32(h, NVS_KEY_PEAK, (int32_t)s_peak_credits);
    nvs_commit(h);
    nvs_close(h);
    save_player_progress();
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
    nvs_set_i32(h, NVS_KEY_HERO, (int32_t)s_hero_idx);
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
            /* 短上行：叮—咚 */
            play_tone_ms(784, 70, 5000);
            play_tone_ms(988, 90, 5500);
            play_tone_ms(1175, 140, 6000);
            break;
        case SFX_JACKPOT:
            /* 明亮大三和弦爬升 */
            play_tone_ms(523, 80, 5500);
            play_tone_ms(659, 80, 5500);
            play_tone_ms(784, 100, 6000);
            play_tone_ms(1047, 180, 7000);
            play_tone_ms(1319, 120, 6500);
            break;
        case SFX_MEGA:
            /* 更长、更高亢的连击 */
            play_tone_ms(392, 70, 5500);
            play_tone_ms(523, 70, 5500);
            play_tone_ms(659, 70, 6000);
            play_tone_ms(784, 70, 6000);
            play_tone_ms(988, 90, 6500);
            play_tone_ms(1319, 160, 7500);
            play_tone_ms(1568, 200, 8000);
            break;
        case SFX_BONUS:
            /* 轻快跳音：神秘开箱感 */
            play_tone_ms(880, 50, 5000);
            play_tone_ms(0, 40, 0);
            play_tone_ms(1109, 50, 5500);
            play_tone_ms(0, 40, 0);
            play_tone_ms(1320, 50, 6000);
            play_tone_ms(0, 40, 0);
            play_tone_ms(1760, 160, 7000);
            break;
        case SFX_LEVEL:
            /* 升级：两段上升 fanfare */
            play_tone_ms(587, 80, 5500);
            play_tone_ms(740, 80, 5500);
            play_tone_ms(880, 80, 6000);
            play_tone_ms(1175, 120, 6500);
            play_tone_ms(880, 60, 5000);
            play_tone_ms(1175, 180, 7000);
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

static void hide_bonus_layer(void)
{
    if (s_bonus_layer) lv_obj_add_flag(s_bonus_layer, LV_OBJ_FLAG_HIDDEN);
}

static void start_bonus_anim(void)
{
    s_phase = SLOT_BONUS_ANIM;
    s_cele_tick = 0;
    if (s_bonus_layer) {
        lv_obj_remove_flag(s_bonus_layer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_bonus_layer);
    }
    if (s_bonus_lbl) lv_label_set_text(s_bonus_lbl, "BONUS!");
    queue_sfx(SFX_BONUS);
    refresh_status();
}

static void apply_spin_payout(void)
{
    int bet = slot_tier_bet(s_player.tier);
    s_last_win = slot_credits_won(bet, s_spin_res.mult_x10);
    s_credits += s_last_win;
    if (s_last_win > 0) s_lifetime_won += s_last_win;

    int new_lv = slot_level_from_lifetime_won(s_lifetime_won);
    s_pending_level = (new_lv > s_player.level) ? new_lv : 0;
    if (s_pending_level > 0) s_player.level = s_pending_level;

    /* 等级升高后，若当前档次未解锁则钳制到最高可用 */
    if (!slot_tier_unlocked(s_player.tier, s_player.level)) {
        s_player.tier = slot_highest_unlocked_tier(s_player.level);
    }

    maybe_save_records();
}

static void begin_celebrate_win(void)
{
    s_phase = SLOT_CELEBRATE;
    s_cele_tick = 0;
    s_cele_jackpot = s_spin_res.is_jackpot && !s_spin_res.is_mega;
    s_cele_mega = s_spin_res.is_mega;
    s_cele_bonus = s_spin_res.from_bonus;

    const char *msg = "WIN!";
    uint32_t bg = cur_theme()->flash_win;
    if (s_cele_mega) {
        msg = "MEGA!";
        bg = cur_theme()->flash_mega;
        queue_sfx(SFX_MEGA);
    } else if (s_cele_bonus) {
        msg = "BONUS!";
        bg = cur_theme()->flash_bonus;
        queue_sfx(SFX_BONUS);
    } else if (s_cele_jackpot) {
        msg = "JACKPOT!";
        bg = cur_theme()->flash_jackpot;
        queue_sfx(SFX_JACKPOT);
    } else {
        queue_sfx(SFX_WIN);
    }
    show_banner(msg, bg);
    refresh_status();
}

static void begin_level_up(int new_lv)
{
    s_phase = SLOT_LEVEL_UP;
    s_cele_tick = 0;
    s_pending_level = 0;
    char buf[24];
    snprintf(buf, sizeof(buf), "LV %d!", new_lv);
    show_banner(buf, cur_theme()->flash_level);
    queue_sfx(SFX_LEVEL);
    refresh_status();
}

static void finish_spin(void)
{
    /* 转轴已停在 s_target；若是 Bonus 入口则进独立动画页 */
    if (s_spin_res.needs_bonus) {
        start_bonus_anim();
        return;
    }

    apply_spin_payout();

    if (s_last_win > 0) {
        begin_celebrate_win();
    } else if (s_pending_level > 0) {
        begin_level_up(s_pending_level);
    } else {
        s_phase = SLOT_RESULT;
        hide_banner();
        set_cells_flash(false, 0);
        refresh_status();
    }
}

static void resolve_bonus_and_finish(void)
{
    hide_bonus_layer();
    if (!slot_bonus_spin(&s_player, esp_random(), esp_random(), &s_spin_res)) {
        s_phase = SLOT_RESULT;
        refresh_status();
        return;
    }
    for (int i = 0; i < 3; i++) {
        s_target[i] = s_spin_res.symbols[i];
        set_reel(i, s_target[i]);
    }
    apply_spin_payout();
    if (s_last_win > 0) begin_celebrate_win();
    else if (s_pending_level > 0) begin_level_up(s_pending_level);
    else {
        s_phase = SLOT_RESULT;
        refresh_status();
    }
}

static void spin_tick(lv_timer_t *t)
{
    (void)t;

    if (s_phase == SLOT_BONUS_ANIM) {
        s_cele_tick++;
        /* 约 1.2s 小动画后开 Bonus 池 */
        if (s_bonus_lbl && (s_cele_tick / 4) % 2 == 0) {
            lv_label_set_text(s_bonus_lbl, "BONUS!");
        } else if (s_bonus_lbl) {
            lv_label_set_text(s_bonus_lbl, "OPEN...");
        }
        if (s_cele_tick >= 24) resolve_bonus_and_finish();
        return;
    }

    if (s_phase == SLOT_CELEBRATE || s_phase == SLOT_LEVEL_UP) {
        s_cele_tick++;
        bool on = (s_cele_tick / 3) % 2 == 0;
        uint32_t col = cur_theme()->flash_win;
        if (s_phase == SLOT_LEVEL_UP) col = cur_theme()->flash_level;
        else if (s_cele_mega) col = cur_theme()->flash_mega;
        else if (s_cele_bonus) col = cur_theme()->flash_bonus;
        else if (s_cele_jackpot) col = cur_theme()->flash_jackpot;
        /* 只闪横幅底板，滚轴保持静止 */
        if (s_banner_bg) {
            lv_obj_set_style_bg_color(s_banner_bg, lv_color_hex(on ? col : 0xFFFFFF), 0);
        }
        if (s_cele_tick >= 24) {
            hide_banner();
            if (s_phase == SLOT_CELEBRATE && s_pending_level > 0) {
                begin_level_up(s_pending_level);
            } else {
                s_phase = SLOT_RESULT;
                refresh_status();
            }
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
    if (s_phase == SLOT_SPINNING || s_phase == SLOT_CELEBRATE
        || s_phase == SLOT_BONUS_ANIM || s_phase == SLOT_LEVEL_UP) return;

    int bet = slot_tier_bet(s_player.tier);
    if (!slot_tier_unlocked(s_player.tier, s_player.level)) {
        if (s_status_left) {
            lv_label_set_text_fmt(s_status_left, "Need Lv%d\nBet %d locked",
                                  slot_tier_unlock_level(s_player.tier), bet);
        }
        refresh_status_right();
        s_phase = SLOT_IDLE;
        return;
    }
    if (s_credits < bet) {
        if (s_status_left) {
            lv_label_set_text_fmt(s_status_left, "Credits %d\nNot enough!", s_credits);
        }
        refresh_status_right();
        s_phase = SLOT_IDLE;
        return;
    }

    if (!slot_spin(&s_player, esp_random(), esp_random(), esp_random(), &s_spin_res)) {
        s_phase = SLOT_IDLE;
        return;
    }

    s_credits -= bet;
    s_last_win = 0;
    s_pending_level = 0;
    s_spin_tick = 0;
    s_stop_at[0] = 12;
    s_stop_at[1] = 18;
    s_stop_at[2] = 24;
    for (int i = 0; i < 3; i++) s_target[i] = s_spin_res.symbols[i];

    s_phase = SLOT_SPINNING;
    hide_banner();
    hide_bonus_layer();
    refresh_status();
}

void demo_slots_reset_credits(void)
{
    /* 进 Demo：只刷新本局筹码，保留 NVS 里的等级进度 */
    s_credits = 500;
    s_phase = SLOT_IDLE;
    s_last_win = 0;
    s_pending_level = 0;
    set_cells_flash(false, 0);
    hide_banner();
    hide_bonus_layer();
    set_reel(0, SLOT_SYM_DIAMOND);
    set_reel(1, SLOT_SYM_DIAMOND);
    set_reel(2, SLOT_SYM_DIAMOND);
    refresh_status();
    refresh_config();
}

void demo_slots_reset_progress(void)
{
    /* OK 长按：重置筹码 + 等级/累计赢分/保底/档次/Best，并写入 NVS */
    s_credits = 500;
    s_lifetime_won = 0;
    s_pending_level = 0;
    s_last_win = 0;
    s_best_win = 0;
    s_peak_credits = 500;
    memset(&s_player, 0, sizeof(s_player));
    s_player.level = 1;
    s_player.tier = SLOT_TIER_BRONZE;
    s_phase = SLOT_IDLE;
    set_cells_flash(false, 0);
    hide_banner();
    hide_bonus_layer();
    set_reel(0, SLOT_SYM_DIAMOND);
    set_reel(1, SLOT_SYM_DIAMOND);
    set_reel(2, SLOT_SYM_DIAMOND);

    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_i32(h, NVS_KEY_LVL, 1);
        nvs_set_i32(h, NVS_KEY_LIFE, 0);
        nvs_set_i32(h, NVS_KEY_TIER, (int32_t)SLOT_TIER_BRONZE);
        nvs_set_i32(h, NVS_KEY_NWIN, 0);
        nvs_set_i32(h, NVS_KEY_NHI, 0);
        nvs_set_i32(h, NVS_KEY_NSP, 0);
        nvs_set_i32(h, NVS_KEY_NJP, 0);
        nvs_set_i32(h, NVS_KEY_BEST, 0);
        nvs_set_i32(h, NVS_KEY_PEAK, 500);
        nvs_commit(h);
        nvs_close(h);
    }
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

    /* 主题标题：紧凑左上，与状态条对齐 */
    s_title_shadow = ui_block(s_ui_layer, 8, 8, 86, 26, 0x000000);
    s_title_plate = make_pixel_chip(s_ui_layer, 6, 6, 86, 26, t0->title_bg, t0->border);
    s_title_lbl = lv_label_create(s_title_plate);
    lv_label_set_text(s_title_lbl, t0->title);
    lv_obj_set_style_text_font(s_title_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_title_lbl, lv_color_hex(t0->title_fg), 0);
    lv_obj_center(s_title_lbl);

    s_batt = lv_label_create(s_ui_layer);
    lv_obj_set_style_text_font(s_batt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_batt, lv_color_hex(t0->ink), 0);
    lv_obj_align(s_batt, LV_ALIGN_TOP_RIGHT, -8, 10);
    refresh_batt();

    /* 状态条：左 Credits/Bet，右 Lv/Best；固定两行，禁止换行撑成三行 */
    s_status_bar = make_pixel_chip(s_ui_layer, 8, 40, 224, 36, t0->reel, t0->accent);
    lv_obj_set_style_bg_opa(s_status_bar, LV_OPA_90, 0);
    s_status_left = lv_label_create(s_status_bar);
    lv_obj_set_width(s_status_left, 118);
    lv_label_set_long_mode(s_status_left, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(s_status_left, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_status_left, lv_color_hex(t0->status), 0);
    lv_obj_set_style_text_line_space(s_status_left, 0, 0);
    lv_obj_align(s_status_left, LV_ALIGN_LEFT_MID, 6, 0);
    s_status_right = lv_label_create(s_status_bar);
    lv_obj_set_width(s_status_right, 98);
    lv_label_set_long_mode(s_status_right, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(s_status_right, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_status_right, lv_color_hex(t0->status), 0);
    lv_obj_set_style_text_align(s_status_right, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_line_space(s_status_right, 0, 0);
    lv_obj_align(s_status_right, LV_ALIGN_RIGHT_MID, -6, 0);

    /* 转轴区：三格完全落在面板内（含阴影） */
    s_panel_glow = ui_block(s_ui_layer, 4, 80, 232, 180, t0->glow);
    s_panel_outline = ui_block(s_ui_layer, 8, 84, 224, 172, 0x000000);
    s_panel = make_flat_panel(s_ui_layer, 12, 88, 216, 164, t0->panel, t0->accent);
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_90, 0);
    lv_obj_set_style_border_width(s_panel, 4, 0);
    lv_obj_set_style_pad_all(s_panel, 0, 0);

    /* 面板外宽 216、边框 4×2 → 内宽 208；三格 54 + 间距 12 → 总 186，左右各 11 居中 */
    const int reel_w = 54;
    const int reel_gap = 12;
    const int reel_total = 3 * reel_w + 2 * reel_gap;
    const int reel_x0 = (216 - 8 - reel_total) / 2;
    for (int i = 0; i < 3; i++) {
        int x = reel_x0 + i * (reel_w + reel_gap);
        s_reel_cell[i] = make_flat_panel(s_panel, x, 16, reel_w, 120,
                                         t0->reel_hi, t0->accent);
        s_reel_icon[i] = lv_obj_create(s_reel_cell[i]);
        lv_obj_remove_flag(s_reel_icon[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(s_reel_icon[i], 40, 56);
        lv_obj_center(s_reel_icon[i]);
        lv_obj_set_style_bg_opa(s_reel_icon[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(s_reel_icon[i], 0, 0);
        lv_obj_set_style_pad_all(s_reel_icon[i], 0, 0);
        for (int p = 0; p < REEL_PIECES; p++) {
            s_reel_piece[i][p] = ui_block(s_reel_icon[i], 0, 0, 4, 4, 0x000000);
            lv_obj_add_flag(s_reel_piece[i][p], LV_OBJ_FLAG_HIDDEN);
        }
    }

    /* 中奖横幅：大色块底板 + 粗描边文字，庆祝时闪烁 */
    s_banner_bg = make_pixel_chip(s_ui_layer, 28, 140, 184, 44, t0->flash_jackpot, t0->border);
    lv_obj_set_style_border_width(s_banner_bg, 3, 0);
    lv_obj_add_flag(s_banner_bg, LV_OBJ_FLAG_HIDDEN);
    s_banner = lv_label_create(s_ui_layer);
    lv_obj_set_style_text_font(s_banner, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_banner, lv_color_hex(0x212121), 0);
    lv_obj_align(s_banner, LV_ALIGN_TOP_MID, 0, 150);
    lv_label_set_text(s_banner, "");
    lv_obj_add_flag(s_banner, LV_OBJ_FLAG_HIDDEN);

    /* Bonus 全屏小动画层 */
    s_bonus_layer = lv_obj_create(s_ui_layer);
    lv_obj_remove_flag(s_bonus_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_bonus_layer, 0, 0);
    lv_obj_set_size(s_bonus_layer, 240, 320);
    lv_obj_set_style_bg_color(s_bonus_layer, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(s_bonus_layer, LV_OPA_70, 0);
    lv_obj_set_style_border_width(s_bonus_layer, 0, 0);
    lv_obj_set_style_pad_all(s_bonus_layer, 0, 0);
    lv_obj_add_flag(s_bonus_layer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *bonus_chip = make_pixel_chip(s_bonus_layer, 28, 120, 184, 64,
                                           t0->flash_bonus, t0->border);
    lv_obj_set_style_border_width(bonus_chip, 3, 0);
    s_bonus_lbl = lv_label_create(bonus_chip);
    lv_label_set_text(s_bonus_lbl, "BONUS!");
    lv_obj_set_style_text_font(s_bonus_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_bonus_lbl, lv_color_hex(0x212121), 0);
    lv_obj_center(s_bonus_lbl);

    /* CONFIG：两列等宽 × 两行（左 Bet/BGM，右 Vol） */
    s_cfg_panel = make_flat_panel(s_ui_layer, 8, 268, 224, 40, t0->panel, t0->accent);
    lv_obj_set_style_bg_opa(s_cfg_panel, LV_OPA_90, 0);
    lv_obj_set_style_pad_all(s_cfg_panel, 4, 0);
    lv_obj_remove_flag(s_cfg_panel, LV_OBJ_FLAG_SCROLLABLE);
    s_cfg_left = lv_label_create(s_cfg_panel);
    lv_obj_set_width(s_cfg_left, 100);
    lv_label_set_long_mode(s_cfg_left, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(s_cfg_left, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_cfg_left, lv_color_hex(t0->ink), 0);
    lv_obj_set_style_text_line_space(s_cfg_left, 1, 0);
    lv_obj_align(s_cfg_left, LV_ALIGN_LEFT_MID, 0, 0);
    s_cfg_right = lv_label_create(s_cfg_panel);
    lv_obj_set_width(s_cfg_right, 100);
    lv_label_set_long_mode(s_cfg_right, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(s_cfg_right, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_cfg_right, lv_color_hex(t0->ink), 0);
    lv_obj_set_style_text_line_space(s_cfg_right, 1, 0);
    lv_obj_align(s_cfg_right, LV_ALIGN_RIGHT_MID, 0, 0);

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
        s_panel_glow = s_panel_outline = NULL;
        s_panel = s_status_bar = NULL;
        s_status_left = s_status_right = NULL;
        s_banner_bg = s_banner = s_batt = NULL;
        s_bonus_layer = s_bonus_lbl = NULL;
        s_cfg_panel = s_cfg_left = s_cfg_right = NULL;
        for (int i = 0; i < 3; i++) {
            s_reel_icon[i] = NULL;
            s_reel_cell[i] = NULL;
            for (int p = 0; p < REEL_PIECES; p++) s_reel_piece[i][p] = NULL;
        }
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

    if (s_phase == SLOT_SPINNING || s_phase == SLOT_CELEBRATE
        || s_phase == SLOT_BONUS_ANIM || s_phase == SLOT_LEVEL_UP) return;

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
            /* 上下循环全部档位下注 10/50/100/500/1000；未解锁档位开转时再拦截 */
            s_player.tier = (slot_tier_t)((s_player.tier + dir + SLOT_TIER_COUNT) % SLOT_TIER_COUNT);
            save_player_progress();
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
