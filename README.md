# JC4827W543 Animated GIF Player

Touch-driven animated GIF browser/player for the JC4827W543 display using an ESP32-S3, SD card storage, and optional PSRAM acceleration.

## Demo / Tutorial

[<img src="https://github.com/thelastoutpostworkshop/images/blob/main/Wiring%20Included-1.png" width="500" alt="Video tutorial">](https://youtu.be/mnOzfRFQJIM)

If this project helped you:

<a href="https://www.buymeacoffee.com/thelastoutpostworkshop" target="_blank">
<img src="https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png" alt="Buy Me A Coffee">
</a>

## Features

- Touch UI to browse GIF files with left/right arrows
- One-tap play button to launch selected animation
- Auto scan of all `.gif`/`.GIF` files in `/gif` on SD card
- PSRAM-aware playback path (preload to PSRAM when possible, otherwise stream from SD)
- On-screen file title and simple playback status messages
- Serial debug output for file loading, touch coordinates, and errors

## What You Can Tinker With

These are the highest-impact knobs in `JC4827W543_AnimatedGIF.ino`:

| Setting | What it changes | Current value |
|---|---|---|
| `GIF_FOLDER` | SD folder scanned for animations | `"/gif"` |
| `MAX_FILES` | Max number of GIFs indexed from SD | `20` |
| `PSRAM_RESERVE_SIZE` | How much PSRAM to keep free for system use | `100 * 1024` bytes |
| `touchController.setRotation(...)` | Touch orientation/alignment | `ROTATION_INVERTED` |
| `TOUCH_SDA`, `TOUCH_SCL`, `TOUCH_INT`, `TOUCH_RST` | Touch controller wiring pins | `8`, `4`, `3`, `38` |
| `TOUCH_WIDTH`, `TOUCH_HEIGHT` | Touch coordinate space | `480`, `272` |
| `arrowSize`, `margin`, `playButtonSize` | UI sizing and button placement | `40`, `10`, `50` |
| `gif.playFrame(false, NULL)` | Frame timing mode | `false` |
| `gfx->setFont(...)` | Title font on selection screen | `FreeSansBold12pt7b` |
| `SD.begin(..., 10000000)` | SD SPI clock speed | `10 MHz` |

Quick ideas to try:

- Increase `MAX_FILES` if your SD card has more than 20 GIFs.
- Lower `PSRAM_RESERVE_SIZE` to preload bigger GIFs into PSRAM (faster playback).
- Change `gif.playFrame(false, NULL)` to `true` for GIF-internal frame timing behavior.
- Adjust `playButtonSize` and `arrowSize` for better touch ergonomics.
- Change `GIF_FOLDER` if you want multiple SD content profiles.

## Requirements

### Board

- ESP32-S3 (use `ESP32S3 Dev Module`)
- PSRAM enabled (`OPI PSRAM`) for best performance

### Libraries

Install with Arduino Library Manager:

- `Dev Device Pins` (tested with `0.0.2`)
- `GFX Library for Arduino` (tested with `1.5.6`)
- `AnimatedGIF` (tested with `2.2.0`)
- `TAMC_GT911` (tested with `1.0.2`)

Core/toolchain used in this project:

- `esp32:esp32` platform `3.2.0`

## SD Card Layout

Copy GIFs into a `gif` folder at SD root:

```text
SD:/
  gif/
    your-file-1.gif
    your-file-2.gif
```

This repo includes sample files under `SD Content/gif/`.

## Build and Upload

1. Open `JC4827W543_AnimatedGIF.ino` in Arduino IDE.
2. Select board: `ESP32S3 Dev Module`.
3. Enable PSRAM (`OPI PSRAM`) in board options.
4. Install required libraries.
5. Insert SD card with the `gif` folder.
6. Upload and open Serial Monitor (`115200`) for logs.

## Touch Controls

- Tap left arrow: previous GIF
- Tap right arrow: next GIF
- Tap play button: play selected GIF

After playback ends, the file selection UI returns automatically.

## Troubleshooting

- `ERROR: SD Card mount failed!`: check SD wiring, card format, and initialization speed.
- `No PSRAM found`: enable PSRAM in board config, or expect more SD streaming fallback.
- `Failed to open GIF folder`: ensure `/gif` exists on SD root (or update `GIF_FOLDER`).
- Slow playback: reduce GIF size/frame complexity, or lower `PSRAM_RESERVE_SIZE` to allow larger preload buffer.
