<p align="right">
  <a href="lucky-triple-slot.md">English</a> · <strong>简体中文</strong>
</p>

# Lucky Triple Slot — 玩家规则

FoloToy AI Passport 上的三轴老虎机 Demo。可以把本页发给朋友，照着设备上手玩。

这是娱乐固件，只有虚拟 Credits，不是真钱赌博。

## 目标

花 Credits 开转，凑符号拿倍率；用累计赢分升级，解锁更高下注；冲击 Jackpot / Mega / Bonus 横幅。

每次进入 Demo 会从 **500 Credits** 开局。等级、累计赢分、当前下注档、保底计数、Best 单次赢分会写入 NVS，关机后仍保留。Credits 本身每次重新进 Demo 会重置。

## 按键

设备有三个键：**上 (UP)**、**下 (DOWN)**、**确认 (OK)**。

### 主页（立绘）

| 操作 | 效果 |
| --- | --- |
| 上 / 下 单击 | 翻同一主题立绘；翻完后切主题（JIE / P3 / MARI） |
| 上 / 下 双击 | 音量 + / − |
| OK 单击 | 进入局内界面 |
| OK 双击 | 开关 BGM |
| OK 长按 | 重置 Credits **以及** 等级进度（累计赢分、保底、下注档、Best） |

### 局内

| 操作 | 效果 |
| --- | --- |
| OK 单击 | 开转（扣除 Bet） |
| OK 双击 | 切换 CONFIG 焦点：Bet → Vol → BGM |
| 上 / 下 单击 | 调节当前焦点项 |
| 上 双击 | 回主页 |

转轴旋转中，或中奖 / Bonus / 升级横幅播放时，按键无效。

## 界面说明

- **状态条（上方）：** 左 = Credits + Bet（或中奖提示）；右 = Lv + 升到下一级还差的累计赢分（`Need`）
- **滚轴：** 三列符号
- **CONFIG（底部，等宽两列）：**
  - 左：Bet、BGM
  - 右：Vol
  - `>` 表示当前选中项

## 下注档与解锁

更高 Bet 扣得更多，结果池也更「肥」。上下键能扫过全部 Bet，但**未解锁**档位按 OK 会拒绝开转（提示 `Need LvN / Bet locked`）。

| Bet | 解锁等级 | 升到该级约需累计赢分 |
| --- | ---: | ---: |
| 10 | Lv 1 | 0 |
| 50 | Lv 3 | 500 |
| 100 | Lv 5 | 2000 |
| 500 | Lv 8 | 12000 |
| 1000 | Lv 10 | 35000 |

等级看的是**历史累计赢分**（所有派彩之和），不是当前剩余 Credits。

每升一级所需增量越来越大（到 Lv 10 大约从 +200 拉到 +15000）。Lv 10 之后：下一级约再要 **+20000**，再下一级 **+25000**、**+30000**……（每次多 +5000）。

## 一局怎么结算

1. 从 Credits 扣除 Bet。
2. 按当前档次从**加权结果池**抽一条结果（不是三轴各自独立随机）。
3. 滚轴动画后停在对应符号上。
4. 若中奖：Credits += `Bet × 倍率`（整数计算）。
5. 特殊情况：
   - **Bonus 入口**（三枚 Bonus）：先播 Bonus 小页，再从 Bonus 高价值池抽奖。
   - **升级：** 累计赢分跨过门槛时，琥珀色 `LV N!` 横幅。

### 常见倍率（× Bet）

| 结果 | 倍率 | 说明 |
| --- | ---: | --- |
| 任意一对（两枚相同） | 1.5× | 小奖 |
| 樱桃 ×3 | 3× | |
| 柠檬 ×3 | 5× | |
| 铃铛 ×3 | 10× | |
| 星星 ×3 | 20× | 更高档更常见 |
| 樱桃 · 星星 · 钻石 | 8× | 特殊线 |
| 星星 · 钻石 · 星星 | 15× | 特殊线 |
| 钻石 ×3 | 50× | JACKPOT 横幅 |
| Wild ×3 | 100× | JACKPOT 横幅 |
| Mega ×3 | 200× 或 500× | MEGA 横幅（池内抽取） |
| Bonus ×3 | — | 进入 Bonus 轮（派彩在 Bonus 池结算后） |

档次越高，大奖权重越高。精确权重见 `main/slot_logic.c`。

## 保底（软保底）

每个下注档各自统计「干瞪眼」次数。连续空转够久，下一把会进保底池（优先级：Jackpot → Special → High → Win → Normal）。计数会进 NVS。

举例（非完整列表）：

- 银档：连续 10 次不中奖 → 必中小奖池
- 金 / 钻 / 大师：按档次有高奖、特殊奖保底
- 大师：连续 30 次没有 Jackpot/Mega → Jackpot 保底

## 主题与声音

- 三个主题：**JIE**、**P3**、**MARI**（立绘与配色不同）
- 横幅颜色与音效区分：WIN / JACKPOT / MEGA / BONUS / 升级
- 局内 CONFIG 可调 Vol、BGM；主页双击上下也可调音量

## 给朋友的提示

1. 先用 Bet **10** 熟悉横幅和音效。
2. 等级不够别硬选高 Bet，开转会被拦住。
3. Credits 每次进 Demo 回到 500——长期玩的是**等级 / Best**，不是永久钱包。
4. Credits 不够当前 Bet 时，先把 Bet 调低。
5. 改 Bet / Vol / BGM 前，先 OK 双击把 `>` 移到对应项。

可打印的玩家手册 PDF：[`lucky-triple-slot-player-guide.zh_CN.pdf`](lucky-triple-slot-player-guide.zh_CN.pdf)（源稿 [`lucky-triple-slot-player-guide.zh_CN.html`](lucky-triple-slot-player-guide.zh_CN.html)）。

想换自己的立绘和主题曲：见 [`lucky-triple-slot-theme-customize.zh_CN.md`](lucky-triple-slot-theme-customize.zh_CN.md)（含可复制提示词）。

## 代码对照

| 内容 | 文件 |
| --- | --- |
| 结果池、保底、升级计算 | `main/slot_logic.c`、`main/slot_logic.h` |
| 界面、按键、音效、NVS | `main/demo_slots.c` |
| 主机侧逻辑测试 | `tests/test_slot_logic.c` |
