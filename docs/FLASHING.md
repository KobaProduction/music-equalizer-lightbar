# ST17H66B ROM flashing contract

This document records the current flashing model for the `Music-Light-V3-221101` target. It is a bring-up contract, not a claim that the generated image has passed hardware validation.

## ROM UART

Public ST17H66B/PHY62x2 tooling establishes the following ROM-loader behavior:

- initial serial rate used by the reference tool: 9600 baud;
- wake/handshake sequence: `UXTDWU`;
- successful command prompt: `cmd>>:`;
- protected/FCT prompt may appear as `fct>>:`;
- the loader can subsequently switch to a higher baud rate;
- the ROM loader is independent from ordinary application Flash contents.

The 9600-baud ROM-loader entry has not yet been reproduced on this specific `Music-Light-V3-221101` board. The confirmed original-application debug UART is 115200 baud and is documented separately in [RUNTIME_UART.md](RUNTIME_UART.md).

Reference tooling:

https://github.com/pvvx/THB2/blob/master/rdwr_phy62x2.py

The tool is not vendored into this repository.

## Read and preserve the original Flash

pvvx/THB2 provides an explicit `rf` operation: **Read all Flash**.

Install the only Python dependency:

```sh
python -m pip install pyserial
```

With P9/P10/GND connected to a 3.3 V USB-UART adapter, start a full-Flash read:

```sh
python rdwr_phy62x2.py -p COM5 rf music-light-v3-original.bin
```

Replace `COM5` with the actual serial port. On Linux this may be a device such as `/dev/ttyUSB0`.

Do **not** add any of these options while preserving the original image:

- `-a` / `--allerase`;
- `-e` / `--erase`;
- write operations `wh`, `we`, or `wf`.

The optional global `-r` flag means **reset after the operation**; it does not mean read. pvvx documents the equivalent convenience command:

```sh
python3 rdwr_phy62x2.py -p COM5 -r rf music-light-v3-original.bin
```

For the first preservation pass, omitting `-r` is preferable so the application does not automatically run again immediately after the dump.

### Entering the loader on ST17H66B

The reference script opens the port and repeatedly sends the ROM handshake while waiting for the chip.

For an ST17H66B board without adapter-controlled reset/TM:

1. connect adapter RX to target P9/TX;
2. connect adapter TX to target P10/RX;
3. connect common GND;
4. start the `rf` command;
5. when the tool is waiting for the chip, power-cycle or apply power to the board.

A successful loader entry reports `cmd>>:` and then chip/revision information.

If the response is `fct>>:`, stop. The reference utility will suggest a full erase to leave FCT mode, but that erase would destroy the firmware we are trying to preserve.

### Validate the backup

After the dump:

1. record the exact file size;
2. compute a SHA-256 hash;
3. keep at least two copies;
4. ideally repeat the dump before any write operation and compare the two hashes.

Examples:

```sh
sha256sum music-light-v3-original.bin
```

PowerShell:

```powershell
Get-FileHash .\music-light-v3-original.bin -Algorithm SHA256
```

A successful `rf` operation reports the detected Flash size and the number of kilobytes saved. Do not assume a nominal 256 KiB or 512 KiB size before the tool reads the actual Flash ID.

## Current HEX contract

The current experimental linker places the runnable image at:

`0x1FFF1838`

The generated Intel HEX therefore describes an SRAM-resident section beginning at that address.

The reference `rdwr_phy62x2.py` HEX writer uses the same run-address model and generates the Boot Info/header plus Flash storage mapping for SRAM sections.

This is source-level compatibility with the reference writer model, not yet a hardware-pass result for `Music-Light-V3-221101`.

## Safety gate: preserve original Flash first

No application write is the first hardware operation.

Bring-up order:

1. use the confirmed P9/P10 programming pads;
2. enter ROM UART and read chip/revision information;
3. read and preserve the complete original Flash;
4. verify the backup size/hash and preferably obtain a second matching dump;
5. only then test a project-generated image.

If ROM responds with `fct>>:`, do not perform a full-chip erase merely to continue. That state is a separate decision boundary because erase destroys the original firmware.

## Build-time image validation

The repository validates the generated BIN/HEX pair before CI accepts it:

- first image address is `0x1FFF1838`;
- initial stack pointer is `0x1FFFA000`;
- reset vector is a Thumb address inside the image;
- HEX bytes match the BIN representation;
- image stays inside the small experimental SRAM bring-up region.

These checks prove internal image consistency only.

## Future BLE layout

The small SRAM-only image is for early loader/GPIO bring-up, not the final BLE firmware.

BLE will require a larger validated SRAM/XIP layout and ROM-symbol contract. The external Phyplus/pvvx GCC integration is documented in [PHYPLUS_BLE.md](PHYPLUS_BLE.md).
