#include "slot_logic.h"

#include <stdio.h>
#include <string.h>

static int fails;

static void expect_eq(const char *name, int got, int want)
{
    if (got != want) {
        fprintf(stderr, "FAIL %s: got %d want %d\n", name, got, want);
        fails++;
    }
}

static void expect_true(const char *name, int cond)
{
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", name);
        fails++;
    }
}

static void test_payout_table(void)
{
    expect_eq("cherry3", slot_payout_mult_x10(SLOT_SYM_CHERRY, SLOT_SYM_CHERRY, SLOT_SYM_CHERRY), 30);
    expect_eq("lemon3", slot_payout_mult_x10(SLOT_SYM_LEMON, SLOT_SYM_LEMON, SLOT_SYM_LEMON), 50);
    expect_eq("bell3", slot_payout_mult_x10(SLOT_SYM_BELL, SLOT_SYM_BELL, SLOT_SYM_BELL), 100);
    expect_eq("star3", slot_payout_mult_x10(SLOT_SYM_STAR, SLOT_SYM_STAR, SLOT_SYM_STAR), 200);
    expect_eq("diamond3", slot_payout_mult_x10(SLOT_SYM_DIAMOND, SLOT_SYM_DIAMOND, SLOT_SYM_DIAMOND), 500);
    expect_eq("wild3", slot_payout_mult_x10(SLOT_SYM_WILD, SLOT_SYM_WILD, SLOT_SYM_WILD), 1000);
    expect_eq("special8", slot_payout_mult_x10(SLOT_SYM_CHERRY, SLOT_SYM_STAR, SLOT_SYM_DIAMOND), 80);
    expect_eq("special15", slot_payout_mult_x10(SLOT_SYM_STAR, SLOT_SYM_DIAMOND, SLOT_SYM_STAR), 150);
    expect_eq("pair", slot_payout_mult_x10(SLOT_SYM_BELL, SLOT_SYM_BELL, SLOT_SYM_STAR), 15);
    expect_eq("none", slot_payout_mult_x10(SLOT_SYM_CHERRY, SLOT_SYM_LEMON, SLOT_SYM_BELL), 0);
    expect_eq("credits 1.5x", slot_credits_won(100, 15), 150);
    expect_eq("credits 50x", slot_credits_won(10, 500), 500);
    expect_true("jackpot diamond", slot_is_jackpot(SLOT_SYM_DIAMOND, SLOT_SYM_DIAMOND, SLOT_SYM_DIAMOND));
    expect_true("mega", slot_is_mega(SLOT_SYM_MEGA, SLOT_SYM_MEGA, SLOT_SYM_MEGA));
    expect_true("no free", !slot_is_free_trigger(SLOT_SYM_STAR, SLOT_SYM_STAR, SLOT_SYM_STAR));
}

static void test_tier_meta(void)
{
    expect_eq("bronze unlock", slot_tier_unlock_level(SLOT_TIER_BRONZE), 1);
    expect_eq("master unlock", slot_tier_unlock_level(SLOT_TIER_MASTER), 10);
    expect_eq("silver bet", slot_tier_bet(SLOT_TIER_SILVER), 50);
    expect_eq("master bet", slot_tier_bet(SLOT_TIER_MASTER), 1000);
    expect_true("lv1 bronze only", slot_highest_unlocked_tier(1) == SLOT_TIER_BRONZE);
    expect_true("lv5 gold", slot_highest_unlocked_tier(5) == SLOT_TIER_GOLD);
    expect_true("lv10 master", slot_highest_unlocked_tier(10) == SLOT_TIER_MASTER);
    expect_true("locked master", !slot_tier_unlocked(SLOT_TIER_MASTER, 9));
}

static void test_spin_basic(void)
{
    slot_player_t p;
    memset(&p, 0, sizeof(p));
    p.level = 1;
    p.tier = SLOT_TIER_BRONZE;

    slot_spin_result_t out;
    expect_true("spin ok", slot_spin(&p, 1, 2, 3, &out));
    expect_true("sym range0", out.symbols[0] >= 0 && out.symbols[0] < SLOT_SYM_COUNT);
    expect_true("sym range1", out.symbols[1] >= 0 && out.symbols[1] < SLOT_SYM_COUNT);
    expect_true("sym range2", out.symbols[2] >= 0 && out.symbols[2] < SLOT_SYM_COUNT);
    expect_true("mult nonneg", out.mult_x10 >= 0);

    p.level = 1;
    p.tier = SLOT_TIER_MASTER;
    expect_true("locked spin fail", !slot_spin(&p, 1, 2, 3, &out));
}

static void test_silver_win_guarantee(void)
{
    slot_player_t p;
    memset(&p, 0, sizeof(p));
    p.level = 3;
    p.tier = SLOT_TIER_SILVER;
    p.pity.no_win = 10; /* 触发 SILVER_WIN_GUARANTEE */

    slot_spin_result_t out;
    expect_true("g spin", slot_spin(&p, 0, 11, 22, &out));
    expect_true("pool win-g", out.pool_used == SLOT_POOL_WIN_GUARANTEE
                || out.pool_used == SLOT_POOL_BONUS);
    expect_true("at least 3x (or bonus>0)", out.mult_x10 >= 30);
    expect_eq("no_win reset", p.pity.no_win, 0);
}

static void test_master_jackpot_guarantee(void)
{
    slot_player_t p;
    memset(&p, 0, sizeof(p));
    p.level = 10;
    p.tier = SLOT_TIER_MASTER;
    p.pity.no_jackpot = 30;

    slot_spin_result_t out;
    expect_true("jp g spin", slot_spin(&p, 0, 5, 7, &out));
    expect_true("pool jp-g", out.pool_used == SLOT_POOL_JACKPOT_GUARANTEE);
    expect_true("high value", out.mult_x10 >= 500);
    expect_eq("jp counter clear", p.pity.no_jackpot, 0);
}

static void test_level_curve(void)
{
    expect_eq("lv1", slot_level_from_lifetime_won(0), 1);
    expect_eq("lv3 at 500", slot_level_from_lifetime_won(500), 3);
    expect_eq("lv5 at 2000", slot_level_from_lifetime_won(2000), 5);
    expect_eq("lv10 at 35000", slot_level_from_lifetime_won(35000), 10);
    expect_eq("need lv3", slot_lifetime_needed_for_level(3), 500);
    expect_eq("need lv8", slot_lifetime_needed_for_level(8), 12000);
    expect_true("gaps grow",
                slot_lifetime_needed_for_level(5) - slot_lifetime_needed_for_level(4)
                > slot_lifetime_needed_for_level(3) - slot_lifetime_needed_for_level(2));
    expect_true("post10 grows",
                slot_lifetime_needed_for_level(12) - slot_lifetime_needed_for_level(11)
                > slot_lifetime_needed_for_level(11) - slot_lifetime_needed_for_level(10));
}

static void test_bonus_deferred(void)
{
    /* 强制走到 BONUS：用 MASTER 普通池，扫描随机直到 needs_bonus */
    slot_player_t p;
    memset(&p, 0, sizeof(p));
    p.level = 10;
    p.tier = SLOT_TIER_MASTER;

    int found = 0;
    for (uint32_t i = 0; i < 5000 && !found; i++) {
        slot_spin_result_t out;
        if (!slot_spin(&p, i, i * 3u, i * 7u, &out)) continue;
        if (out.needs_bonus) {
            expect_eq("bonus gate mult", out.mult_x10, 0);
            expect_true("bonus sym", out.symbols[0] == SLOT_SYM_BONUS);
            int pity_before = p.pity.no_special;
            slot_spin_result_t bout;
            expect_true("bonus resolve", slot_bonus_spin(&p, i + 9u, i + 11u, &bout));
            expect_true("from bonus", bout.from_bonus);
            expect_true("bonus paid", bout.mult_x10 > 0);
            expect_true("special cleared", p.pity.no_special == 0 || p.pity.no_special <= pity_before);
            found = 1;
        }
    }
    expect_true("found bonus gate", found);
}

static void test_miss_increments(void)
{
    slot_player_t p;
    memset(&p, 0, sizeof(p));
    p.level = 1;
    p.tier = SLOT_TIER_BRONZE;

    /* 权重最大是未中奖；用足够多样本找一次 miss */
    int found_miss = 0;
    for (uint32_t i = 0; i < 400; i++) {
        slot_spin_result_t out;
        int before = p.pity.no_win;
        slot_spin(&p, i * 17u + 3u, i, i * 9u, &out);
        if (out.needs_bonus) continue;
        if (out.mult_x10 == 0 && !out.from_bonus) {
            expect_eq("miss bump", p.pity.no_win, before + 1);
            found_miss = 1;
            break;
        }
    }
    expect_true("found a miss", found_miss);
}

int main(void)
{
    test_payout_table();
    test_tier_meta();
    test_level_curve();
    test_spin_basic();
    test_silver_win_guarantee();
    test_master_jackpot_guarantee();
    test_bonus_deferred();
    test_miss_increments();

    if (fails) {
        fprintf(stderr, "%d failures\n", fails);
        return 1;
    }
    puts("slot_logic: PASS");
    return 0;
}
