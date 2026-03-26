# JustType (Prototype Software)

<img src="assets/device.png" alt="Leaf device" width="300" />

I built this as a birthday gift for my daughter.

She had started writing stories and learning to type, and I wanted to support that without giving her a laptop full of distractions. Everything on the market was more than I wanted to spend (~$500), so I built a simple one myself for about $70 in parts: keyboard, screen, battery, and a single-purpose writing app.

This repository is the writing software used on the Raspberry Pi prototype.

If you want the build story and prototype timeline, start here:  
[justtypeleaf.com/journey](https://justtypeleaf.com/journey?entry=home)

If you want the 3D printable prototype case files, start here:  
[justtypeleaf.com/prototype-files](https://justtypeleaf.com/prototype-files?entry=home)

## Current Prototype Hardware

<img src="assets/inside.png" alt="Leaf device inside" width="220" />

- Raspberry Pi Zero 2W
- 7" 1024x600 display
- Slim wired keyboard
- 10,000mAh power bank
- 3D printed enclosure

## What This Software Does

- Launches straight into a writing interface (no desktop workflow)
- Auto-saves edits using a debounce timer: changes are buffered in memory, then flushed to disk after a short idle window to reduce write frequency while keeping data loss risk low
- Stores all files as plain text in the local `user_files/` folder
- Remembers editor state so the user can pick up where they left off
- Includes a file sidebar, in-editor search, and keyboard-first controls
- Includes built-in keyboard shortcuts focused on writing flow (for example: `Ctrl+Up` / `Ctrl+Down` paragraph jump, `Shift+Arrow` selection, `Ctrl+Backspace` delete previous word)
- Supports dark/light theme toggle and font size adjustment

<img src="assets/Video.gif" alt="JustType writing flow demo" width="300" />

<img src="assets/Video2.gif" alt="JustType writing flow demo 2" width="300" />

The app is intentionally focused and minimal.

## Build

### Dependencies

- CMake (3.16+)
- C++17 compiler
- SDL2 development package

### Build Commands

```bash
cmake -S . -B build
cmake --build build
```

## Run

Run from the `build/` directory so relative paths resolve correctly:

```bash
cd build
./justtype
```

The app expects these paths relative to the executable:

- `../fonts/OpenSans-Regular.ttf`
- `../user_files/`

## Project Direction

The Pi prototype validated the core idea: fast access to writing, no distractions.

The current product-design direction moves away from Linux toward an MCU-based stack.

### Hardware

- MCU firmware platform
- Compact 60% keyboard layout
- Folio-style enclosure
- Lid-open wake, lid-close sleep
- USB-C and/or multi-QR transfer
- Target boot/wake under 3 seconds
- Target market price of under $100

### Software (LVGL)

- New LVGL-based writing interface
- Folder support for organizing files
- QR export flow for file transfer
- Simple spellcheck for writing assistance


Follow the journey at 
[justtypeleaf.com/journey](https://justtypeleaf.com/journey?entry=home)
