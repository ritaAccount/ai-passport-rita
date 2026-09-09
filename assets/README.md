<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

## Images

Store reusable source images and generated display assets in `images/`.

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

| File | Size | Notes |
| --- | --- | --- |
| `images/slots-jie-hero-240x320.png` | 240×320 | Angelina (JIE) splash preview from `resource/jie/jie-7.png`. |
| `images/slots-p3-hero-240x320.png` | 240×320 | P3R splash preview from `resource/p3/p3-4.png`. |
| `images/slots-mari-hero-240x320.png` | 240×320 | Mari splash preview from `resource/mari/`. |
| `../main/slots_*_hero.rgb565` / `*_hero2.rgb565` / `*_hero3.rgb565` | 240×320 RGB565 | Embedded theme splash frames (3 per theme). |

Conversion: resize to 240×320 → optional posterize (no dither) for soft/painted art → little-endian RGB565. Soft gradients and dither look like “snow” on ST7789; flat cel art (like P3) survives best. Full-resolution sources stay under `resource/`.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.
- Device playback uses PCM/ADPCM only; do not commit MP3 into `assets/` (keep originals in `resource/` if needed).

| File | Format | Notes |
| --- | --- | --- |
| `music/slots_music_bank.bin` | SLBM + 3× IMA-ADPCM WAV @ 8 kHz mono | Full songs: jie / p3 / mari. Flashed to `music` partition at `0x35A000` (see `partitions.csv`). |

Flash music separately (or after merge-bin):

```bash
esptool.py write_flash 0x35A000 assets/music/slots_music_bank.bin
```

App embeds only short SFX; BGM is mmap’d from the `music` partition and loops per theme.