# Technical provenance and licensing notes

This file records external sources used as technical evidence during bring-up. A reference here does not imply that its source code has been copied into this repository.

## pvvx/THB2

Repository:

https://github.com/pvvx/THB2

Used as evidence for:

- ST17H66B support in the PHY62x2 ecosystem;
- ROM UART programming workflow;
- P09/P10 UART convention on known ST17H66B hardware;
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
- board metadata;
- WS2812B protocol serialization;
- CI/release workflows.

They are not claimed to be vendor reference implementations and remain subject to hardware validation.
