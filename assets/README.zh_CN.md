<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

| 文件 | 尺寸 | 说明 |
| --- | --- | --- |
| `images/slots-jie-hero-240x320.png` | 240×320 | 安洁莉娜（JIE）主页预览，来自 `resource/jie/jie-7.png`。 |
| `images/slots-p3-hero-240x320.png` | 240×320 | P3R 主页预览，来自 `resource/p3/p3-4.png`。 |
| `images/slots-mari-hero-240x320.png` | 240×320 | 真希波（MARI）主页预览（为 RGB565 做过色块化）。 |
| `../main/slots_*_hero.rgb565` / `*_hero2.rgb565` / `*_hero3.rgb565` | 240×320 RGB565 | 嵌入固件的主题主页图（每主题 3 张）。 |

转换：缩放到 240×320 → 柔和渐变图做无抖动色块化 → 小端 RGB565。柔和渐变/抖动在 ST7789 上容易像「雪花」；扁平赛璐璐风（如 P3）最稳。高清源图在 `resource/`。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。
- 设备只播 PCM/ADPCM；不要把 MP3 提交进 `assets/`（需要时可留在 `resource/`）。

| 文件 | 格式 | 说明 |
| --- | --- | --- |
| `music/slots_music_bank.bin` | SLBM + 3 首 IMA-ADPCM WAV @ 8 kHz 单声道 | jie / p3 / mari 整首歌。刷到 `music` 分区 `0x35A000`（见 `partitions.csv`）。 |

单独刷音乐（或 merge-bin 之后）：

```bash
esptool.py write_flash 0x35A000 assets/music/slots_music_bank.bin
```

应用内只嵌短音效；BGM 从 `music` 分区 mmap，按主题整首循环。
