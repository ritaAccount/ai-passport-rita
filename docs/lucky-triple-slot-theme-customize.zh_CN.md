<p align="right">
  <a href="lucky-triple-slot-theme-customize.md">English</a> · <strong>简体中文</strong>
</p>

# Lucky Triple Slot · 给朋友的主题定制说明

把下面两段一起发给朋友即可：

1. **素材放哪里**（她自己准备图和音乐）
2. **复制给 Cursor / AI 的提示词**（她不用会写，填空后粘贴）

本仓库当前有三个主题槽位：**JIE / P3 / MARI**。定制时建议先**替换其中一个**（最简单），不要一次加第四个主题。

---

## 一、素材放哪里

在仓库根目录操作。先选定要替换的主题，例如换成自己的角色，就改 **JIE**（或 P3 / MARI）。

### 1. 立绘（主页大图）

| 放这里 | 说明 |
| --- | --- |
| `resource/<主题>/` | **原图**放这里，PNG 即可。`<主题>` 用 `jie` / `p3` / `mari` 之一。 |
| 例：`resource/jie/my-1.png` | 第 1 张立绘源图 |
| 例：`resource/jie/my-2.png` | 第 2 张（可 2～4 张） |

要求（尽量遵守，AI 会帮你裁切缩放）：

- 竖图更好，最终会做成 **240 × 320**
- 每主题建议 **2～4 张**（主页用上下键翻图）
- 不要放含隐私信息的图

> AI 会再生成：`assets/images/slots-*-240x320.png`（预览）和 `main/slots_*_hero*.rgb565`（真正嵌进固件）。**你只要管 `resource/` 里的原图。**

### 2. 主题曲（BGM）

| 放这里 | 说明 |
| --- | --- |
| `resource/<主题>/music/` | **源音频**放这里（mp3 / wav 都行） |
| 例：`resource/jie/music/my-theme.mp3` | 该主题循环播放的一首歌 |

注意：

- 每个主题槽位 **1 首歌**（和 JIE / P3 / MARI 一一对应）
- 设备播的是压缩后的曲库，不是直接播 mp3；**转换和刷写交给 AI**
- 请确认你有权使用该音乐（个人玩 / 已获授权）

> AI 会更新：`assets/music/slots_music_bank.bin`，并刷到设备的 `music` 分区（地址 `0x35A000`）。

### 3. 你不用手改的东西

下面这些让提示词里的 AI 去做即可：

- `main/demo_slots.c` 里的主题标题、配色
- `main/slots_*_hero.*`、`main/CMakeLists.txt`
- `assets/images/`、`assets/music/slots_music_bank.bin`
- 编译、刷机

---

## 二、填写后发给 AI 的提示词（复制模板）

让朋友把【】里的内容换成自己的，整段粘贴进 Cursor 对话：

```text
你是本仓库的 AI 助手。请先读 AGENTS.md 和 docs/development/ai-guide.md，再按下面要求，
把 Lucky Triple Slot（feature/slot-machine 相关代码，主入口 main/demo_slots.c）的一个主题
改成我的定制主题。不要改玩法逻辑（slot_logic），只改主题资源与配色。

【要替换的主题槽位】：JIE
（只能三选一：JIE / P3 / MARI。我选的是上面这个。）

【主题显示名】：例如 NANA（屏上标题，尽量短，4 字符以内更好）

【立绘原图路径】（已放进仓库）：
- resource/jie/my-1.png
- resource/jie/my-2.png
- resource/jie/my-3.png
（按实际文件名列全；2～4 张都可以）

【主题曲路径】（已放进仓库）：
- resource/jie/music/my-theme.mp3

【配色感觉】（用自然语言即可，没有就写「请从立绘主色自动取色」）：
- 主色 / 强调色：……
- 面板偏亮还是偏暗：……
- 中奖横幅风格：……

请你完成：
1. 把立绘做成 240×320、小端 RGB565，写入 main/ 对应 hero*.rgb565，并更新
   slots_*_hero.c/.h、CMakeLists.txt 的 EMBED_FILES，以及 demo_slots.c 的 heroes 数组；
   hero_count 必须等于实际张数。
2. 在 assets/images/ 留下同尺寸 PNG 预览，命名与现有 slots-*-hero*-240x320.png 风格一致。
3. 把主题曲转成设备可用的 BGM（与现有 slots_music_bank.bin 相同的 SLBM + IMA-ADPCM WAV
   方案，8 kHz 单声道优先），替换曲库里对应槽位那一首，更新 assets/music/slots_music_bank.bin。
4. 更新 THEMES[] 里该槽位的 title 与颜色字段，让 UI 和立绘气质一致。
5. 更新 docs/CHANGELOG.md 与 .zh_CN.md 各一句用户可见说明；assets/README 若有表项一并改。
6. 编译固件，merge-bin，刷机；音乐分区若需单独刷写，按 assets/README 的
   0x35A000 地址刷 slots_music_bank.bin。
7. 交付时报告：Build / Host tests / Device tests / Unverified。

约束：
- 保留 3 MB 应用上限，以及 cardid / Recovery 分区契约。
- 不要提交密钥、私人数据；音乐/图片请假设我已自备授权。
- 先 git status，不要覆盖我未相关的改动。
```

---

## 三、给朋友的超短版（可直接微信转发）

```text
嗨！这是老虎机主题定制步骤，很简单：

1) 打开仓库，选定要换的主题：JIE 或 P3 或 MARI（先只换一个）。

2) 把立绘 PNG 放到：
   resource/jie/     ← 若换 JIE（p3/mari 同理）
   建议 2～4 张竖图。

3) 把主题曲放到：
   resource/jie/music/xxx.mp3

4) 打开 Cursor，把「主题定制提示词」整段粘贴进去，
   只改【】里的主题名、文件路径、配色感觉。

5) 等 AI 编译刷机。主页上下键翻你的立绘，进局后就能听到你的歌。

详细说明见：docs/lucky-triple-slot-theme-customize.zh_CN.md
```
