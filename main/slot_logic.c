#include "slot_logic.h"

#include <stddef.h>

typedef struct {
    slot_reward_t reward;
    uint16_t weight;
    int mult_x10;
    int sym[3]; /* 若全 -1 则按 reward 生成图案 */
} slot_outcome_t;

/* ---------- 档次元数据 ---------- */

int slot_tier_unlock_level(slot_tier_t tier)
{
    static const int unlock[SLOT_TIER_COUNT] = { 1, 3, 5, 8, 10 };
    if (tier < 0 || tier >= SLOT_TIER_COUNT) return 99;
    return unlock[tier];
}

int slot_tier_bet(slot_tier_t tier)
{
    static const int bet[SLOT_TIER_COUNT] = { 10, 50, 100, 500, 1000 };
    if (tier < 0 || tier >= SLOT_TIER_COUNT) return 10;
    return bet[tier];
}

int slot_tier_high_threshold_x10(slot_tier_t tier)
{
    /* GOLD:5x  DIAMOND:10x  MASTER:20x；BRONZE/SILVER 无高奖保底 → 0 */
    static const int thr[SLOT_TIER_COUNT] = { 0, 0, 50, 100, 200 };
    if (tier < 0 || tier >= SLOT_TIER_COUNT) return 0;
    return thr[tier];
}

bool slot_tier_unlocked(slot_tier_t tier, int level)
{
    return level >= slot_tier_unlock_level(tier);
}

slot_tier_t slot_highest_unlocked_tier(int level)
{
    slot_tier_t best = SLOT_TIER_BRONZE;
    for (int t = 0; t < SLOT_TIER_COUNT; t++) {
        if (slot_tier_unlocked((slot_tier_t)t, level)) best = (slot_tier_t)t;
    }
    return best;
}

/* 累计赢分门槛：级差递增；Lv3/5/8/10 对齐档次解锁 */
int slot_lifetime_needed_for_level(int level)
{
    /* 累计：每升一级所需增量大致按 ~1.5–2× 拉长 */
    static const int need[] = {
        0,      /* lv1 */
        200,    /* lv2  +200 */
        500,    /* lv3  +300  → SILVER */
        1000,   /* lv4  +500 */
        2000,   /* lv5  +1000 → GOLD */
        4000,   /* lv6  +2000 */
        7000,   /* lv7  +3000 */
        12000,  /* lv8  +5000 → DIAMOND */
        20000,  /* lv9  +8000 */
        35000,  /* lv10 +15000 → MASTER */
    };
    if (level <= 1) return 0;
    if (level <= 10) return need[level - 1];

    /* Lv10 之后：首档 +20000，之后每级再多 +5000 */
    int total = need[9];
    for (int k = 1; k <= level - 10; k++) {
        total += 20000 + (k - 1) * 5000;
    }
    return total;
}

int slot_level_from_lifetime_won(int lifetime_won)
{
    if (lifetime_won < 0) lifetime_won = 0;
    int lv = 1;
    for (int n = 2; n <= 99; n++) {
        if (lifetime_won >= slot_lifetime_needed_for_level(n)) lv = n;
        else break;
    }
    return lv;
}

int slot_credits_won(int bet, int mult_x10)
{
    if (bet <= 0 || mult_x10 <= 0) return 0;
    return (bet * mult_x10) / 10;
}

bool slot_is_special_reward(slot_reward_t r)
{
    return r == SLOT_REWARD_SPECIAL_8 || r == SLOT_REWARD_SPECIAL_15
        || r == SLOT_REWARD_JACKPOT_50 || r == SLOT_REWARD_JACKPOT_100
        || r == SLOT_REWARD_MEGA_200 || r == SLOT_REWARD_MEGA_500
        || r == SLOT_REWARD_BONUS_GATE;
}

static bool reward_is_jackpot_or_mega(slot_reward_t r)
{
    return r == SLOT_REWARD_JACKPOT_50 || r == SLOT_REWARD_JACKPOT_100
        || r == SLOT_REWARD_MEGA_200 || r == SLOT_REWARD_MEGA_500;
}

/* ---------- 图案填充 ---------- */

static void fill_triple(int *s, int sym)
{
    s[0] = s[1] = s[2] = sym;
}

static void fill_special_8(int *s)
{
    s[0] = SLOT_SYM_CHERRY;
    s[1] = SLOT_SYM_STAR;
    s[2] = SLOT_SYM_DIAMOND;
}

static void fill_special_15(int *s)
{
    s[0] = SLOT_SYM_STAR;
    s[1] = SLOT_SYM_DIAMOND;
    s[2] = SLOT_SYM_STAR;
}

/* 未中奖：三枚互不相同且非特殊组合 */
static void fill_miss(int *s, uint32_t r)
{
    static const int bag[] = {
        SLOT_SYM_CHERRY, SLOT_SYM_LEMON, SLOT_SYM_BELL, SLOT_SYM_STAR, SLOT_SYM_DIAMOND
    };
    const int n = (int)(sizeof(bag) / sizeof(bag[0]));
    s[0] = bag[r % (uint32_t)n];
    s[1] = bag[(r / 7u) % (uint32_t)n];
    s[2] = bag[(r / 49u) % (uint32_t)n];
    /* 打散成对/三同 */
    if (s[0] == s[1]) s[1] = bag[(s[1] + 1) % n];
    if (s[1] == s[2]) s[2] = bag[(s[2] + 1) % n];
    if (s[0] == s[2]) s[2] = bag[(s[2] + 2) % n];
    if (s[0] == s[1] || s[1] == s[2] || s[0] == s[2]) {
        s[0] = SLOT_SYM_CHERRY;
        s[1] = SLOT_SYM_LEMON;
        s[2] = SLOT_SYM_BELL;
    }
    /* 避免撞上 🍒⭐💎 / ⭐💎⭐ */
    if ((s[0] == SLOT_SYM_CHERRY && s[1] == SLOT_SYM_STAR && s[2] == SLOT_SYM_DIAMOND)
        || (s[0] == SLOT_SYM_STAR && s[1] == SLOT_SYM_DIAMOND && s[2] == SLOT_SYM_STAR)) {
        s[2] = SLOT_SYM_LEMON;
    }
}

static void fill_pair(int *s, uint32_t r)
{
    static const int bag[] = {
        SLOT_SYM_CHERRY, SLOT_SYM_LEMON, SLOT_SYM_BELL, SLOT_SYM_STAR
    };
    int a = bag[r % 4u];
    int b = bag[(r / 5u) % 4u];
    if (b == a) b = bag[(a + 1) % 4];
    int pos = (int)((r / 17u) % 3u);
    if (pos == 0) { s[0] = a; s[1] = a; s[2] = b; }
    else if (pos == 1) { s[0] = a; s[1] = b; s[2] = a; }
    else { s[0] = b; s[1] = a; s[2] = a; }
}

static void apply_outcome_symbols(const slot_outcome_t *o, int *s, uint32_t r)
{
    if (o->sym[0] >= 0) {
        s[0] = o->sym[0];
        s[1] = o->sym[1];
        s[2] = o->sym[2];
        return;
    }
    switch (o->reward) {
    case SLOT_REWARD_NONE:
        fill_miss(s, r);
        break;
    case SLOT_REWARD_PAIR:
        fill_pair(s, r);
        break;
    case SLOT_REWARD_THREE_CHERRY:
        fill_triple(s, SLOT_SYM_CHERRY);
        break;
    case SLOT_REWARD_THREE_LEMON:
        fill_triple(s, SLOT_SYM_LEMON);
        break;
    case SLOT_REWARD_THREE_BELL:
        fill_triple(s, SLOT_SYM_BELL);
        break;
    case SLOT_REWARD_THREE_STAR:
        fill_triple(s, SLOT_SYM_STAR);
        break;
    case SLOT_REWARD_SPECIAL_8:
        fill_special_8(s);
        break;
    case SLOT_REWARD_SPECIAL_15:
        fill_special_15(s);
        break;
    case SLOT_REWARD_JACKPOT_50:
        fill_triple(s, SLOT_SYM_DIAMOND);
        break;
    case SLOT_REWARD_JACKPOT_100:
        /* 普通池 WILD；Jackpot 保底里也有 Mega Small(100x 👑) 由显式 sym 指定 */
        fill_triple(s, SLOT_SYM_WILD);
        break;
    case SLOT_REWARD_MEGA_200:
    case SLOT_REWARD_MEGA_500:
        fill_triple(s, SLOT_SYM_MEGA);
        break;
    case SLOT_REWARD_BONUS_GATE:
        fill_triple(s, SLOT_SYM_BONUS);
        break;
    default:
        fill_miss(s, r);
        break;
    }
}

#define O(reward, w, mx, a, b, c) { (reward), (w), (mx), { (a), (b), (c) } }
#define OG(reward, w, mx) O(reward, w, mx, -1, -1, -1)

/* ---------- 普通池 ---------- */

static const slot_outcome_t POOL_BRONZE[] = {
    OG(SLOT_REWARD_NONE, 500, 0),
    OG(SLOT_REWARD_PAIR, 250, 15),
    OG(SLOT_REWARD_THREE_CHERRY, 50, 30),
    OG(SLOT_REWARD_THREE_LEMON, 25, 50),
    OG(SLOT_REWARD_THREE_BELL, 10, 100),
    OG(SLOT_REWARD_SPECIAL_8, 10, 80),
    OG(SLOT_REWARD_JACKPOT_50, 3, 500),
    OG(SLOT_REWARD_BONUS_GATE, 2, 0),
};

static const slot_outcome_t POOL_SILVER[] = {
    OG(SLOT_REWARD_NONE, 400, 0),
    OG(SLOT_REWARD_PAIR, 280, 15),
    OG(SLOT_REWARD_THREE_CHERRY, 80, 30),
    OG(SLOT_REWARD_THREE_LEMON, 50, 50),
    OG(SLOT_REWARD_THREE_BELL, 25, 100),
    OG(SLOT_REWARD_SPECIAL_8, 20, 80),
    OG(SLOT_REWARD_SPECIAL_15, 10, 150),
    OG(SLOT_REWARD_JACKPOT_50, 5, 500),
    OG(SLOT_REWARD_BONUS_GATE, 5, 0),
};

static const slot_outcome_t POOL_GOLD[] = {
    OG(SLOT_REWARD_NONE, 250, 0),
    OG(SLOT_REWARD_PAIR, 300, 15),
    OG(SLOT_REWARD_THREE_CHERRY, 100, 30),
    OG(SLOT_REWARD_THREE_LEMON, 80, 50),
    OG(SLOT_REWARD_THREE_BELL, 50, 100),
    OG(SLOT_REWARD_THREE_STAR, 25, 200),
    OG(SLOT_REWARD_SPECIAL_8, 30, 80),
    OG(SLOT_REWARD_SPECIAL_15, 20, 150),
    OG(SLOT_REWARD_JACKPOT_50, 10, 500),
    OG(SLOT_REWARD_JACKPOT_100, 5, 1000),
    OG(SLOT_REWARD_BONUS_GATE, 10, 0),
};

static const slot_outcome_t POOL_DIAMOND[] = {
    OG(SLOT_REWARD_NONE, 200, 0),
    OG(SLOT_REWARD_PAIR, 320, 15),
    OG(SLOT_REWARD_THREE_CHERRY, 120, 30),
    OG(SLOT_REWARD_THREE_LEMON, 100, 50),
    OG(SLOT_REWARD_THREE_BELL, 70, 100),
    OG(SLOT_REWARD_THREE_STAR, 40, 200),
    OG(SLOT_REWARD_SPECIAL_8, 40, 80),
    OG(SLOT_REWARD_SPECIAL_15, 30, 150),
    OG(SLOT_REWARD_JACKPOT_50, 15, 500),
    OG(SLOT_REWARD_JACKPOT_100, 10, 1000),
    OG(SLOT_REWARD_BONUS_GATE, 15, 0),
};

static const slot_outcome_t POOL_MASTER[] = {
    OG(SLOT_REWARD_NONE, 170, 0),
    OG(SLOT_REWARD_PAIR, 330, 15),
    OG(SLOT_REWARD_THREE_CHERRY, 150, 30),
    OG(SLOT_REWARD_THREE_LEMON, 120, 50),
    OG(SLOT_REWARD_THREE_BELL, 90, 100),
    OG(SLOT_REWARD_THREE_STAR, 70, 200),
    OG(SLOT_REWARD_SPECIAL_8, 50, 80),
    OG(SLOT_REWARD_SPECIAL_15, 40, 150),
    OG(SLOT_REWARD_JACKPOT_50, 20, 500),
    OG(SLOT_REWARD_JACKPOT_100, 15, 1000),
    OG(SLOT_REWARD_BONUS_GATE, 20, 0),
    OG(SLOT_REWARD_MEGA_200, 5, 2000),
};

/* ---------- 保底池 ---------- */

static const slot_outcome_t POOL_SILVER_WIN_G[] = {
    OG(SLOT_REWARD_THREE_CHERRY, 40, 30),
    OG(SLOT_REWARD_THREE_LEMON, 30, 50),
    OG(SLOT_REWARD_THREE_BELL, 20, 100),
    OG(SLOT_REWARD_SPECIAL_8, 10, 80),
};

static const slot_outcome_t POOL_GOLD_LOW_G[] = {
    OG(SLOT_REWARD_THREE_LEMON, 35, 50),
    OG(SLOT_REWARD_SPECIAL_8, 25, 80),
    OG(SLOT_REWARD_THREE_BELL, 25, 100),
    OG(SLOT_REWARD_SPECIAL_15, 15, 150),
};

static const slot_outcome_t POOL_GOLD_SPECIAL_G[] = {
    OG(SLOT_REWARD_THREE_STAR, 10, 200),
    OG(SLOT_REWARD_JACKPOT_50, 45, 500),
    OG(SLOT_REWARD_JACKPOT_100, 25, 1000),
    OG(SLOT_REWARD_BONUS_GATE, 20, 0),
};

static const slot_outcome_t POOL_DIAMOND_WIN_G[] = {
    OG(SLOT_REWARD_THREE_LEMON, 30, 50),
    OG(SLOT_REWARD_SPECIAL_8, 25, 80),
    OG(SLOT_REWARD_THREE_BELL, 20, 100),
    OG(SLOT_REWARD_SPECIAL_15, 15, 150),
    OG(SLOT_REWARD_THREE_STAR, 7, 200),
    OG(SLOT_REWARD_JACKPOT_50, 3, 500),
};

static const slot_outcome_t POOL_DIAMOND_HIGH_G[] = {
    OG(SLOT_REWARD_THREE_BELL, 35, 100),
    OG(SLOT_REWARD_SPECIAL_15, 25, 150),
    OG(SLOT_REWARD_THREE_STAR, 20, 200),
    OG(SLOT_REWARD_JACKPOT_50, 15, 500),
    OG(SLOT_REWARD_JACKPOT_100, 5, 1000),
};

static const slot_outcome_t POOL_DIAMOND_SPECIAL_G[] = {
    OG(SLOT_REWARD_JACKPOT_50, 40, 500),
    OG(SLOT_REWARD_JACKPOT_100, 30, 1000),
    OG(SLOT_REWARD_BONUS_GATE, 20, 0),
    OG(SLOT_REWARD_MEGA_200, 10, 2000),
};

static const slot_outcome_t POOL_MASTER_WIN_G[] = {
    OG(SLOT_REWARD_THREE_LEMON, 35, 50),
    OG(SLOT_REWARD_SPECIAL_8, 25, 80),
    OG(SLOT_REWARD_THREE_BELL, 20, 100),
    OG(SLOT_REWARD_SPECIAL_15, 10, 150),
    OG(SLOT_REWARD_THREE_STAR, 10, 200),
};

static const slot_outcome_t POOL_MASTER_HIGH_G[] = {
    OG(SLOT_REWARD_THREE_STAR, 45, 200),
    OG(SLOT_REWARD_JACKPOT_50, 30, 500),
    OG(SLOT_REWARD_JACKPOT_100, 20, 1000),
    OG(SLOT_REWARD_MEGA_200, 5, 2000),
};

static const slot_outcome_t POOL_MASTER_SPECIAL_G[] = {
    OG(SLOT_REWARD_JACKPOT_50, 35, 500),
    OG(SLOT_REWARD_JACKPOT_100, 30, 1000),
    OG(SLOT_REWARD_BONUS_GATE, 25, 0),
    OG(SLOT_REWARD_MEGA_200, 10, 2000),
};

/* MASTER Jackpot 保底：含 Mega Small(100x 👑) */
static const slot_outcome_t POOL_JACKPOT_G[] = {
    OG(SLOT_REWARD_JACKPOT_50, 35, 500),
    OG(SLOT_REWARD_JACKPOT_100, 30, 1000),
    O(SLOT_REWARD_JACKPOT_100, 20, 1000, SLOT_SYM_MEGA, SLOT_SYM_MEGA, SLOT_SYM_MEGA),
    OG(SLOT_REWARD_MEGA_200, 10, 2000),
    OG(SLOT_REWARD_MEGA_500, 5, 5000),
};

/* Bonus 奖励池（文档未给细表：给一组高价值结果） */
static const slot_outcome_t POOL_BONUS[] = {
    OG(SLOT_REWARD_THREE_STAR, 30, 200),
    OG(SLOT_REWARD_JACKPOT_50, 30, 500),
    OG(SLOT_REWARD_JACKPOT_100, 25, 1000),
    OG(SLOT_REWARD_MEGA_200, 10, 2000),
    OG(SLOT_REWARD_MEGA_500, 5, 5000),
};

typedef struct {
    const slot_outcome_t *items;
    int count;
} slot_pool_view_t;

#define VIEW(arr) ((slot_pool_view_t){ (arr), (int)(sizeof(arr) / sizeof((arr)[0])) })

static slot_pool_view_t normal_pool(slot_tier_t tier)
{
    switch (tier) {
    case SLOT_TIER_BRONZE:  return VIEW(POOL_BRONZE);
    case SLOT_TIER_SILVER:  return VIEW(POOL_SILVER);
    case SLOT_TIER_GOLD:    return VIEW(POOL_GOLD);
    case SLOT_TIER_DIAMOND: return VIEW(POOL_DIAMOND);
    case SLOT_TIER_MASTER:  return VIEW(POOL_MASTER);
    default:                return VIEW(POOL_BRONZE);
    }
}

static int pick_weighted(const slot_outcome_t *items, int n, uint32_t r)
{
    uint32_t sum = 0;
    for (int i = 0; i < n; i++) sum += items[i].weight;
    if (sum == 0) return 0;
    uint32_t x = r % sum;
    uint32_t acc = 0;
    for (int i = 0; i < n; i++) {
        acc += items[i].weight;
        if (x < acc) return i;
    }
    return n - 1;
}

/* 保底优先级：Jackpot > Special > High > Win > Normal */
static void select_pool(const slot_player_t *p, slot_pool_t *pool_id, slot_pool_view_t *view)
{
    slot_tier_t t = p->tier;
    const slot_pity_t *c = &p->pity;

    if (t == SLOT_TIER_MASTER && c->no_jackpot >= 30) {
        *pool_id = SLOT_POOL_JACKPOT_GUARANTEE;
        *view = VIEW(POOL_JACKPOT_G);
        return;
    }
    if (t == SLOT_TIER_GOLD && c->no_special >= 15) {
        *pool_id = SLOT_POOL_SPECIAL_GUARANTEE;
        *view = VIEW(POOL_GOLD_SPECIAL_G);
        return;
    }
    if (t == SLOT_TIER_DIAMOND && c->no_special >= 20) {
        *pool_id = SLOT_POOL_SPECIAL_GUARANTEE;
        *view = VIEW(POOL_DIAMOND_SPECIAL_G);
        return;
    }
    if (t == SLOT_TIER_MASTER && c->no_special >= 15) {
        *pool_id = SLOT_POOL_SPECIAL_GUARANTEE;
        *view = VIEW(POOL_MASTER_SPECIAL_G);
        return;
    }
    if (t == SLOT_TIER_GOLD && c->no_high >= 5) {
        *pool_id = SLOT_POOL_HIGH_GUARANTEE;
        *view = VIEW(POOL_GOLD_LOW_G);
        return;
    }
    if (t == SLOT_TIER_DIAMOND && c->no_high >= 10) {
        *pool_id = SLOT_POOL_HIGH_GUARANTEE;
        *view = VIEW(POOL_DIAMOND_HIGH_G);
        return;
    }
    if (t == SLOT_TIER_MASTER && c->no_high >= 8) {
        *pool_id = SLOT_POOL_HIGH_GUARANTEE;
        *view = VIEW(POOL_MASTER_HIGH_G);
        return;
    }
    if (t == SLOT_TIER_SILVER && c->no_win >= 10) {
        *pool_id = SLOT_POOL_WIN_GUARANTEE;
        *view = VIEW(POOL_SILVER_WIN_G);
        return;
    }
    if (t == SLOT_TIER_DIAMOND && c->no_win >= 5) {
        *pool_id = SLOT_POOL_WIN_GUARANTEE;
        *view = VIEW(POOL_DIAMOND_WIN_G);
        return;
    }
    if (t == SLOT_TIER_MASTER && c->no_win >= 3) {
        *pool_id = SLOT_POOL_WIN_GUARANTEE;
        *view = VIEW(POOL_MASTER_WIN_G);
        return;
    }

    *pool_id = SLOT_POOL_NORMAL;
    *view = normal_pool(t);
}

static void update_pity(slot_player_t *p, const slot_spin_result_t *res)
{
    slot_pity_t *c = &p->pity;
    int high = slot_tier_high_threshold_x10(p->tier);

    if (res->mult_x10 > 0) c->no_win = 0;
    else c->no_win++;

    if (high > 0) {
        if (res->mult_x10 >= high) c->no_high = 0;
        else c->no_high++;
    } else {
        c->no_high = 0;
    }

    if (slot_is_special_reward(res->reward) || res->from_bonus
        || res->is_jackpot || res->is_mega) {
        c->no_special = 0;
    } else {
        c->no_special++;
    }

    if (p->tier == SLOT_TIER_MASTER) {
        if (reward_is_jackpot_or_mega(res->reward) || res->is_jackpot || res->is_mega)
            c->no_jackpot = 0;
        else
            c->no_jackpot++;
    } else {
        c->no_jackpot = 0;
    }
}

static void finalize_flags(slot_spin_result_t *out)
{
    out->is_jackpot = (out->reward == SLOT_REWARD_JACKPOT_50
                       || out->reward == SLOT_REWARD_JACKPOT_100);
    out->is_mega = (out->reward == SLOT_REWARD_MEGA_200
                    || out->reward == SLOT_REWARD_MEGA_500
                    || (out->symbols[0] == SLOT_SYM_MEGA
                        && out->symbols[1] == SLOT_SYM_MEGA
                        && out->symbols[2] == SLOT_SYM_MEGA));
    /* Mega Small 记为 jackpot+mega 展示 */
    if (out->symbols[0] == SLOT_SYM_MEGA && out->mult_x10 >= 1000)
        out->is_mega = true;
}

bool slot_spin(slot_player_t *player, uint32_t r0, uint32_t r1, uint32_t r2,
               slot_spin_result_t *out)
{
    (void)r2;
    if (!player || !out) return false;
    if (!slot_tier_unlocked(player->tier, player->level)) return false;

    slot_pool_t pool_id = SLOT_POOL_NORMAL;
    slot_pool_view_t view;
    select_pool(player, &pool_id, &view);

    int idx = pick_weighted(view.items, view.count, r0);
    const slot_outcome_t *o = &view.items[idx];

    out->pool_used = pool_id;
    out->reward = o->reward;
    out->mult_x10 = o->mult_x10;
    out->from_bonus = false;
    out->needs_bonus = false;
    apply_outcome_symbols(o, out->symbols, r1);
    finalize_flags(out);

    /* Bonus 入口：交给 UI 播小动画，再调 slot_bonus_spin；此处不改 pity */
    if (out->reward == SLOT_REWARD_BONUS_GATE) {
        out->needs_bonus = true;
        out->mult_x10 = 0;
        out->is_jackpot = false;
        out->is_mega = false;
        return true;
    }

    update_pity(player, out);
    return true;
}

bool slot_bonus_spin(slot_player_t *player, uint32_t r0, uint32_t r1,
                     slot_spin_result_t *out)
{
    if (!player || !out) return false;

    slot_pool_view_t bonus = VIEW(POOL_BONUS);
    int bi = pick_weighted(bonus.items, bonus.count, r0);
    const slot_outcome_t *bo = &bonus.items[bi];

    out->pool_used = SLOT_POOL_BONUS;
    out->reward = bo->reward;
    out->mult_x10 = bo->mult_x10;
    out->needs_bonus = false;
    out->from_bonus = true;
    apply_outcome_symbols(bo, out->symbols, r1);
    finalize_flags(out);
    update_pity(player, out);
    return true;
}

/* ---------- 展示用判奖 ---------- */

int slot_payout_mult_x10(int a, int b, int c)
{
    if (a < 0 || b < 0 || c < 0) return 0;
    if (a >= SLOT_SYM_COUNT || b >= SLOT_SYM_COUNT || c >= SLOT_SYM_COUNT) return 0;

    if (a == SLOT_SYM_CHERRY && b == SLOT_SYM_STAR && c == SLOT_SYM_DIAMOND) return 80;
    if (a == SLOT_SYM_STAR && b == SLOT_SYM_DIAMOND && c == SLOT_SYM_STAR) return 150;

    if (a == b && b == c) {
        switch (a) {
        case SLOT_SYM_CHERRY:  return 30;
        case SLOT_SYM_LEMON:   return 50;
        case SLOT_SYM_BELL:    return 100;
        case SLOT_SYM_STAR:    return 200;
        case SLOT_SYM_DIAMOND: return 500;
        case SLOT_SYM_WILD:    return 1000;
        case SLOT_SYM_MEGA:    return 2000; /* 展示默认；开奖以结果池倍率为准 */
        case SLOT_SYM_BONUS:   return 0;
        default: return 0;
        }
    }
    if (a == b || b == c || a == c) return 15;
    return 0;
}

int slot_payout_mult(int a, int b, int c)
{
    int x10 = slot_payout_mult_x10(a, b, c);
    return x10 / 10; /* PAIR 1.5 → 1（旧 API 近似） */
}

bool slot_is_free_trigger(int a, int b, int c)
{
    (void)a; (void)b; (void)c;
    return false; /* Lucky Triple 无免费转；Bonus 走结果池 */
}

bool slot_is_jackpot(int a, int b, int c)
{
    return (a == b && b == c
            && (a == SLOT_SYM_DIAMOND || a == SLOT_SYM_WILD));
}

bool slot_is_mega(int a, int b, int c)
{
    return a == SLOT_SYM_MEGA && b == SLOT_SYM_MEGA && c == SLOT_SYM_MEGA;
}

int slot_symbol_from_rand(uint32_t r)
{
    return (int)(r % (uint32_t)SLOT_SYM_COUNT);
}
