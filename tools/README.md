# Development flasher

rdwr_phy62x2_melb.py is the project development flasher for the
Music-Light-V3-221101 / ST17H66B target.

It is derived from pvvx/THB2 rdwr_phy62x2.py and keeps the original ROM-UART
operations. Project additions are:

- stable artifact/dev firmware channel;
- manifest download before flashing;
- firmware size and SHA-256 verification;
- firmware/source identity printed before writing;
- --dev shortcut using 500000 baud by default;
- optional --monitor handoff to the runtime UART after reset;
- standalone --monitor mode.

The only non-standard Python dependency is pyserial.

Stable development manifest:
https://raw.githubusercontent.com/KobaProduction/music-equalizer-lightbar/artifact/dev/artifacts/Music-Light-V3-221101_ST17H66B_DEV.json

The manifest points at a fixed-name HEX on the same artifact/dev branch. The
branch is a mutable development channel: its URL stays constant while the
manifest records the exact source commit, size and SHA-256 of the current image.

## Attribution

The base flasher is by pvvx and originates from:
https://github.com/pvvx/THB2/blob/master/rdwr_phy62x2.py

The upstream repository separates project source from the restricted Phyplus
SDK. rdwr_phy62x2.py is treated as upstream source code under its permissive
SOURCE LICENSE; the license text used for this vendored tool is preserved in
pvvx_SOURCE_LICENSE.txt.

## Dev channel publishing

.github/workflows/publish-dev.yml watches successful build workflow runs from
debug/osal-heartbeat. It checks out the exact tested source SHA, rebuilds the
HappyLighting firmware, then updates the fixed DEV.hex and DEV.json files on
artifact/dev. The same publisher can also be run manually for an explicit ref.
