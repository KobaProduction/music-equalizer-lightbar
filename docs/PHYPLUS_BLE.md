# External Phyplus BLE integration

## Purpose

The ST17H66B BLE backend follows the proven GCC path used by pvvx/THB2.

CMake fetches a pinned THB2 revision into the build tree with `FetchContent`; users do not need to provide a local SDK path. The fetched third-party tree keeps its original upstream licenses and is not copied into this repository.

Pinned upstream revision:

`48db5245d235aef57cfdf5fcc58ec74fa753d89c`

Repository:

https://github.com/pvvx/THB2

The SDK is resolved from `bthome_phy6222/SDK` inside that fetched tree.

## Build the profile check

```sh
cmake --preset phyplus-profile-check
cmake --build --preset phyplus-profile-check
```

The configure step automatically downloads the pinned THB2 revision when it is not already present in the CMake FetchContent cache.

For an offline/local-development override, standard CMake FetchContent behavior may be used with `FETCHCONTENT_SOURCE_DIR_THB2`; this is optional and is not required for the normal build.

## License boundary

The THB2 root license separates permissive project/source material from Phyplus SDK material with separate vendor terms.

The project therefore:

- pins and fetches the upstream repository rather than vendoring the SDK;
- preserves the upstream source and license files unchanged in the build tree;
- does not relicense SDK sources as part of this project's MIT code;
- keeps project-owned GATT/application code in this repository.

Users remain responsible for the terms applicable to the fetched Phyplus SDK.

## Project-owned GATT profile

The project-owned adapter registers:

- primary service `0xFFF0`;
- writable characteristic `0xFFF3`;
- both Write Request and Write Without Response properties.

A successful write is passed through `lotus_lantern_apply_frame()`. The backend then invokes a project callback with the updated `melb_control_state_t`.

This keeps three layers separate:

1. fetched Phyplus BLE stack / radio;
2. project-owned GATT transport glue;
3. project-owned Lotus Lantern command semantics.

## Validation boundary

`phyplus-profile-check` compiles the project-owned profile against the actual fetched Phyplus headers.

A full BLE firmware still additionally requires the stack/RF/startup/linker integration used by the pvvx GCC build. That is a separate validation level from the profile compile-check and from hardware radio validation.
