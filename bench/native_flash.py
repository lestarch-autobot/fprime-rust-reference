#!/usr/bin/env python3
"""Native "flash" for the bench (R7): installs the delivered executable where the GDS launches it from

Empties the install directory, copies the image to --install (no suffix), marks it executable and prints the bench
serial so the harness can match `Serial# = (\\S+)`.
"""
import argparse
import shutil
import socket
import stat
import sys
from pathlib import Path


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--image", required=True, type=Path, help="delivered deployment executable")
    parser.add_argument("--install", required=True, type=Path, help="install path of the executable (its parent is emptied)")
    args = parser.parse_args(argv)
    if not args.image.is_file():
        print(f"[ERROR] image not found: {args.image}", file=sys.stderr)
        return 1
    install_dir = args.install.parent
    if install_dir.exists():
        shutil.rmtree(install_dir)
    install_dir.mkdir(parents=True)
    shutil.copyfile(args.image, args.install)
    args.install.chmod(args.install.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    print(f"Serial# = native-{socket.gethostname()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
