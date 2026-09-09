#include "slot_logic.h"

#include <stdio.h>

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

int main(void)
{
    expect_eq("triple seven", slot_payout_mult(SLOT_SYM_SEVEN, SLOT_SYM_SEVEN, SLOT_SYM_SEVEN), 20);
    expect_eq("triple cherry", slot_payout_mult(SLOT_SYM_CHERRY, SLOT_SYM_CHERRY, SLOT_SYM_CHERRY), 8);
    expect_eq("pair ab", slot_payout_mult(SLOT_SYM_BELL, SLOT_SYM_BELL, SLOT_SYM_STAR), 2);
    expect_eq("none", slot_payout_mult(SLOT_SYM_CHERRY, SLOT_SYM_LEMON, SLOT_SYM_BELL), 0);
    expect_true("free", slot_is_free_trigger(SLOT_SYM_STAR, SLOT_SYM_STAR, SLOT_SYM_STAR));
    expect_true("not free", !slot_is_free_trigger(SLOT_SYM_STAR, SLOT_SYM_STAR, SLOT_SYM_SEVEN));
    expect_true("jackpot", slot_is_jackpot(SLOT_SYM_SEVEN, SLOT_SYM_SEVEN, SLOT_SYM_SEVEN));
    expect_eq("rand map", slot_symbol_from_rand(7), 7 % SLOT_SYM_COUNT);

    if (fails) {
        fprintf(stderr, "%d failures\n", fails);
        return 1;
    }
    puts("slot_logic: PASS");
    return 0;
}
