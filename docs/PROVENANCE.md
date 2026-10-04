# Technical provenance and licensing notes

This file records external sources used as technical evidence during bring-up. A reference here does not imply that its source code has been copied into this repository.

## pvvx/THB2

Repository:

https://github.com/pvvx/THB2

Used as evidence for:

- ST17H66B support in the PHY62x2 ecosystem;
- ROM UART programming workflow and handshake;
- P09/P10 UART convention on known ST17H66B hardware;
- HEX writer run-address and Flash-storage model;
- GCC-based BLE host/controller build feasibility;
- larger SRAM/XIP firmware layout patterns.

License boundary observed in that repository:

- project/source material is presented under a permissive license;
- material identified as Phyplus SDK is subject to restrictive vendor terms.

Policy here:

- do not vendor the restricted SDK tree;
- review individual non-SDK files before reuse;
- prefer project-owned reimplementation where the license boundary is unclear.

## biemster/st17h66_blinky

Repository:

https://github.com/biemster/st17h66_blinky

Used as evidence for:

- successful `arm-none-eabi-gcc` execution on ST17H66-class hardware;
- the small early bring-up address model around `0x1FFF1838` and `0x1FFF8000`;
- P9/P10 UART usage;
- ROM-UART-based development workflows.

No source from that repository is treated as project-owned code here. The repository currently does not expose a root license file, so source reuse is not assumed to be permitted.

## biemster/ST17H66_SDK

Repository:

https://github.com/biemster/ST17H66_SDK

Used as reference evidence for:

- original ST17H66 SDK organization;
- BLE host/controller/profile API surface;
- vendor library boundaries.

Policy here:

- reference-only until an applicable redistribution license is proven;
- do not copy vendor binaries or restricted SDK source into this repository.

## Repository-owned implementation

The following are project-owned implementations in this repository:

- CMake target/build logic;
- minimal Cortex-M0 reset/vector startup;
- experimental linker script;
- image-consistency validation script;
- firmware manifest generation;
- board metadata;
- WS2812B protocol serialization;
- application BLE control packet parser/state;
- CI/release workflows.

They are not claimed to be vendor reference implementations and remain subject to hardware validation.


## External Phyplus BLE build dependency

The repository compile-checks its project-owned HappyLighting/Triones GATT profile against a pinned pvvx/THB2 revision fetched by CMake FetchContent.

The Phyplus SDK is not committed into this repository and retains its upstream vendor license inside the fetched build tree. See [PHYPLUS_BLE.md](PHYPLUS_BLE.md).


## Full BLE bring-up substrate

The `music-light-v3-221101-happylighting-ble` target prepares a temporary build-tree overlay from the pinned pvvx/THB2 `bthome_phy6222` source.

The overlay is not committed as project-owned source. Upstream pvvx source and Phyplus SDK material retain their respective licenses. Project-owned replacements/additions are limited to the HappyLighting/Triones GATT adapter, protocol/state code, board renderer and overlay preparation logic.


## HappyLighting / Triones protocol references

The HappyLighting/Triones compatibility contract is based on independently documented/reverse-engineered behavior, cross-checked against:

- https://github.com/Dazaike/lighting
- https://github.com/sysofwan/ha-triones

No source from those clients/integrations is copied into the firmware. The repository-owned implementation uses the documented UUIDs and wire-level command semantics for interoperability.

## Vendored pvvx ROM-UART flasher

The file tools/melb_tool.py is a modified copy of pvvx/THB2
rdwr_phy62x2.py. The upstream file is source material covered by the permissive
SOURCE LICENSE published in the THB2 repository; that license text and original
author attribution are preserved in tools/pvvx_SOURCE_LICENSE.txt.

The project modifications add only development-channel download/verification
and UART-monitor workflow around the existing ROM-UART flasher. No Phyplus SDK
source is copied into this tool.
