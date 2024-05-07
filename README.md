# TiR Tweaks

A collection of minor tweaks & fixes for Total Immersion Racing

Includes a widescreen fix, increased shadow resolution, splash screen skip, and more.

![in-game preview](https://i.imgur.com/IRjqYRw.png)
![menu preview](https://i.imgur.com/q02pA1m.png)

## Installation

- Make sure you have the Nestle Edition v1.3 of the game, as this is the only version this plugin is compatible with. (exe size of 1366016 bytes)
- Plop the files into your game folder, edit `TIRTweaks_gcp.toml` to change the options to your liking.
- Enjoy, nya~ :3

## Feature List

- Disables the CD check at startup
- Skips the splash screens and intro video, resulting in the game getting to the main menu in around 1-2 seconds from startup
- Fixes the stretched view on aspect ratios other than 4:3
- Fixes the stretched HUD on aspect ratios other than 4:3 (WIP, unfinished)
- Adds letterboxing to splash screens and the intro video instead of stretching it
- Adds borderless windowed support
- Adds anti-aliasing support
- Makes the game use the monitor resolution without having to edit syscfg.ini
- Increases the resolution of car shadows
- Increases the resolution of reflections
- Re-enables 2 unused splash screens at startup
- Disables all background videos (unstable but can fix some crashes and lockups on Wine/Proton)
- Simple configuration file to enable or disable any of these options at any time

## Known problems

- Some menus are scaled oddly with `fix_hud` enabled
- Shadows seem to randomly disappear on some systems (no idea what the contributing factors are here, if I could get reports of this along with some HW info I'd appreciate it ^^)

## Building

Building is done on an Arch Linux system with CLion and vcpkg being used for the build process. 

Before you begin, clone [nya-common](https://github.com/gaycoderprincess/nya-common) to a folder next to this one, so it can be found.

Required packages: `mingw-w64-gcc vcpkg`

To install all dependencies, use:
```console
vcpkg install tomlplusplus:x86-mingw-static
```

Once installed, copy files from `~/.vcpkg/vcpkg/installed/x86-mingw-static/`:

- `include` dir to `nya-common/3rdparty`
- `lib` dir to `nya-common/lib32`

You should be able to build the project now in CLion.
