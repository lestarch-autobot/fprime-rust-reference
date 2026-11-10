#!/usr/bin/env python3
"""bench/native_recorder.py — the bench's event/channel recorder for fth (native Linux adapter, bench.json `recorder`).

Runs the GDS recorder `fprime-cli events` (its untouched output stays under {logs}/recorder-native/) and re-stamps
every row for fth's exporter, which reads only the F Prime TB_PROC_TIME row form::

    TB_PROC_TIME: <sec>.<usec>s, context=<ctx>,(<tb>(<ctx>)-<sec>:<usec>),<name>,<id>,<rest>

The deployment's Svc.PosixTime stamps TB_WORKSTATION_TIME, which fprime-gds prints as an ISO date::

    2026-11-10T09:53:30.262342,(2(0)-1794304410:262342),<name>,<id>,<rest>

The rewrite is a pure re-rendering of the row's own time field (seconds and microseconds of the (tb(ctx)-sec:usec)
stamp; nothing is added, dropped, reordered or deduplicated); rows that do not have that shape are copied verbatim.
Output: {logs}/fprime-cli-<timestamp>/event.log and channel.log (the layout fth's export and wait_event read).
"""
import argparse
import datetime
import re
import signal
import subprocess
import sys
import time
from pathlib import Path

ROW = re.compile(r"^(?P<iso>\d{4}-\d{2}-\d{2}T[0-9:.]+),\((?P<tb>\d+)\((?P<ctx>\d+)\)-(?P<sec>\d+):(?P<usec>\d+)\),(?P<rest>.*)$")


def restamp(line: str) -> str:
    m = ROW.match(line.rstrip("\n"))
    if not m:
        return line
    sec, usec, ctx, tb = m["sec"], int(m["usec"]), m["ctx"], m["tb"]
    return f"TB_PROC_TIME: {sec}.{usec:06d}s, context={ctx},({tb}({ctx})-{sec}:{usec}),{m['rest']}\n"


class Tail:
    def __init__(self, src: Path, dst: Path):
        self.src, self.dst, self.pos, self.buf = src, dst, 0, ""

    def pump(self) -> None:
        if not self.src.is_file():
            return
        with self.src.open("r", encoding="utf-8", errors="replace") as fh:
            fh.seek(self.pos)
            chunk = fh.read()
            self.pos = fh.tell()
        if not chunk:
            return
        self.buf += chunk
        lines = self.buf.split("\n")
        self.buf = lines.pop()  # incomplete tail, if any
        with self.dst.open("a", encoding="utf-8") as out:
            for ln in lines:
                out.write(restamp(ln + "\n"))


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--dictionary", required=True)
    parser.add_argument("--zmq-transport", required=True, nargs=2, metavar=("IN", "OUT"))
    parser.add_argument("--logs", required=True, type=Path)
    args = parser.parse_args(argv)
    native = args.logs / "recorder-native"
    native.mkdir(parents=True, exist_ok=True)
    out = args.logs / ("fprime-cli-" + datetime.datetime.now().strftime("%Y_%m_%d-%H_%M_%S"))
    out.mkdir(parents=True, exist_ok=True)
    child = subprocess.Popen(["fprime-cli", "events", "--dictionary", args.dictionary, "--zmq-transport", *args.zmq_transport,
                              "--logs", str(native)], stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    stop = {"flag": False}

    def on_signal(signum, frame):
        stop["flag"] = True

    signal.signal(signal.SIGTERM, on_signal)
    signal.signal(signal.SIGINT, on_signal)
    tails: dict[str, Tail] = {}
    while True:
        for d in sorted(p for p in native.glob("fprime-cli-*") if p.is_dir()):
            for name in ("event.log", "channel.log"):
                tails.setdefault(f"{d.name}/{name}", Tail(d / name, out / name))
        for t in tails.values():
            t.pump()
        if stop["flag"] or child.poll() is not None:
            break
        time.sleep(0.05)
    if child.poll() is None:
        child.terminate()
        try:
            child.wait(10)
        except subprocess.TimeoutExpired:
            child.kill()
    time.sleep(0.2)
    for t in tails.values():
        t.pump()
    return 0 if stop["flag"] else (child.returncode or 1)


if __name__ == "__main__":
    sys.exit(main())
