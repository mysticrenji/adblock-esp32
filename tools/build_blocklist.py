#!/usr/bin/env python3
"""Build a sorted five-byte FNV-1a hash table for the ESP32 firmware.

The input is deliberately local-only. Review/download lists yourself, then pass
their paths to this command. Supported lines are domains and hosts-file rows.
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

MASK40 = (1 << 40) - 1
DOMAIN = re.compile(r"^(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\.)+[a-z]{2,63}$", re.I)


def normalise(value: str) -> str | None:
    value = value.strip().lower().rstrip(".")
    if value.startswith("||") and value.endswith("^"):
        value = value[2:-1]
    if value.startswith(("@@", "#", "!")) or "/" in value or "$" in value or "*" in value:
        return None
    return value if DOMAIN.fullmatch(value) else None


def domains_from(path: Path):
    for raw in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        fields = line.split()
        candidates = fields[1:] if fields and fields[0] in {"0.0.0.0", "127.0.0.1", "::"} else fields[:1]
        for candidate in candidates:
            domain = normalise(candidate)
            if domain:
                yield domain


def fnv1a40(domain: str) -> int:
    value = 2166136261
    for byte in domain.encode("ascii"):
        value = (value ^ byte) * 16777619 & 0xFFFFFFFFFFFFFFFF
    return value & MASK40


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("inputs", type=Path, nargs="+", help="reviewed local hosts/domain list files")
    args = parser.parse_args()
    hashes = set()
    count = 0
    for source in args.inputs:
        if not source.is_file():
            parser.error(f"input does not exist or is not a file: {source}")
        for domain in domains_from(source):
            hashes.add(fnv1a40(domain))
            count += 1
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("wb") as handle:
        for value in sorted(hashes):
            handle.write(value.to_bytes(5, "big"))
    print(f"Wrote {len(hashes):,} unique hashes ({args.output.stat().st_size:,} bytes) from {count:,} domains to {args.output}")


if __name__ == "__main__":
    main()
