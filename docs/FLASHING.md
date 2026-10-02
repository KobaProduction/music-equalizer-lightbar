# ST17H66B ROM flashing contract

This document records the current flashing model for the `Music-Light-V3-221101` target. It is a bring-up contract, not a claim that the generated image has already passed hardware validation.

## ROM UART

Public ST17H66B/PHY62x2 tooling establishes the following ROM-loader behavior:

- initial serial rate: 9600 baud;
- loader wake/handshake sequence: `UXTDWU`;
- successful command prompt: `cmd>>:`;
- protected/FCT prompt may appear as `fct>>:`;
- the loader can subsequently switch to a higher baud rate;
- ROM loader remains independent from ordinary application Flash contents.

The external reference implementation currently used for protocol evidence is:

https://github.com/pvvx/THB2/blob/master/rdwr_phy62x2.py

It is not vendored into this repository.

## Current release HEX contract

The current bring-up linker places the runnable image at:

`0x1FFF1838`

The generated Intel HEX therefore describes an SRAM-resident section beginning at that address.

The reference `rdwr_phy62x2.py` `wh` path uses these defaults for ST17H66B/PHY62x2:

- run address: `0x1FFF1838`;
- Boot Info/header Flash offset: `0x00002000`;
- first storage offset for SRAM payload: `0x00005000`.

That means our generated `.hex` is the correct input class for the reference `wh` writer: the writer generates the ROM Boot Info/header and maps the SRAM section into Flash storage.

This is source-confirmed compatibility with the writer's model, not yet a hardware-pass result for `Music-Light-V3-221101`.

## Safety gate: preserve original Flash first

No application write should be treated as the first hardware operation.

The bring-up order is:

1. confirm the two board programming pads by continuity to ST17H66B package pins 5/P9 and 6/P10;
2. enter ROM UART and read chip/revision information;
3. read and preserve the complete original Flash;
4. verify that the backup is non-empty and has the expected full-Flash size;
5. only then test a project-generated image.

If ROM responds with `fct>>:`, do not perform a full-chip erase merely to continue. That state is a separate decision boundary because erase destroys the original firmware.

## Build-time validation

The repository build validates the generated BIN/HEX pair before CI accepts it:

- first image address is `0x1FFF1838`;
- initial stack pointer is `0x1FFFA000`;
- reset vector is a Thumb address inside the image;
- HEX bytes match the BIN representation;
- the image stays inside the small experimental SRAM bring-up region.

These checks prove internal image consistency only.

## Future production layout

The small SRAM-only image is suitable for early GPIO/loader bring-up, not for the final BLE firmware.

The BLE milestone will require a larger validated SRAM/XIP layout and ROM-symbol contract. That transition must happen explicitly; the current linker layout must not silently become the production layout.
