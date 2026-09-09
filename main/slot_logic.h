#pragma once

#include <stdint.h>
#include <stdbool.h>

enum {
    SLOT_SYM_CHERRY = 0,
    SLOT_SYM_LEMON,
    SLOT_SYM_BELL,
    SLOT_SYM_STAR,
    SLOT_SYM_SEVEN,
    SLOT_SYM_COUNT
};

// 三轴结果判奖：返回赢得的倍数（相对投注），0 表示未中。
// 三同 SEVEN → 20；三同其他 → 8；任意两同 → 2；否则 0。
int slot_payout_mult(int a, int b, int c);

// 三同 STAR 触发免费旋转。
bool slot_is_free_trigger(int a, int b, int c);

// 三同 SEVEN 为大奖展示。
bool slot_is_jackpot(int a, int b, int c);

int slot_symbol_from_rand(uint32_t r);
