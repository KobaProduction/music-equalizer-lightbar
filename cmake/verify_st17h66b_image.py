#!/usr/bin/env python3

import argparse
import struct
from pathlib import Path

IMAGE_ORIGIN = 0x1FFF1838
IMAGE_LIMIT = IMAGE_ORIGIN + 0x1F40
RAM_ORIGIN = 0x1FFF8000
RAM_SIZE = 0x2000
EXPECTED_STACK = RAM_ORIGIN + RAM_SIZE


def parse_ihex(path: Path) -> dict[int, int]:
    memory: dict[int, int] = {}
    upper = 0
    eof_seen = False

    for lineno, raw in enumerate(path.read_text(encoding="ascii").splitlines(), 1):
        line = raw.strip()
        if not line:
            continue
        if not line.startswith(":"):
            raise ValueError(f"{path}:{lineno}: invalid Intel HEX record")

        record = bytes.fromhex(line[1:])
        if len(record) < 5:
            raise ValueError(f"{path}:{lineno}: short Intel HEX record")

        length = record[0]
        if len(record) != length + 5:
            raise ValueError(f"{path}:{lineno}: invalid Intel HEX length")
        if sum(record) & 0xFF:
            raise ValueError(f"{path}:{lineno}: Intel HEX checksum mismatch")

        offset = (record[1] << 8) | record[2]
        record_type = record[3]
        payload = record[4:4 + length]

        if record_type == 0x00:
            base = upper + offset
            for index, value in enumerate(payload):
                address = base + index
                if address in memory:
                    raise ValueError(
                        f"{path}:{lineno}: overlapping data at 0x{address:08X}"
                    )
                memory[address] = value
        elif record_type == 0x01:
            eof_seen = True
            break
        elif record_type == 0x04:
            if length != 2:
                raise ValueError(
                    f"{path}:{lineno}: invalid extended-linear-address record"
                )
            upper = int.from_bytes(payload, "big") << 16
        elif record_type in (0x03, 0x05):
            continue
        else:
            raise ValueError(
                f"{path}:{lineno}: unsupported Intel HEX record type 0x{record_type:02X}"
            )

    if not eof_seen:
        raise ValueError(f"{path}: Intel HEX EOF record missing")
    if not memory:
        raise ValueError(f"{path}: Intel HEX contains no data")

    return memory


def verify(bin_path: Path, hex_path: Path) -> None:
    image = bin_path.read_bytes()

    if len(image) < 8:
        raise ValueError(f"{bin_path}: image is too short for a vector table")
    if len(image) > IMAGE_LIMIT - IMAGE_ORIGIN:
        raise ValueError(
            f"{bin_path}: image size 0x{len(image):X} exceeds bring-up region"
        )

    stack_pointer, reset_vector = struct.unpack_from("<II", image, 0)

    if stack_pointer != EXPECTED_STACK:
        raise ValueError(
            f"{bin_path}: initial SP 0x{stack_pointer:08X}, "
            f"expected 0x{EXPECTED_STACK:08X}"
        )

    if (reset_vector & 1) == 0:
        raise ValueError(
            f"{bin_path}: reset vector 0x{reset_vector:08X} is not Thumb"
        )

    reset_address = reset_vector & ~1
    if not (IMAGE_ORIGIN <= reset_address < IMAGE_ORIGIN + len(image)):
        raise ValueError(
            f"{bin_path}: reset handler 0x{reset_address:08X} is outside the image"
        )

    ihex = parse_ihex(hex_path)
    first_address = min(ihex)
    last_address = max(ihex)

    if first_address != IMAGE_ORIGIN:
        raise ValueError(
            f"{hex_path}: first data address 0x{first_address:08X}, "
            f"expected 0x{IMAGE_ORIGIN:08X}"
        )

    if last_address >= IMAGE_LIMIT:
        raise ValueError(
            f"{hex_path}: data ends at 0x{last_address:08X}, "
            f"outside bring-up region ending before 0x{IMAGE_LIMIT:08X}"
        )

    for address, value in ihex.items():
        offset = address - IMAGE_ORIGIN
        if offset < 0 or offset >= len(image):
            raise ValueError(
                f"{hex_path}: data at 0x{address:08X} has no BIN counterpart"
            )
        if image[offset] != value:
            raise ValueError(
                f"{hex_path}: byte mismatch at 0x{address:08X}: "
                f"HEX=0x{value:02X}, BIN=0x{image[offset]:02X}"
            )

    print(
        "ST17H66B image verified: "
        f"origin=0x{IMAGE_ORIGIN:08X}, size=0x{len(image):X}, "
        f"sp=0x{stack_pointer:08X}, reset=0x{reset_vector:08X}"
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bin", required=True, type=Path)
    parser.add_argument("--hex", required=True, type=Path)
    args = parser.parse_args()
    verify(args.bin, args.hex)


if __name__ == "__main__":
    main()
