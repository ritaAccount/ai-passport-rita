#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Lucky Triple Slot：结果池 + 权重开奖（非三轴独立随机）。 */

enum {
    SLOT_SYM_CHERRY = 0,
    SLOT_SYM_LEMON,
    SLOT_SYM_BELL,
    SLOT_SYM_STAR,
    SLOT_SYM_DIAMOND,   /* Jackpot 💎 */
    SLOT_SYM_WILD,      /* 高级 Jackpot 🃏 */
    SLOT_SYM_BONUS,     /* Bonus 入口 🎁 */
    SLOT_SYM_MEGA,      /* Mega Jackpot 👑 */
    SLOT_SYM_COUNT
};

/* 兼容旧测试/过渡：SEVEN 等价 DIAMOND */
#define SLOT_SYM_SEVEN SLOT_SYM_DIAMOND

typedef enum {
    SLOT_TIER_BRONZE = 0,
    SLOT_TIER_SILVER,
    SLOT_TIER_GOLD,
    SLOT_TIER_DIAMOND,
    SLOT_TIER_MASTER,
    SLOT_TIER_COUNT
} slot_tier_t;

typedef enum {
    SLOT_REWARD_NONE = 0,
    SLOT_REWARD_PAIR,          /* 1.5x */
    SLOT_REWARD_THREE_CHERRY,  /* 3x */
    SLOT_REWARD_THREE_LEMON,   /* 5x */
    SLOT_REWARD_THREE_BELL,    /* 10x */
    SLOT_REWARD_THREE_STAR,    /* 20x */
    SLOT_REWARD_SPECIAL_8,     /* 🍒⭐💎 */
    SLOT_REWARD_SPECIAL_15,    /* ⭐💎⭐ */
    SLOT_REWARD_JACKPOT_50,    /* 💎💎💎 */
    SLOT_REWARD_JACKPOT_100,   /* 🃏🃏🃏 或 Mega Small */
    SLOT_REWARD_MEGA_200,
    SLOT_REWARD_MEGA_500,
    SLOT_REWARD_BONUS_GATE,    /* 抽中 BONUS 入口（内部会再进 Bonus 池） */
} slot_reward_t;

typedef enum {
    SLOT_POOL_NORMAL = 0,
    SLOT_POOL_WIN_GUARANTEE,
    SLOT_POOL_HIGH_GUARANTEE,
    SLOT_POOL_SPECIAL_GUARANTEE,
    SLOT_POOL_JACKPOT_GUARANTEE,
    SLOT_POOL_BONUS,
} slot_pool_t;

/* 保底计数器（按档次独立，换档可重置或由调用方管理） */
typedef struct {
    int no_win;       /* 连续未获得金币奖励 */
    int no_high;      /* 连续未达到高奖励阈值 */
    int no_special;   /* 连续未获得特殊（Bonus/Jackpot/Mega） */
    int no_jackpot;   /* 连续未获得 Jackpot/Mega（MASTER） */
} slot_pity_t;

typedef struct {
    int level;            /* 玩家等级，用于解锁档次 */
    slot_tier_t tier;     /* 当前下注档次 */
    slot_pity_t pity;
} slot_player_t;

typedef struct {
    int symbols[3];
    int mult_x10;           /* 倍率 ×10：15=1.5x，30=3x，5000=500x */
    slot_reward_t reward;
    slot_pool_t pool_used;  /* 实际使用的结果池 */
    bool is_jackpot;        /* 💎/🃏 类大奖展示 */
    bool is_mega;           /* 👑 Mega 展示 */
    bool needs_bonus;       /* 抽中 Bonus 入口，需再调 slot_bonus_spin */
    bool from_bonus;        /* 本局经 Bonus 池结算 */
} slot_spin_result_t;

/* 档次：解锁等级、单次下注、高奖励阈值（×10） */
int slot_tier_unlock_level(slot_tier_t tier);
int slot_tier_bet(slot_tier_t tier);
int slot_tier_high_threshold_x10(slot_tier_t tier);

bool slot_tier_unlocked(slot_tier_t tier, int level);
slot_tier_t slot_highest_unlocked_tier(int level);

/* 累计赢分 → 等级：达到阈值才升级。返回处于该累计赢分时的等级。 */
int slot_level_from_lifetime_won(int lifetime_won);
/* 升到 next_level 所需的累计赢分下限（next_level>=2）。 */
int slot_lifetime_needed_for_level(int level);

/* 校验档次合法后执行开奖；若 needs_bonus，不更新 pity，需再调 slot_bonus_spin。 */
bool slot_spin(slot_player_t *player, uint32_t r0, uint32_t r1, uint32_t r2,
               slot_spin_result_t *out);

/* Bonus 小游戏结算：从 Bonus 池抽最终结果并更新 pity。 */
bool slot_bonus_spin(slot_player_t *player, uint32_t r0, uint32_t r1,
                     slot_spin_result_t *out);

/* 根据倍率×10 与下注算赢分（向下取整）。 */
int slot_credits_won(int bet, int mult_x10);

/* 三图标判奖（调试/展示用；正式开奖以结果池为准）。 */
int slot_payout_mult_x10(int a, int b, int c);
bool slot_is_free_trigger(int a, int b, int c); /* 保留：现为 false（无免费转） */
bool slot_is_jackpot(int a, int b, int c);
bool slot_is_mega(int a, int b, int c);
bool slot_is_special_reward(slot_reward_t r);

int slot_symbol_from_rand(uint32_t r);

/* 旧 API：整倍率近似（PAIR→1，其余 /10）。新代码请用 mult_x10。 */
int slot_payout_mult(int a, int b, int c);
