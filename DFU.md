# Device Firmware Update over USB

DeskHop devices support [USB DFU (Device Firmware
Update)](https://en.wikipedia.org/wiki/USB#Device_Firmware_Upgrade_mechanism)
version
[1.1](https://web.archive.org/web/20141011015811/http://www.usb.org/developers/docs/devclass_docs/DFU_1.1.pdf). DFU
can be used to both upload (device to host) and download (host to
device) firmware and configuration data.

## Tools and modes

There are a variety of DFU tools available for various platforms. The
DeskHop implementation has been tested with [dfu-util version
0.11](https://dfu-util.sourceforge.net/). This tool is available as a
regular package in most Linux distributions; it can be installed on
macOS using Homebrew; and it can be installed on Windows using
packages from the project site.

All of the examples in this documentation use `dfu-util` but the
commands can be adapted to other tools.

On Linux systems, `dfu-util` requires root access, or sufficient
permissions to the USB devices to allow raw USB data transfers.

DFU operates in two modes: 'runtime' and 'transfer'.

### 'runtime' mode

DFU 'runtime' mode is an additional feature available during DeskHop's
normal operation (as a USB HID). This mode allows the host software to
trigger a switch to DFU 'transfer' mode in order to initiate uploads or
downloads. Because this can introduce a security concern, it is
disabled by default but can be enabled by setting `DFU_RT_ENABLED` to
`1` in `user_config.h` when building the DeskHop firmware.

### 'transfer' mode

DFU 'transfer' mode allows uploads and downloads. It can be entered from
DFU 'runtime' mode by using a DFU host tool to trigger a `DETACH`
operation, or by using the key sequences `Left Shift-Right Shift-F12-C`
(for board A) and `Left Shift-Right Shift-F12-D` (for board B).

Once this mode has been entered, the DeskHop will stay in the mode for
a maximum of 30 seconds, or until an upload or download has been
completed. Also note that once this mode has been entered on a board
the normal operation of that board (keyboard or mouse input, and
output to the host) will stop. Alternative input device(s) will need
to be connected to the host in order to issue commands to execute the
DFU transfer operations.

## Firwmare management with 'runtime' mode

### Backup/Archive

To backup the firmware from a DeskHop device:

(command run on host connected to board A)

```shell
$ dfu-util -a board_a_fw -d 2E8A:107C -U firmware_a.bin
```

(command run on host connected to board B)

```shell
$ dfu-util -a board_b_fw -d 2E8A:107C -U firmware_b.bin
```

These commands will switch the DeskHop into DFU 'transfer' mode,
upload the firmware from the DeskHop's flash memory, and then the
DeskHop will reboot back into normal operation.

### Upgrade

To upgrade the firmware on a DeskHop device:

(command run on host connected to board A)

```shell
$ dfu-util -a board_a_fw -d 2E8A:107C -D board_A.bin
```

(command run on host connected to board B)

```shell
$ dfu-util -a board_b_fw -d 2E8A:107C -U board_B.bin
```

The firmware filenames listed above are the usual names produced by
the DeskHop firmware build process; do not attempt to upload the
`.uf2`, `.elf`, or `.hex` versions of the firmware file, as doing so
will result in a non-operational DeskHop board and recovery will
require using the `BOOTSEL` button to boot the board into USB recovery
mode.

These commands will switch the DeskHop into DFU 'transfer' mode,
download the firmware into the DeskHop's flash memory, and then the
DeskHop will reboot back into normal operation.

## Firwmare management without 'runtime' mode

### Backup/Archive

To backup the firmware from a DeskHop device, switch one of the
DeskHop boards into 'transfer' mode using the key sequence documented
above. Within 30 seconds:

(command run on host connected to board A)

```shell
$ dfu-util -a board_a_fw -d 2E8A:XXXX -U firmware_a.bin
```

(command run on host connected to board B)

```shell
$ dfu-util -a board_b_fw -d 2E8A:XXXX -U firmware_b.bin
```

These commands will upload the firmware from the DeskHop's flash
memory, and then the DeskHop will reboot back into normal operation.

### Upgrade

To upgrade the firmware on a DeskHop device, switch one of the DeskHop
boards into 'transfer' mode using the key sequence documented
above. Within 30 seconds:

(command run on host connected to board A)

```shell
$ dfu-util -a board_a_fw -d 2E8A:107C -D board_A.bin
```

(command run on host connected to board B)

```shell
$ dfu-util -a board_b_fw -d 2E8A:107C -U board_B.bin
```

The firmware filenames listed above are the usual names produced by
the DeskHop firmware build process; do not attempt to upload the
`.uf2`, `.elf`, or `.hex` versions of the firmware file, as doing so
will result in a non-operational DeskHop board and recovery will
require using the `BOOTSEL` button to boot the board into USB recovery
mode.

These commands will download the firmware into the DeskHop's flash
memory, and then the DeskHop will reboot back into normal operation.

## Configuration management with 'runtime' mode

### Backup

To backup the configuration from a DeskHop device:

(command run on host connected to either board, since they have
identical configurations)

```shell
$ dfu-util -a config -d 2E8A:107C -U config.bin
```

This command will switch the DeskHop into DFU 'transfer' mode, upload
the configuration from the DeskHop's flash memory, and then the
DeskHop will reboot back into normal operation.

Note that the configuration data is a binary data structure; it is not
human-readable and there are no tools available to convert it into a
human-readable format.

### Restore

To restore the configuration on a DeskHop device (must be done for
both boards):

(command run on host connected to board A)

```shell
$ dfu-util -a config -d 2E8A:107C -D config.bin
```

(command run on host connected to board B)

```shell
$ dfu-util -a config -d 2E8A:107C -D config.bin
```

This command will switch the DeskHop into DFU 'transfer' mode,
download the configuration into the DeskHop's flash memory, and then
the DeskHop will reboot back into normal operation.

## Configuration management without 'runtime' mode

### Backup/Archive

To backup the configuration from a DeskHop device, switch one of the
DeskHop boards into 'transfer' mode using the key sequence documented
above. Within 30 seconds:

(command run on host connected to the board in 'transfer' mode, since
they have identical configurations)

```shell
$ dfu-util -a config -d 2E8A:XXXX -U config.bin
```

This command will upload the configuration from the DeskHop's flash
memory, and then the DeskHop will reboot back into normal operation.

Note that the configuration data is a binary data structure; it is not
human-readable and there are no tools available to convert it into a
human-readable format.

### Restore

To restore the configurartion on a DeskHop device, switch one of the DeskHop
boards into 'transfer' mode using the key sequence documented
above. Within 30 seconds:

(command run on host connected to the board in 'transfer' mode)

```shell
$ dfu-util -a config -d 2E8A:XXXX -D config.bin
```

This command will download the configuration into the DeskHop's flash
memory, and then the DeskHop will reboot back into normal operation.

Repeat this sequence for the other board in the DeskHop device so that
the configuration on the boards will be identical.
