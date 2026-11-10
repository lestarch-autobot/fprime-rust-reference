#!/usr/bin/env python3
"""bench/native_send.py — the bench's command sender for fth (native Linux adapter, bench.json `send`).

Same command line as ``fprime-cli command-send``; runs it unchanged with its log redirected to
``{logs}/send-native/`` (kept verbatim), then copies its ``command.log`` row into ``{logs}/fprime-cli-<ts>/command.log``
(the layout fth's export reads) — the command given in fth's short form is resolved to the one dictionary command
ending in it — with the argument list re-rendered the way it was given on the command line:
an argument whose text is a decimal integer is logged as that integer, anything else as the string it is::

    ...,CdhCore.cmdDisp.CMD_NO_OP_STRING,16777217,['20']   ->   ...,CdhCore.cmdDisp.CMD_NO_OP_STRING,16777217,[20]

Why: the plan's window marker is CMD_NO_OP_STRING (its one argument is a string); fth 0.1.2 slices a window's
commands by comparing the marker row's first logged argument with the marker id as an integer, so a row logged with
the string form matches nothing and every window reads ``commands_sent=0``. Nothing else in the row changes.
"""
import ast
import subprocess
import sys
import time
from pathlib import Path


def rerender(raw: str) -> str:
    try:
        v = ast.literal_eval(raw)
    except (ValueError, SyntaxError):
        return raw
    if not isinstance(v, (list, tuple)):
        return raw
    out = []
    for a in v:
        if isinstance(a, str) and a.strip().lstrip("-").isdigit():
            out.append(int(a))
        else:
            out.append(a)
    return repr(out)


def resolve_command(argv: list[str]) -> list[str]:
    """Resolve an fth short command name (``cmdDisp.CMD_NO_OP_STRING``) to the one dictionary command ending in it;
    full names (``CdhCore.cmdDisp.CMD_NO_OP_STRING``) pass through unchanged (fth hands windows' marker in short form)."""
    import json
    if "--dictionary" not in argv:
        return argv
    dictionary = Path(argv[argv.index("--dictionary") + 1])
    names = [c["name"] for c in json.loads(dictionary.read_text()).get("commands", [])]
    skip = {"--dictionary": 1, "--logs": 1, "--zmq-transport": 2}
    k = 0
    while k < len(argv):
        a = argv[k]
        if a in skip:
            k += 1 + skip[a]
            continue
        if a.startswith("-"):
            break
        if a not in names:
            full = [n for n in names if n.endswith("." + a)]
            if len(full) == 1:
                argv[k] = full[0]
        break
    return argv


def main(argv: list[str]) -> int:
    if "--logs" not in argv:
        sys.stderr.write("native_send.py: --logs is required\n")
        return 2
    i = argv.index("--logs")
    logs = Path(argv[i + 1])
    native = logs / "send-native"
    native.mkdir(parents=True, exist_ok=True)
    before = {p for p in native.glob("fprime-cli-*/command.log")}
    argv = list(argv)
    argv[i + 1] = str(native)
    argv = resolve_command(argv)
    rc = subprocess.run(["fprime-cli", "command-send", *argv]).returncode
    time.sleep(0.1)
    for log in sorted(set(native.glob("fprime-cli-*/command.log")) - before) or sorted(native.glob("fprime-cli-*/command.log"))[-1:]:
        dst = logs / log.parent.name
        dst.mkdir(parents=True, exist_ok=True)
        done = (dst / "command.log").read_text().splitlines() if (dst / "command.log").exists() else []
        with (dst / "command.log").open("a") as out:
            for line in log.read_text().splitlines()[len(done):]:
                head, sep, raw = line.partition(",[") if ",[" in line else (line, "", "")
                out.write((head + "," + rerender("[" + raw) if sep else line) + "\n")
    return rc


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
