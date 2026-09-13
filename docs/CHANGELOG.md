<p align="right">
  <a href="CHANGELOG.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Changelog

## Unreleased

- Slots: steeper level curve (rising gaps to Lv10 / beyond); keep status bar to two lines; CONFIG as equal-width 2×2 columns; amber level-up banner; distinct celebrate SFX; centered reels; Bet cycle + banner-only flash; add player rules in `docs/lucky-triple-slot.md`.

- Lucky Triple Slot: weighted pools, tier bets, pity, lifetime-won leveling; new Wild/Bonus/Mega pixel icons; Lv on status; banners for WIN/JACKPOT/MEGA/BONUS/level-up; Bonus mini page; FREE removed.

- Lucky Triple Slot core logic: weighted result pools, five tiers, pity counters, and Bonus pool (`slot_logic`); in-game UI wiring pending confirmation.

- Reel frame, CONFIG bar, and reel cells use flat panels without drop shadow.

- Status bar left/right layout (Credits/Bet | Best); keep rightmost reel inside panel; win banner gets a flashing plate.

- In-game UI: drop side color columns and EVA/ORANGE ribbon; tighten theme title and center/widen reels.

- Compact in-game CONFIG to a two-line bar; replace reel text (ORNG/EVA/777) with cute pixel icons (cherry/lemon/bell/star/gem).

- Persist splash hero index in NVS so power-off/power-on restores the last splash image.

- Full-song theme BGM: add a `music` partition between cardid and Recovery, flash an ADPCM bank, and loop whole tracks; short embedded PCM clips removed. Keep JIE/MARI splash as direct RGB565 (no posterize/pixel filter). Each theme has 3 splash frames (`jie-5` / `mar-3` / `p3-2` as the new third). Splash hides OK hints; in-game CONFIG lists Bet / Vol / BGM separately with vertical scroll. Splash swaps: JIE uses `jie-7`/`jie-8`, P3 primary uses `p3-4`. In-game slots UI restyled as flashy anime pixel chrome (neon frame, ribbon tag, theme color tiles instead of portraits, themed symbol names).

- Fix splash art (little-endian RGB565) with 2 switchable images per theme; splash UP/DOWN flips art then theme. Lengthen BGM to ~30 s loops at 8 kHz PCM.

- Fix theme splash snow/noise (RGB565 re-encoded for ST7789 `swap_bytes`); splash UP/DOWN cycles theme, UP/DOWN double adjusts volume, OK double toggles BGM; in-game OK spins, OK double cycles bet, UP/DOWN volume, UP double returns home.

- Slots themes are now JIE / P3 / MARI from `resource` art: splash UP/DOWN adjusts volume, OK double cycles theme, UP long loops that theme’s PCM BGM; in-game UI recolored per theme; old AI P5/P3 themes removed.

- Slots supports P5/P3 themes: UP/DOWN switches theme on splash, UP double returns to splash in-game; P3 uses a Reload-styled blue/cyan hero and matching UI.

- Slots now uses a full-screen P5-inspired hero illustration as the background; config hint remains `CONFIG (OKx2)`.

- Slots UI restyled in a P5R-like red/black/white slash look; config hint is now `CONFIG (OKx2)` on the title line.

- Slots bottom area is now a config panel for bet and volume (volume persisted in NVS); mascot removed from that slot.

- Slots upgrade: spin/stop/win SFX, win-flash banner animation, NVS best-win record, and triple-ST free spins.

- Added a slots game that boots straight into a three-reel machine (OK to spin, UP/DOWN to change bet, OK long-press to reset credits), keeping the ui_pixel theme and top-right battery SOC.

- Added the supplied 80-byte CW2017 profile for the specified 520 mAh cell, including content/update-flag checks, verified writes, the required restart sequence, and bounded SOC-readiness polling.

- Reorganized the documentation by function area with a dual entry point: the root `AGENTS.md` is now a thin router (hard constraints + task routing only) and the detailed AI workflow lives in `docs/development/ai-guide.md`; `agent-guide.md` was folded in. `docs/development/` gained a second level (`engineering/`, `ci/`, `release/`), and the `plays/` application archive and `experiences/` moved into a `docs/reference/` area with a dedicated README. Removed `docs/software-design/` (empty scaffold); folded the three `assets/{fonts,images,music}/README` leaves into the `assets/` README; flattened the six `project-completion` sub-documents into a single file; and unified each directory to a single README, eliminating every `INDEX` file and a duplicated experience index. All cross-references and bibliographic links were updated; no content was dropped.

- Made mini-program BLE install compatibility a template-level invariant: fixed
  protected `cardid`/Recovery partitions, retained the five-second UP-key
  Recovery boot hook, and added CI validation for merged-image structure,
  partition MD5/ranges, the 3 MB app limit, and protected payload exclusion.
- Documented a release-title convention for multi-app releases: name tags as `v<version>-<app-name>` (e.g. `v0.1.0-voice-keychain`) so the release title carries the version and the app, and confirm the title after the release is published so a release list is scannable by app.
- Added a post-release follow-up workflow: an `issue-suggestions` skill for filing user feedback as issues against the upstream project, an `experience-pr` skill for submitting reusable development experience as a documentation PR, a `docs/experiences/` directory for per-entry experience files, and supporting `project-completion`, `file-issues`, and experience-index documents.
- Simplified the tracked repository root: moved GitHub-recognized community documents into `.github/`, moved the changelog into `docs/`, updated every reference, and added a root-document allowlist to repository checks.
- Repository-wide language policy: every maintained Markdown default `.md` file is English, Simplified Chinese uses a paired `.zh_CN.md`, and both provide language switches. Static checks reject missing peers, missing switches, and Chinese prose in English defaults.
- Phase one of the AI development workflow: streamlined task-based context routing, unified local/CI validation, added PR checks and a template, and committed the dependency lock for reproducible builds.
- PR review fixes: pinned GitHub Actions to full commit SHAs, split build/release jobs by least privilege, disabled persisted sync checkout credentials, added Feature Request and Usage Question forms, clarified private security-report fallback, and corrected stale README, CI-trigger, and branch descriptions.
- Changed commit titles, PR titles, and PR bodies from Chinese-default to English; updated the Chinese punctuation rule so it no longer applies to PR descriptions.
- Reworked `build-firmware.yml` to pass `SDKCONFIG_DEFAULTS=sdkconfig.defaults`, enable `partitions.csv`, preserve the 8 MB image header, merge a flashable `FoloToy-AI-Passport-full.bin`, publish only that artifact, and use Actions cache v5.
- Integrated upstream PR #6 to resolve PR #4 conflicts: Wi-Fi, Bluetooth LE, radio lifecycle, and low-power demos; a 3 MB factory partition; build/menu/configuration updates; hardware-guide coverage; and bilingual capability tables.
- Defined English imperative Conventional Commit formatting for both commits and PR titles.
- Removed stale sync-workflow template comments and generalized an irrelevant Redis TTL rule to cache components.
- Added Chinese punctuation, credential safety, and recoverable file-deletion conventions.
- Expanded source-comment requirements for functions, state, ownership, concurrency, timing, registers, and magic values.
- Removed AI execution instructions from product READMEs so they remain human-facing product and repository overviews.
- Added `docs/development/agent-guide.md` as the focused AI workflow guide.
- Updated `AGENTS.md`, `docs/INDEX.md`, and the development index for the agent guide.
- Documented why the root README path is reserved for fork owners and how GitHub README precedence supports it.
- Created `main-update` from the upstream-aligned baseline and combined the repository-structure, firmware-CI, and upstream-sync work.
- Corrected the merged documentation index, workflow path, project tree, and CI references.
- Moved CI documentation from software design to `docs/development/`.
- Moved fork-only documentation assets from `assets/docs/` to `docs/assets/`.
- Moved the upstream English/Chinese project READMEs under `docs/` and renamed the documentation catalog to `docs/INDEX.md`.
- Initialized `AGENTS.md`, `CLAUDE.md`, and `CHANGELOG.md`.
- Standardized the initial project README language filenames.
- Added the `docs/`, `assets/`, and `skills/` directory structure.
- Moved the upstream hardware guide into `docs/hardware-design/`.
- Standardized subdirectory README capitalization and introduced fork conventions.
- Allowed fork-owned root README and supplemental documentation content on fork `main`.
- Added and documented the fork-only supplemental-document directory.
- Moved the build CI document to its dedicated CI branch before consolidation.
- Documented clean-`main` reasons, the direct-development exception, and Actions enablement for forks.
- Split the original agent rules into contribution, development, and fork documents with a compact root index.
- Updated software-design and project README references for the new documentation structure.
- Added the documentation catalog and task-triggered routing based on the earlier repository model.
- Added bilingual contribution, code-of-conduct, security, and support documents tailored to this ESP-IDF and fork workflow.
