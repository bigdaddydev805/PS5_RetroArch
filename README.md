# PS5 RetroArch 🎮

**Native RetroArch for jailbroken PlayStation 5 consoles, rendering through
[PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan)'s port of Mesa's RADV
Vulkan driver.**

Maintained by [Mihawk](https://github.com/mihawk-99). Based on
[RetroArch / libretro](https://github.com/libretro/RetroArch), with a native PS5
application foundation derived from
[ProsperoLight](https://github.com/blackbearreloaded/ProsperoLight).

The current build launches as a homebrew title, presents XMB through RetroArch's
Vulkan video driver, and runs eight native libretro cores, three of them (PSP,
GameCube/Wii and PlayStation 2) rendering on the PS5's GPU. Input, stereo audio,
configuration persistence and content browsing have been verified on a console.
This is an active development project; the tested paths below do not imply
complete core compatibility or Vulkan conformance.

**Latest release: v0.5.0-alpha.5** — see the [release notes](docs/releases/v0.5.0-alpha.5.md).
It is the first release on RADV; [v0.4.0-alpha.4](docs/releases/v0.4.0-alpha.4.md)
was the last on ps5vk, the project's first driver.

## Table of contents

- [Current status](#current-status)
- [Available cores](#available-cores)
- [Graphics and native runtime](#graphics-and-native-runtime)
- [Roadmap](#roadmap)
- [Build from source](#build-from-source)
- [Install and file locations](#install-and-file-locations)
- [Testing and troubleshooting](#testing-and-troubleshooting)
- [Documentation](#documentation)
- [Authors and acknowledgements](#authors-and-acknowledgements)
- [License and third-party terms](#license-and-third-party-terms)

## Current status

| Feature | Status |
| --- | --- |
| Native title startup and Quit | ✅ Working, including splash dismissal and clean native exit |
| Vulkan video output | ✅ Menu and software-core frames presented through RADV, Mesa's Vulkan driver, linked into the title (PS5_Vulkan's port, reporting Vulkan 1.4). ps5vk remains a build option |
| XMB | ✅ Default menu, with icons, fonts and background rendering |
| RGUI | ✅ Alternative menu |
| Native controller input | ✅ Buttons, left-stick menu navigation and button/axis binding capture. 🚧 DualSense accelerometer and gyroscope through libretro's sensor interface: the raw sample fields, units (g, rad/s) and the X and Z axes were measured on a console, and the driver hands cores libretro.h's frame (one sign change, yaw). Dolphin's Wii Remote gets that frame rotated into its own inside the core. The resting sign of the accelerometer's Y axis and the pitch sign are the next console check; no game's motion controls are claimed to work yet |
| Native audio | ✅ `audio_ps5` stereo PCM output, audible channel test and buffering diagnostics |
| Filesystem and configuration | ✅ Directory browsing, configuration loading/saving and FTP-writable application folders |
| Core loading | ✅ Native shared-core loader, official `.info` discovery and recovery from rejected loads |
| Content loading | ✅ Tested games and archives with the cores below. Load Content opens two roots: **INTERNAL**, the title's folder, and **EXTERNAL**, the console's `/mnt`, where USB drives and extended storage mount. The title's sandbox hides `/mnt` for now, so EXTERNAL is empty |
| Colour and menu transitions | ✅ Corrected pixel uploads; Quick Menu/Close Content/next-game transitions I verified on the console |
| Hardware-rendered cores | ✅ PPSSPP, Dolphin, LRPS2, Beetle PSX HW, Mupen64Plus-Next (ParaLLEl-RDP) and Azahar render on RADV through their Vulkan renderers, with JITs where they have them, at up to 18× internal resolution; PPSSPP's MSAA works |
| Shader cache | ✅ RADV keeps compiled pipelines in `radv-shader-cache/`: a game's next start reads them back instead of compiling |
| Save states and fast-forward | ✅ Save/load states (including `--entryslot`) and fast-forward, tested with PPSSPP and mGBA |
| 120 Hz output | ✅ 120 Hz by default where the display offers it; the refresh is measured, and a display that stays at 60 Hz gets 60 Hz |
| Stability | ✅ On RADV: every core with a game through boot, the menu, Close Content and a reload, with and without Threaded Video, and a 10-minute PPSSPP soak with 25 menu toggles: no crash ([release checks](docs/PHASE_LOG.md)) |
| CPU video fallback | ✅ `video_ps5` remains registered and selectable |
| Development diagnostics | ✅ `retroarch.log`, startup/GPU trace, kernel captures and optional buffered frame timing |

The Alpha 5 checks recorded no crash and no kernel fatal signal, and full-speed
audio windows once each game had booted. These results apply to the captured
tests, not every possible workload. See
[active state](docs/ACTIVE.md) and [committed evidence](evidence/) for exact builds,
test coverage and known exceptions.

## Available cores

The title build includes these cores and their official metadata. FCEUmm,
mGBA, Snes9x, FBNeo, Genesis Plus GX, Beetle Saturn, VICE, MAME and DeSmuME
**render emulated games in software**; RetroArch uploads their frames and presents
them through Vulkan. PPSSPP, Dolphin, LRPS2, Beetle PSX HW, Mupen64Plus-Next and
Azahar **render on the GPU** through RADV, with their Vulkan renderers.

Each core's PS5 defaults are its highest graphical settings that hold full speed
in the games I tested: the most internal resolution the core offers, unless a
lower one is the most that keeps full speed (DeSmuME).

| Core | Systems covered by the core | Console verification in this port |
| --- | --- | --- |
| [FCEUmm](https://github.com/libretro/libretro-fceumm) | NES / Famicom | ✅ Gameplay, audio and input; subsequent shared menu-transition fixes verified. [Evidence](evidence/native-core-loading/) |
| [mGBA](https://github.com/libretro/mgba) | Game Boy, Game Boy Color, Game Boy Advance | ✅ GB/GBC/GBA loading, corrected colours and clean menu/next-game transitions. [Evidence](evidence/mgba-native/) |
| [Snes9x](https://github.com/libretro/snes9x) | SNES / Super Famicom | ✅ Tested gameplay, colours, audio/input and menu transitions; not every special chip or video mode. [Evidence](evidence/snes9x-native/) |
| [FinalBurn Neo](https://github.com/libretro/FBNeo) | Supported arcade boards, including Neo Geo and Sega System 16/32 | ✅ Tested arcade games using both native 32-bit and converted 16-bit output; not every board or ROM set. [Evidence](evidence/fbneo-native/) |
| [Genesis Plus GX](https://github.com/libretro/Genesis-Plus-GX) | Mega Drive / Genesis, Master System, Game Gear, SG-1000, Sega CD | ✅ Genesis gameplay and clean transitions, which I confirmed on the console. Other Sega systems and disc/BIOS paths still need separate acceptance. [Evidence](evidence/genesis-plus-gx-native/) |
| [PPSSPP](https://github.com/hrydgard/ppsspp) v1.20.4 | PlayStation Portable | ✅ God of War: Ghost of Sparta and Yu-Gi-Oh! GX Tag Force at 10× internal resolution (4800×2720), 16× anisotropy: correct picture, full speed at 120 Hz, save states, fast-forward, and closing and reopening games. MSAA renders on RADV (it needs render pass 2, which ps5vk lacked). |
| [Dolphin](https://github.com/libretro/dolphin) 2609 | GameCube, Wii | ✅ Wind Waker (an hour), Resident Evil 4 (30 minutes), Super Smash Bros. Melee, Mario Kart Wii and Rogue Leader, with the JIT and fast memory, save states and closing and reopening games. Rogue Leader's attract sequence still dips to 72–85% (see the release notes). |
| [LRPS2](https://github.com/libretro/LRPS2) (PCSX2) | PlayStation 2 | ✅ The God of War II and Final Fantasy X demos and GTA San Andreas at 6× internal resolution on the Vulkan hardware renderer, full speed, with multi-threaded VU1 and save states. Needs your own BIOS in `system/pcsx2/bios/`. |
| [Beetle PSX HW](https://github.com/libretro/beetle-psx-libretro) | PlayStation | ✅ Crash Bandicoot at 16× internal resolution on the Vulkan renderer, 32-bit colour, PGXP (no wobbling polygons), full speed, and closing and reopening the game. The disc image is read into memory at load. It runs with its built-in OpenBIOS; your own BIOS (`scph5501.bin` and the others its metadata lists) in `system/` is used when present. |
| [Mupen64Plus-Next](https://github.com/libretro/mupen64plus-libretro-nx) | Nintendo 64 | ✅ Mario Kart 64 with ParaLLEl-RDP at 8× upscaling and ParaLLEl-RSP, both JITs on, full speed after boot, and closing and reopening the game. |
| [Beetle Saturn](https://github.com/libretro/beetle-saturn-libretro) | Sega Saturn | ⚠️ Loads, then needs your own BIOS in `system/`: `mpr-17933.bin` (US/EU) or `sega_101.bin` (JP). Without it the game refuses to load and the menu stays usable. Gameplay not yet tested. |
| [VICE](https://github.com/libretro/vice-libretro) x64sc | Commodore 64 | ✅ A `.d64` disk game at full speed, and closing and reopening it. |
| [MAME](https://github.com/libretro/mame) 0.289 | Arcade | ✅ Metal Slug 3 from a 0.289 non-merged set, BIOS in the same folder: full speed, and closing and reopening it. Raster games render at their native size and are scaled on the GPU. Vector games are drawn at 4K by MAME's alternate renderer (not yet tested on the console). Sets must match 0.289. |
| [DeSmuME](https://github.com/libretro/desmume) | Nintendo DS | ✅ Pokémon Diamond at 5× (1280×960) with the JIT and eight rasterizer threads, full speed, and closing and reopening it. 6× measured 93–95%. |
| [Azahar](https://github.com/azahar-emu/azahar) | Nintendo 3DS | ✅ Mario & Luigi: Superstar Saga + Bowser's Minions at 18× internal resolution (the most Azahar offers) on Vulkan, with asynchronous shader compilation and the JIT, full speed after boot, and closing and reopening it. Decrypted games only. |
| [RPCS3](https://github.com/mihawk-99/PS5_RPCS3) (my fork) | PlayStation 3 | ⚠️ **A console build only, in no release** (its licence, see [License and third-party terms](#license-and-third-party-terms)): it builds with this repository. God of War HD at 4K and 60 fps in gameplay. GTA IV at 4K held at 30 fps: a game with only an "Unlock FPS" patch runs it with RPCS3's frame limit at 30 by default (the "Frame-rate patches" option), steady where the emulation keeps up; its busiest city drives still dip to 24–27 fps. Needs your own PS3 firmware (`PS3UPDAT.PUP` in `system/RPCS3/`) and games; a PSN package installs with its `.rap`. |

Use **FBNeo for Sega System 16/32 arcade sets**, rather than Genesis Plus GX.
FBNeo needs compatible arcade sets and receives its ZIP/7z archives intact.
Archive support and BIOS requirements vary by core.

Core binaries must be built for **this native pipeline and SDK**. A desktop `.so`
or a core from a different PS5 RetroArch distribution is not automatically
compatible. No games or BIOS files are bundled.

## Graphics and native runtime

```text
Software core → video callback → RetroArch Vulkan video driver
                                → RADV, linked into the title → PS5 display
XMB / RGUI ──────────────────────┘
PPSSPP, Dolphin, LRPS2 (hardware cores) → Vulkan through RetroArch's HW context → RADV
```

[PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan), which I also maintain,
is the separate GPU-driver project used here. Since v0.5.0-alpha.5 the title
links its port of RADV: Mesa's Vulkan driver and ACO compiler, unchanged but
where the console differs, over a PS5 winsys, built from my Mesa fork
[PS5_Mesa](https://github.com/mihawk-99/PS5_Mesa). It reports Vulkan 1.4, and
its conformance is that project's milestone: the full Khronos CTS runs on the
console. Up to v0.4.0-alpha.4 the title linked ps5vk, the project's first
driver, which `PS5_VULKAN_DRIVER=ps5vk` still builds.

The driver is **linked into the title**. Updating a driver checkout does not
update the linked code: rebuild and redeploy the RetroArch title against the
intended driver artifacts. A `libvulkan.so.1` left in the title folder by an
earlier release is ps5vk's and is not used.

This repository supplies the frontend/platform integration, native audio and
input backends, core loader, build scripts and console validation. Core-side
pixel adapters preserve the renderer's buffers while matching the frontend's
upload format. The CPU video backend remains available as a fallback; the
current default is `video_driver = "vulkan"`, `menu_driver = "xmb"`.

PPSSPP's JIT runs: its code memory is mapped read-write and then made
executable, and its fast-memory fault handler reads the console's own signal
context layout. The port builds PPSSPP v1.20.4 with one patch
(`patches/ppsspp/ps5-port.patch`) and FFmpeg 3.0.2 for game videos. It starts
with the settings I test with (10× internal resolution, 16× anisotropy, auto
max-quality filtering, hardware transform, software skinning, no frameskip, no
speed hacks); an existing `PPSSPP.opt` is set aside once as
`PPSSPP.opt.before-ps5-profile`. The native loader has explicit limits, including
no TLS or general exception-unwind registration, and it waits for a core's
threads to finish before unmapping the core. See the
[runtime contract](docs/REFERENCE.md#native-in-process-core-loader).

## Roadmap

**✅ = verified for the stated scope. ❌ = pending implementation or acceptance
in this port, even if upstream RetroArch already offers the feature.**

### Frontend and platform

- ✅ Native startup and clean exit.
- ✅ GPU presentation through PS5_Vulkan; selectable CPU fallback.
- ✅ XMB, with RGUI retained.
- ✅ Native input, analog menu navigation and remapping.
- 🚧 DualSense motion (accelerometer and gyroscope) through the libretro sensor API, in libretro.h's frame and units, from sample fields and axes measured on a console; Dolphin's core rotates that frame into the Wii Remote's. Awaiting the Wii Sports console test.
- ✅ Native stereo audio and buffering diagnostics.
- ✅ Filesystem browsing, configuration persistence and content loading.
- ✅ Native core loading, metadata and failed-load recovery.
- ❌ Save RAM and save-state persistence verified across restarts and core changes.
- ❌ Core-option persistence and per-game/per-core overrides fully validated.
- ❌ BIOS/system-file coverage, disc swapping and multi-disc acceptance tests.
- ❌ RetroAchievements and netplay; networking is disabled in the current frontend build.
- ❌ User Slang shader presets and multipass effects validated on PS5_Vulkan.
- ✅ 120 Hz output where the display offers it, with a 60 Hz fallback chosen by measuring the refresh.
- ❌ No core losing speed while a shader compiles. On RADV (Alpha 5), PPSSPP's compiles on its own threads no longer cost it speed, and a game's second start reads its pipelines from the cache; Dolphin's first start of a game still compiles its ubershaders at a cost (see the release notes).
- ✅ Save states and fast-forward, including PPSSPP.
- ❌ 4K output/upscaling, VRR and HDR validated in this application.
- ❌ Low-latency features, runahead and sustained per-core performance measurements.
- ❌ Broader compatibility testing and release qualification.

### Cores

| Status | Core / milestone |
| --- | --- |
| ✅ | FCEUmm — NES |
| ✅ | mGBA — GB / GBC / GBA |
| ✅ | Snes9x — SNES |
| ✅ | FBNeo — tested arcade games |
| ✅ | Genesis Plus GX — tested Genesis gameplay |
| ❌ | Beetle PCE — PC Engine / TurboGrafx-16, SuperGrafx and CD; next proposed addition |
| ❌ | Stella — Atari 2600; candidate |
| ❌ | PicoDrive — add Sega 32X coverage; candidate |
| ✅ | MAME 0.289 — arcade; tested games only |
| ✅ | Beetle PSX HW — PlayStation, Vulkan renderer at 16× |
| ✅ | Mupen64Plus-Next — Nintendo 64, ParaLLEl-RDP at 8× |
| 🚧 | Beetle Saturn — Sega Saturn; needs a gameplay test with a BIOS |
| ✅ | VICE x64sc — Commodore 64 |
| ✅ | DeSmuME — Nintendo DS at 5× |
| ✅ | Azahar — Nintendo 3DS, Vulkan at 18× |
| ❌ | EXTERNAL storage (USB, extended storage) readable from inside the title's sandbox |
| ✅ | PPSSPP — PSP, Vulkan rendering and JIT; tested games only |
| ✅ | PPSSPP MSAA — render pass 2 and depth/stencil resolve, on RADV |
| ✅ | Dolphin — GameCube and Wii, Vulkan rendering and JIT; tested games, long play and the enhancement profiles |
| ✅ | LRPS2 — PlayStation 2, Vulkan hardware renderer at 4K; tested games only |
| 🚧 | LRPS2 — upstream PCSX2's newer renderer fixes, 8× internal resolution and texture replacement |
| 🚧 | RPCS3 — PlayStation 3 at 4K; a console build only (not in releases); GTA IV's busiest drives below 30 |

Future entries are development targets, not a promised release order. Hardware
rendering introduces new Vulkan requirements beyond presenting software frames;
PPSSPP is the first core that exercises them.

## Build from source

The current build uses Linux host tools and the project's cached public PS5 SDK.
Start with `bash tools/doctor.sh` for host-tool checks. You also need `curl`,
`patch`, `pkg-config`, ELF utilities such as `readelf`, and working host C/C++
compilers with sanitizer support for the tests. The target scripts currently use
`/usr/bin/clang` through the SDK wrappers.

Prepare and build [PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan) following
its own instructions, normally as a sibling directory:

```text
workspace/
├── PS5_RetroArch/
├── PS5_Vulkan/       # The RADV release archive, its link recipe and dependencies
├── PS5_Mesa/         # My Mesa fork, which PS5_Vulkan builds RADV from
└── PS5_PayloadSDK/   # My payload SDK fork and its platform layer, at a pinned revision
```

The cores that needed changes for the console build from my forks of them
(PS5_LRPS2, PS5_BeetlePSX, PS5_Mupen64Plus, PS5_BeetleSaturn, PS5_VICE,
PS5_MAME, PS5_DeSmuME, PS5_Azahar with PS5_Dynarmic), each pinned by revision in
its build script. The script uses the sibling checkout when there is one, and
`github.com/mihawk-99/<fork>` otherwise.

The title build consumes PS5_Vulkan's RADV release archive
(`tools/build-radv.sh release` there, built from PS5_Mesa at the revision it
pins) and links it with that project's `tools/radv-link.sh`; it does not build
the driver for you. `PS5_VULKAN_DRIVER=ps5vk` links ps5vk's driver, Vulkan
runtime and shader-compiler archives instead, and `RADV_ARCHIVE` names another
RADV archive. `PS5_VULKAN_DIR` can select an alternative checkout for the
build; some host tests currently require the sibling layout above. Driver
dependencies and setup requirements are documented in the driver repository.

From the RetroArch repository root:

```bash
make deps
export PS5_PAYLOAD_SDK="$PWD/.deps/native/ps5-payload-sdk"
export PS5_CLANG=/usr/bin/clang
bash tools/fetch-retroarch.sh
bash tools/build-title.sh     # Create the artifacts inspected by the host tests
bash tools/verify.sh
```

The five gates are **format → unit → build → integration → evidence**. The build
pins RetroArch 1.22.2, fetches core sources/metadata with checked hashes, builds the
frontend and all sixteen cores, and stages the native title in `dist/PPSA99169/` (a
release build, `PS5_RELEASE_TAG` set, leaves RPCS3 out).
The initial dependency/source fetch requires network access.

For an already configured checkout:

```bash
bash tools/build-title.sh     # Build/stage the frontend and all shipped cores
make genesis-plus-gx         # Build and ABI-check one core only
# Other core targets: fceumm, mgba, snes9x, fbneo
bash tools/build-ppsspp.sh   # The larger cores have scripts of their own:
                             # build-ppsspp.sh, build-dolphin.sh, build-lrps2.sh,
                             # build-beetle-psx.sh, build-mupen64plus.sh,
                             # build-beetle-saturn.sh, build-vice.sh,
                             # build-mame.sh, build-desmume.sh, build-azahar.sh
```

When adding or updating a core, rebuild the title too: the frontend's native
import table and build identity depend on the shipped core binaries. Source
patches live in `patches/`; fetched and generated trees stay in ignored
`vendor/`, `.deps/`, `build/` and `dist/` directories.

## Install and file locations

The verified deployment is a **homebrew title folder**, not a retail package.
Copy the complete `dist/PPSA99169/` tree to the title location used by your
configured homebrew launcher. The current validation setup uses
`/data/homebrew/PPSA99169/`. This project does not install a jailbreak or launcher.

`/app0` is the running application's mount point. Over FTP, use the corresponding
title folder instead:

| Purpose | Under `/data/homebrew/PPSA99169/` | In RetroArch |
| --- | --- | --- |
| Native cores | `cores/` | `/app0/cores/` |
| Core metadata | `info/`, with compatibility copies in `cores/` | `/app0/info/` |
| Games | `content/` | `/app0/content/` |
| BIOS/system data | `system/` | `/app0/system/` |
| PS2 BIOS (your own dump) | `system/pcsx2/bios/` | `/app0/system/pcsx2/bios/` |
| Live configuration | `config/retroarch.cfg` | `/app0/config/retroarch.cfg` |
| Save RAM | `savefiles/` | `/app0/savefiles/` |
| Save states | `savestates/` | `/app0/savestates/` |
| RADV shader cache | `radv-shader-cache/` | `/app0/radv-shader-cache/` |

RADV creates `radv-shader-cache/` itself, open to FTP like the other folders;
deleting it only makes the next start compile again. `ps5vk-shader-cache/` and
`sce_module/libvulkan.so.1`, left by releases up to v0.4.0-alpha.4, are not used
since v0.5.0-alpha.5 and can be deleted.

For FBNeo, use `system/fbneo/` for its system files. Genesis Plus GX's Sega CD BIOS
filenames belong in the configured `system/` root, as listed by its metadata.

The application creates its managed writable folders and seeds live settings
only when no live configuration exists. Ordinary scripted updates preserve user
content and saved settings. Existing settings can therefore keep RGUI selected
even though XMB is the packaged default. Back up user files before any clean
removal; `--clean` removes the entire title folder.

See [deployment](docs/DEPLOYMENT.md) for FTP setup, verified readback and test runs.
Console details belong in the ignored `.env`, based on `.env.example`.

## Testing and troubleshooting

A successful build proves neither gameplay nor correct rendering. Core acceptance
includes native loading, gameplay, colour checks, audio/input, Quick Menu →
Close Content, and loading another game. I record my own visual confirmation on
the console alongside the logs; a camera can miss refresh-synchronous flicker.

The v0.5.0-alpha.5 release passed all five host gates with 74 Python tests;
52 recorded captures replay successfully. Exact results and limitations are in
[ACTIVE](docs/ACTIVE.md), rather than implied by a core's upstream feature list.

For reports, include the core, game-file format, relevant settings, reproduction
steps and whether the application was closed manually. Preserve `retroarch.log`
and `trace.txt` before reopening: the frontend log is replaced on a new launch.
The test tools retain kernel captures in ignored `klog/`. Redact private paths,
console addresses and credentials before sharing logs.

Old `gpu-buffers-*.bin`, `gpu-stages-*.bin` and `gpu-tables-*.bin` files are temporary
rendering diagnostics from earlier investigations. They are not required runtime
assets and can be removed. Keep the normal development logs when reporting bugs.

## Documentation

| Document | Purpose |
| --- | --- |
| [Active state](docs/ACTIVE.md) | Current verified build, known errors and acceptance limits |
| [Reference](docs/REFERENCE.md) | Native loader, core contracts, source pins and platform details |
| [Testing](docs/TESTING.md) | Host gates and console acceptance procedures |
| [Deployment](docs/DEPLOYMENT.md) | Build staging, FTP locations, updates and capture workflow |
| [Troubleshooting](docs/TROUBLESHOOTING.md) | Recorded symptoms, causes and fixes |
| [Findings](docs/FINDINGS.md) | Technical observations and evidence behind decisions |
| [Phase log](docs/PHASE_LOG.md) | Dated development and console-test history |
| [Releasing](docs/RELEASING.md) | What a release carries (licences, component manifest, source archives) and the order that keeps tag, ZIP and source in step; [what each past release was built from](docs/releases/SOURCE_CORRESPONDENCE.md) |
| [Evidence](evidence/) | Machine-readable captures and expected results |
| [Contributor/agent instructions](AGENTS.md) | Scope, verification, attribution and commit rules |

## Authors and acknowledgements

This port builds on substantial upstream and PS5 homebrew work. Credits below
identify project authors and teams; their repositories retain the full contributor
lists and original notices.

### Frontend, platform and graphics

| Project / author | Contribution |
| --- | --- |
| [Mihawk](https://github.com/mihawk-99) — [PS5_RetroArch](https://github.com/mihawk-99/PS5_RetroArch), [PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan), [PS5_Mesa](https://github.com/mihawk-99/PS5_Mesa), [PS5_PayloadSDK](https://github.com/mihawk-99/PS5_PayloadSDK) | This native RetroArch port and core integration; the PS5 Vulkan drivers (the RADV port and ps5vk); the Mesa fork with the PS5 winsys; the payload SDK fork and its platform layer |
| [RetroArch / libretro contributors](https://github.com/libretro/RetroArch) | Frontend, libretro API, menus, video pipeline and shared libraries |
| [BlackBearReloaded — ProsperoLight](https://github.com/blackbearreloaded/ProsperoLight) | Project starting point and reference for native PS5 input and audio integration |
| [BlackBearReloaded — PS5 Native App Boilerplate](https://github.com/blackbearreloaded/ps5-native-app-boilerplate) | Underlying native title tooling, ELF/FSELF conversion and runtime-shim foundation |
| [John Törnblom and ps5-payload-dev contributors](https://github.com/ps5-payload-dev/sdk) | Public PS5 Payload SDK, toolchain and API stubs; [PacBrew](https://github.com/ps5-payload-dev/pacbrew-repo) ports infrastructure |
| [John Törnblom / ps5-payload-dev — websrv](https://github.com/ps5-payload-dev/websrv) | Reference for per-core fetch/build/stage scripts; this port uses a separate native title pipeline |
| [BlackBearReloaded — ps5-opengl](https://github.com/blackbearreloaded/ps5-opengl) | Shader-compiler and graphics foundations consumed by PS5_Vulkan; not the active RetroArch video backend |
| [Mesa contributors](https://gitlab.freedesktop.org/mesa/mesa) | RADV, the Vulkan driver the title renders through, with its ACO compiler, NIR, the Vulkan runtime and utilities |
| [Khronos Group](https://github.com/KhronosGroup/Vulkan-Headers) | Vulkan API headers and [specification](https://github.com/KhronosGroup/Vulkan-Docs) |
| [RetroArch assets contributors](https://github.com/libretro/retroarch-assets) and the [M+ Fonts project](https://mplusfonts.github.io/) | Packaged XMB assets and font; original notices retained |
| [LLVM / Clang contributors](https://github.com/llvm/llvm-project), [zlib authors Jean-loup Gailly and Mark Adler](https://github.com/madler/zlib) | Compilation and compression tooling |

### Emulator cores

| Core | Authors / maintainers credited by upstream |
| --- | --- |
| [FCEUmm](https://github.com/libretro/libretro-fceumm) | FCEU Team, CaH4e3 and contributors |
| [mGBA](https://github.com/libretro/mgba) | endrift and contributors |
| [Snes9x](https://github.com/libretro/snes9x) | Snes9x Team and contributors |
| [FinalBurn Neo](https://github.com/libretro/FBNeo) | Team FBNeo and contributors |
| [Genesis Plus GX](https://github.com/libretro/Genesis-Plus-GX) | Charles MacDonald, Eke-Eke and contributors |
| [PPSSPP](https://github.com/hrydgard/ppsspp) | Henrik Rydgård and contributors |
| [Dolphin](https://github.com/dolphin-emu/dolphin), [libretro/dolphin](https://github.com/libretro/dolphin) | Dolphin Emulator Project and contributors; libretro core maintainers |
| [PCSX2](https://github.com/PCSX2/pcsx2), [LRPS2](https://github.com/libretro/LRPS2) | PCSX2 Dev Team and contributors; libretro LRPS2 maintainers |
| [RPCS3](https://github.com/RPCS3/rpcs3) | RPCS3 Team and contributors |
| [libretro core-info](https://github.com/libretro/libretro-core-info) | Metadata maintainers and contributors |

## License and third-party terms

This repository's own code is **GPL-3.0-or-later** ([LICENSE](LICENSE)). Most source
files carry a copyright and SPDX notice; the ones that do not (for example
`src/memory_ps5.cpp` and the build scripts in `tools/`) are under the same licence.
Code inherited from BlackBearReloaded's ps5-native-app-boilerplate and ProsperoLight
is Copyright (C) 2026 BlackBearReloaded, GPL-3.0-or-later. The generated
`sce_module/libc.prx` is described in [runtime/README.md](runtime/README.md).

Every built title folder carries `licenses/`: the licence texts each part requires,
and `components.json`, which ties every executable file to the source revision it
was built from ([tooling/notices/components.json](tooling/notices/components.json),
[docs/RELEASING.md](docs/RELEASING.md)). Releases up to v0.5.0-alpha.5 were
published without it; [docs/releases/SOURCE_CORRESPONDENCE.md](docs/releases/SOURCE_CORRESPONDENCE.md)
records what each was built from.

The cores keep their own licences, and they differ:

| Licence | Cores |
| --- | --- |
| GPL-2.0-or-later | FCEUmm, PPSSPP, Dolphin, Beetle PSX HW, Beetle Saturn, Mupen64Plus-Next (with MIT and LGPL parts), VICE, DeSmuME, Azahar (Dynarmic is 0BSD), MAME (as a whole; many files BSD-3-Clause) |
| GPL-3.0-or-later | LRPS2 (PCSX2) |
| GPL-2.0-only | RPCS3: a console build only. Its source is public and it builds with this repository, but no release carries it until the question of combining it with this port's GPL-3.0 code is settled |
| MPL-2.0 | mGBA |
| Non-commercial licences | Snes9x, FinalBurn Neo, Genesis Plus GX: they may not be sold or used commercially, and FBNeo's forbids asking for donations for a project that uses its code |

Assets and fonts keep their licences too: the XMB theme is CC-BY-4.0 with the M+
font licence, PPSSPP's fonts are OFL-1.1, and Dolphin's `Sys` files carry theirs.

This is an independent homebrew project, not affiliated with or endorsed by Sony
Interactive Entertainment, the Khronos Group or the libretro project. PlayStation
and PS5 are Sony trademarks. Vulkan is a registered trademark of the Khronos Group
Inc.; the RADV port this title uses is not a Khronos-conformant product (see
[PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan)). RetroArch is the libretro
project's name and logo, used here to name the frontend this port is built from.
