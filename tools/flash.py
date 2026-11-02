#!/usr/bin/env python3
"""Clean, build and upload firmware plus the LittleFS blocklist."""
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parent.parent


def run(command: list[str]) -> None:
    print("+ " + " ".join(command), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("lists", type=Path, nargs="*", help="local domain/hosts text files")
    parser.add_argument("--blocklist", type=Path, help="existing compiled binary, e.g. domain.bin")
    parser.add_argument("--port", help="serial port; otherwise PlatformIO auto-detects it")
    parser.add_argument("--build-only", action="store_true", help="build both images without uploading")
    args = parser.parse_args()
    if args.lists and args.blocklist:
        parser.error("use domain text files or --blocklist, not both")
    if not (ROOT / "src/config.h").is_file():
        parser.error("create src/config.h from src/config.example.h first")

    pio = shutil.which("pio") or shutil.which("platformio")
    if not pio:
        candidate = Path.home() / ".platformio/penv" / ("Scripts/platformio.exe" if sys.platform == "win32" else "bin/pio")
        if candidate.is_file():
            pio = str(candidate)
    if not pio:
        parser.error("PlatformIO not found; run from the PlatformIO Core CLI terminal")

    output = ROOT / "data/blocklist.bin"
    if args.lists:
        sources = [source.resolve() for source in args.lists]
        for source in sources:
            if not source.is_file():
                parser.error(f"domain list not found: {source}")
            if source == output:
                parser.error("data/blocklist.bin is a binary output, not a text input")
        run([sys.executable, str(ROOT / "tools/build_blocklist.py"), str(output), *map(str, sources)])

    source = args.blocklist.resolve() if args.blocklist else output
    if not source.is_file():
        parser.error("supply domain text files or --blocklist domain.bin, or generate data/blocklist.bin first")
    size = source.stat().st_size
    if size == 0 or size % 5:
        parser.error("blocklist must contain a non-empty table of five-byte hashes")
    # Firmware performs binary search, so hashes must be sorted and unique.
    with source.open("rb") as handle:
        previous = None
        while value := handle.read(5):
            if previous is not None and value <= previous:
                parser.error("blocklist hashes must be sorted and unique; regenerate from a domain text list")
            previous = value
    if source != output:
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, output)

    command = [pio, "run", "-e", "t-display-s3"]
    run([*command, "-t", "clean"])
    run(command)
    run([*command, "-t", "buildfs"])
    if not args.build_only:
        port = ["--upload-port", args.port] if args.port else []
        run([*command, "-t", "upload", *port])
        run([*command, "-t", "uploadfs", *port])
    print("Firmware and filesystem " + ("built." if args.build_only else "uploaded."))


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
    except OSError as error:
        print(f"Error: {error}", file=sys.stderr)
        sys.exit(1)
