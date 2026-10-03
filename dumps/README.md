# Original firmware dumps

## Music-Light-V3-221101 / ST17H66B

Original factory firmware dump preserved before any erase or write operation.

File:

`Music-Light-V3-221101_ST17H66B_PHY6222M005_original-firmware-dump_256KiB.bin`

Hardware / ROM identification observed during readout:

- PCB marking: `Music-Light-V3-221101`
- MCU: `ST17H66B`
- ROM / PHY identification: `PHY6222`
- Revision response: `00124485 6222M005`
- Flash ID: `0x124485`
- Flash size: `256 KiB` (`0x40000` bytes)
- Flash mapped read address: `0x11000000`
- ROM prompt: `cmd>>:`

Readout tool:

- `pvvx/THB2/rdwr_phy62x2.py`
- utility version reported during capture: `08.02.25`
- operation used: `rf` / Read all Flash

Successful readout summary:

```text
Chip Reset Ok. Response: b'cmd>>:'
Revision: b'00124485 6222M005'
FlashID: 124485, size: 256 kbytes
PHY6222 - connected Ok
Read at 0x11000000, size: 0x00040000:
Read 0x1103ff80... ok
256.000 KBytes saved to file 'original.bin'
```

SHA-256:

`059097fbf535af3e9bafe45b1a0fb698b790ca007e41175260c92b8ee8470ba3`

This dump is archival evidence of the factory firmware. Do not modify it in place.
