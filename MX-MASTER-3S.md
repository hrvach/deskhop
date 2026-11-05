# Logitech MX Master 3S custom firmware

This firmware was tested successfully on both DeskHop boards with an MX Master 3S receiver. Shortcuts target Windows.

| Button | Action |
| --- | --- |
| Back side button | Ctrl+V (Paste) |
| Forward side button | Ctrl+C (Copy) |
| Thumb gesture button | Toggle gaming mode |
| Top square button | Windows+V (clipboard history) |
| Wheel click | Switch active computer (respects switch lock) |

The receiver plugs into DeskHop, rather than directly into a computer. Flash the same UF2 onto both boards.

## Firmware

`firmware/deskhop-mx-custom-buttons.uf2` is the working binary confirmed by the user.

SHA-256: `756d1e6805f042089e7856d22c83f3d2af3a950f3f516eed04076f317f81c255`

## Build

Use CMake and an Arm GNU embedded toolchain, with the bundled Pico SDK and Pico-PIO-USB dependencies:

```sh
cmake -S . -B build
cmake --build build
```

Output: `build/deskhop.uf2`. See README.md for prerequisites.

## Implementation and limits

HID++ button setup uses control SET_REPORT transfers with a persistent buffer, independent button acknowledgments, retries from the USB host loop, and cleanup on receiver removal. Synthetic shortcut reports are merged with keyboard state.

Receiver device index **2** and REPROG_CONTROLS_V4 feature index **9** were queried and verified on this receiver. Thumb CID is `0x00C3`; top CID is `0x00C4`. These indexes remain fixed in `src/usb.c`; another pairing or receiver may require different indexes.

Windows clipboard history must be available for Windows+V. Gaming mode follows the existing DeskHop behavior. Wheel click is consumed for switching instead of being sent as a middle click.
