<p align="right">
  <a href="lucky-triple-slot.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Lucky Triple Slot — Player Guide

A three-reel slot demo on FoloToy AI Passport. Share this page with friends who will play on the device.

This is entertainment firmware with virtual credits only. It is not real gambling.

## Goal

Spend credits to spin. Match symbols for multipliers, raise your level from lifetime winnings, unlock higher bets, and chase Jackpot / Mega / Bonus banners.

You start each session with **500 credits**. Level, lifetime winnings, current bet tier, pity counters, and Best win are saved in NVS across power cycles. Session credits reset when you re-enter the demo.

## Hardware buttons

The device has three buttons: **UP**, **DOWN**, and **OK**.

### Splash (character art)

| Input | Action |
| --- | --- |
| UP / DOWN click | Flip hero art; after the last art, change theme (JIE / P3 / MARI) |
| UP / DOWN double | Volume up / down |
| OK click | Enter the game UI |
| OK double | Toggle BGM on the splash |

### In game

| Input | Action |
| --- | --- |
| OK click | Spin (deducts Bet) |
| OK double | Cycle CONFIG focus: Bet → Vol → BGM |
| UP / DOWN click | Adjust the focused CONFIG item |
| UP double | Return to splash |

While reels are spinning, or during win / Bonus / level-up banners, buttons are ignored.

## On-screen layout

- **Status (top):** left = Credits + Bet (or win line); right = Lv + Best
- **Reels:** three symbol columns
- **CONFIG (bottom, two equal columns):**
  - Left: Bet, BGM
  - Right: Vol
  - `>` marks the focused item

## Bet tiers and unlocks

Higher bets cost more and use richer result pools. You can scroll every Bet value, but a **locked** tier refuses to spin (`Need LvN / Bet locked`).

| Bet | Unlock level | Lifetime won needed for that level (approx.) |
| --- | ---: | ---: |
| 10 | Lv 1 | 0 |
| 50 | Lv 3 | 500 |
| 100 | Lv 5 | 2000 |
| 500 | Lv 8 | 12000 |
| 1000 | Lv 10 | 35000 |

Level comes from **lifetime credits won** (sum of all payouts), not from current credits.

The gap between levels grows each step (about +200 → +15000 to reach Lv 10). After Lv 10, each further level needs **+20000**, then **+25000**, **+30000**, … (+5000 more each time).

## How a spin works

1. Deduct Bet from Credits.
2. The game draws one **weighted outcome** for your tier (not three independent reel RNGs).
3. Reels animate, then stop on the outcome symbols.
4. If you win: Credits += `Bet × multiplier` (integer math).
5. Special cases:
   - **Bonus gate** (three Bonus symbols): a short Bonus screen, then a high-value Bonus pool draw.
   - **Level up:** amber `LV N!` banner if lifetime won crossed a threshold.

### Common payouts (multiplier × Bet)

| Result | Multiplier | Notes |
| --- | ---: | --- |
| Any pair (two alike) | 1.5× | Small win |
| Cherry ×3 | 3× | |
| Lemon ×3 | 5× | |
| Bell ×3 | 10× | |
| Star ×3 | 20× | Higher tiers |
| Cherry · Star · Diamond | 8× | Special line |
| Star · Diamond · Star | 15× | Special line |
| Diamond ×3 | 50× | Jackpot banner |
| Wild ×3 | 100× | Jackpot banner |
| Mega ×3 | 200× or 500× | Mega banner (pool pick) |
| Bonus ×3 | — | Enters Bonus round (payout after Bonus pool) |

Higher tiers weight big wins more often. Exact odds live in `main/slot_logic.c`.

## Pity (soft guarantee)

Each bet tier tracks dry streaks. After enough misses, the next spin uses a guarantee pool (priority: Jackpot → Special → High → Win → Normal). Counters persist in NVS.

Examples (not exhaustive):

- Silver: win guarantee after 10 dry spins
- Gold / Diamond / Master: high-win and special guarantees at tier-specific thresholds
- Master: Jackpot/Mega guarantee after 30 spins without Jackpot/Mega

## Themes and audio

- Three themes with different art and UI colors: **JIE**, **P3**, **MARI**
- Win banners use different colors and SFX for WIN / JACKPOT / MEGA / BONUS / level-up
- Adjust Vol and BGM in CONFIG; splash double-UP/DOWN also changes volume

## Tips for friends

1. Start on Bet **10** until you learn the banners and sounds.
2. Raise Bet only after the unlock level; locked bets will not spin.
3. Credits reset to 500 each demo entry — grind **level / Best**, not a permanent wallet.
4. If Credits are too low for your Bet, lower Bet or wait for a win streak.
5. Double-OK to move the `>` focus before changing Bet / Vol / BGM.

## Source map

| Topic | File |
| --- | --- |
| Pools, pity, level math | `main/slot_logic.c`, `main/slot_logic.h` |
| UI, buttons, audio, NVS | `main/demo_slots.c` |
| Host tests | `tests/test_slot_logic.c` |
