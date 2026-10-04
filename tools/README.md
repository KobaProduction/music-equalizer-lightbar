# PHY62x2 Firmware Tool

phytool.py is the repository firmware utility for PHY62x2/ST17H66B ROM-UART
bring-up, flashing, full-Flash backup and runtime UART monitoring.

The project-facing CLI is intentionally small:

- flash — flash an Intel HEX from a local path or HTTP/HTTPS URL;
- dump — save the complete external Flash to a local binary file;
- monitor — open the runtime UART;
- info — print chip/Flash information.

## Flash

Local file:

```powershell
python phytool.py -p COM5 flash --target=".\\firmware.hex" --monitor
```

Web URL:

```powershell
python phytool.py -p COM5 flash --target="https://example.invalid/firmware.hex" --monitor
```

For a HEX target, the tool automatically probes a sibling manifest with the
same basename and a .json suffix. firmware.hex maps to firmware.json, including
for web URLs. Missing local JSON files and HTTP 404 are treated as optional and
flashing continues without a manifest. Other manifest errors are reported.

A manifest may provide label, board, mcu, source_commit, validation, size,
sha256, and a firmware reference. For an explicit HEX --target, that target
remains authoritative; an auto-discovered manifest may validate and describe it
but may not silently redirect flashing to another artifact.

A JSON manifest may itself be the target:

```powershell
python phytool.py -p COM5 flash --target="https://example.invalid/firmware.json" --monitor
```

Use --manifest to override auto-discovery, --no-manifest to disable it, or
--sha256 to supply an expected digest directly.

Remote flashing defaults to 500000 baud. With --monitor, after the ROM reset
command is flushed the same open serial handle is switched immediately to
runtime 115200 baud and starts reading. There is no close/reopen cycle and no
intentional post-reset delay.

## Dump

```powershell
python phytool.py -p COM5 dump --target=".\\board-backup.bin"
```

Dump transfer defaults to 500000 baud.

## Monitor

```powershell
python phytool.py -p COM5 monitor
```

The default runtime baud is 115200.

## Attribution

The ROM-UART implementation is derived from:

https://github.com/pvvx/THB2/blob/master/rdwr_phy62x2.py

The upstream source attribution and permissive source license used for the
modified implementation are preserved in UPSTREAM_LICENSE.txt.
