#!/usr/bin/env python3
"""Native "dump" for the bench (R7): reads the installed executable back so its hash can be compared to the image"""
import argparse
import shutil
import sys
from pathlib import Path


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", required=True, type=Path, help="installed executable written by native_flash.py")
    parser.add_argument("--readback", required=True, type=Path, help="where to write the read-back copy")
    args = parser.parse_args(argv)
    if not args.install.is_file():
        print(f"[ERROR] nothing installed at {args.install}", file=sys.stderr)
        return 1
    args.readback.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(args.install, args.readback)
    return 0


if __name__ == "__main__":
    sys.exit(main())
