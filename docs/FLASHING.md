# ST17H66B ROM flashing contract

This document records the current flashing model for the `Music-Light-V3-221101` target. It is a bring-up contract, not a claim that the generated image has passed hardware validation.

## ROM UART

Public ST17H66B/PHY62x2 tooling establishes the following ROM-loader behavior:

- initial serial rate: 9600 baud;
- wake/handshake sequence: `UXTDWU`;
- successful command prompt: `cmd>>:`;
- protected/FCT prompt may appear as `fct>>:`;
- the loader can subsequently switch to a higher baud rate;
- the ROM loader is independent from ordinary application Flash contents.

Reference tooling used as protocol evidence:

https://github.com/pvvx/THB2/blob/master/rdwr_phy62x2.py

It is not vendored into this repository.

## Current HEX contract

The current experimental linker places the runnable image at:

`0x1FFF1838`

The generated Intel HEX therefore describes an SRAM-resident section beginning at that address.

The reference `rdwr_phy62x2.py` HEX writer uses the same run-address model and generates the Boot Info/header plus Flash storage mapping for SRAM sections.

This is source-level compatibility with the reference writer model, not yet a hardware-pass result for `Music-Light-V3-221101`.

## Safety gate: preserve original Flash first

No application write is the first hardware operation.

Bring-up order:

1. confirm the two unlabeled board programming pads against P9/P10;
2. enter ROM UART and read chip/revision information;
3. read and preserve the complete original Flash;
4. verify the backup is non-empty and complete;
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

BLE will require a larger validated SRAM/XIP layout and ROM-symbol contract. That transition must be explicit; the current linker layout must not silently become the production layout.
