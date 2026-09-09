#include "slot_logic.h"

int slot_payout_mult(int a, int b, int c)
{
    if (a < 0 || b < 0 || c < 0) return 0;
    if (a >= SLOT_SYM_COUNT || b >= SLOT_SYM_COUNT || c >= SLOT_SYM_COUNT) return 0;

    if (a == b && b == c) {
        return (a == SLOT_SYM_SEVEN) ? 20 : 8;
    }
    if (a == b || b == c || a == c) return 2;
    return 0;
}

bool slot_is_free_trigger(int a, int b, int c)
{
    return a == SLOT_SYM_STAR && b == SLOT_SYM_STAR && c == SLOT_SYM_STAR;
}

bool slot_is_jackpot(int a, int b, int c)
{
    return a == SLOT_SYM_SEVEN && b == SLOT_SYM_SEVEN && c == SLOT_SYM_SEVEN;
}

int slot_symbol_from_rand(uint32_t r)
{
    return (int)(r % (uint32_t)SLOT_SYM_COUNT);
}
