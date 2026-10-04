# MELB Firmware Tool

`melb_tool.py` is the project firmware utility for the
`Music-Light-V3-221101` / ST17H66B target.

The normal development path is explicit and URL-driven. The tool does not
hard-code a repository artifact, branch, release, or development channel.

Example: download and flash an explicit HEX, then immediately continue on the
same open COM port as a 115200 runtime UART monitor:

```powershell
python melb_tool.py -p COM5 --url "https://example.invalid/firmware.hex" --monitor
```

Optional verification can be supplied either directly:

```powershell
python melb_tool.py -p COM5 --url "https://example.invalid/firmware.hex" --sha256 <sha256> --monitor
```

or through a JSON manifest:

```powershell
python melb_tool.py -p COM5 --url "https://example.invalid/firmware.hex" --manifest-url "https://example.invalid/firmware.json" --monitor
```

If a manifest contains `firmware_url`, `--url` may be omitted. Supported
manifest metadata includes `label`, `board`, `mcu`, `source_commit`,
`validation`, `size`, and `sha256`.

Remote flashing uses 500000 baud by default. After the ROM reset command the
tool keeps the same serial handle open, flushes the reset command at the ROM
baud, switches that handle directly to the runtime baud (115200 by default),
and starts streaming immediately. There is no intentional post-reset delay or
close/reopen cycle.

Standalone monitor mode remains available:

```powershell
python melb_tool.py -p COM5 --monitor
```

The existing local ROM-UART operations (`wh`, `we`, `wf`, `rf`, etc.) remain
available for local files, backup, erase, and low-level bring-up work.

A future release resolver may provide a convenient default/latest release
selection, but it must resolve to an explicit artifact URL/manifest rather than
embedding a mutable development artifact in the tool.

## Attribution

This utility is derived from the ROM-UART utility at:

https://github.com/pvvx/THB2/blob/master/rdwr_phy62x2.py

The upstream source attribution and permissive source license used for this
modified tool are preserved in `pvvx_SOURCE_LICENSE.txt`.
