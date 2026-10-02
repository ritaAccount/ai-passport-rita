<p align="right">
  <a href="lucky-triple-slot-theme-customize.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Lucky Triple Slot · Theme customize guide for friends

Send your friend both parts below:

1. **Where to put assets** (they prepare art and music)
2. **A fill-in Cursor / AI prompt** (they do not need to invent wording)

There are three theme slots: **JIE / P3 / MARI**. Replace **one slot first** — do not add a fourth theme on the first try.

---

## 1. Where to put assets

Work from the repository root. Pick the slot to replace (for example **JIE**).

### Art (splash heroes)

| Put files here | Notes |
| --- | --- |
| `resource/<theme>/` | Original PNGs. `<theme>` is `jie`, `p3`, or `mari`. |
| Example: `resource/jie/my-1.png` | Hero 1 source |
| Example: `resource/jie/my-2.png` | Hero 2 (2–4 images total) |

Guidelines:

- Portrait art works best; final size is **240 × 320**
- Prefer **2–4** images per theme (UP/DOWN flips them on the splash)
- No private data in images

The AI will generate `assets/images/slots-*-240x320.png` previews and `main/slots_*_hero*.rgb565` embeds. **Friends only need to drop originals under `resource/`.**

### Music (BGM)

| Put files here | Notes |
| --- | --- |
| `resource/<theme>/music/` | Source audio (mp3 / wav) |
| Example: `resource/jie/music/my-theme.mp3` | One looping track for that slot |

Notes:

- One song per theme slot (aligned with JIE / P3 / MARI)
- The device does not play mp3 directly; **conversion is the AI’s job**
- Confirm the friend may use the track

The AI updates `assets/music/slots_music_bank.bin` and flashes it to the `music` partition at `0x35A000`.

### Friends should not hand-edit

Leave these to the prompt:

- Theme title / colors in `main/demo_slots.c`
- `main/slots_*_hero.*`, `main/CMakeLists.txt`
- `assets/images/`, `assets/music/slots_music_bank.bin`
- Build and flash

---

## 2. Copy-paste AI prompt (fill the brackets)

```text
You are the AI assistant for this repository. Read AGENTS.md and
docs/development/ai-guide.md first. Customize one Lucky Triple Slot theme
(main UI in main/demo_slots.c). Do not change slot_logic gameplay — only
theme assets and colors.

[Theme slot to replace]: JIE
(One of: JIE / P3 / MARI.)

[On-screen title]: e.g. NANA (keep it short)

[Hero source paths] (already in the repo):
- resource/jie/my-1.png
- resource/jie/my-2.png
- resource/jie/my-3.png

[BGM source path]:
- resource/jie/music/my-theme.mp3

[Color direction] (or write “auto-pick from the art”):
- Accent / panel / banner mood: ...

Please:
1. Convert heroes to 240×320 little-endian RGB565 under main/, update
   slots_*_hero.c/.h, CMakeLists.txt EMBED_FILES, and demo_slots.c heroes;
   set hero_count to the real image count.
2. Write matching PNG previews under assets/images/.
3. Rebuild assets/music/slots_music_bank.bin (existing SLBM + IMA-ADPCM WAV
   scheme, prefer 8 kHz mono) replacing that theme’s track.
4. Update THEMES[] title and colors for the slot.
5. Update CHANGELOG (+ zh_CN) and assets README table rows if needed.
6. Build, merge-bin, flash; flash music at 0x35A000 if required.
7. Report Build / Host tests / Device tests / Unverified.

Constraints: keep the 3 MB app limit and cardid/Recovery contracts; no
secrets; assume the user owns or licensed the media; start from git status
and do not clobber unrelated local changes.
```

---

## 3. Short message template

```text
Hi! Theme customize is simple:

1) Pick one slot: JIE, P3, or MARI.
2) Put PNGs in resource/jie/ (or p3/mari).
3) Put one mp3 in resource/jie/music/.
4) Paste the theme prompt into Cursor; fill the bracket fields only.
5) Let the AI build and flash.

Details: docs/lucky-triple-slot-theme-customize.md
```
