"""Planted faults and helper checks for RUST-REF (FprimeRustReference on the native host).

Loaded by ``fth`` through ``plan.json: helpers_module``. Each fault receives ``(test_id, command_name, args)``
and returns rewritten ``args`` (``kind="rewrite"``) or a list of ``(command_name, args)`` to send instead
(``kind="insert"``). ``fails`` names, per test and verbatim, every check of the plan the fault breaks.
The fault menu is generated FROM these decorators (``fth planted-faults-menu plan.json``).
Design-lane declarations: the stimulus (``test_rust_reference.py``) is written in DEVELOPMENT.

Helpers: ``delta_in_range``, ``version_payloads_present``, ``prmdb_record_equals`` are check declarations (design lane).
Insulation: every functional test opens with ``rustCounter.RESET`` or an explicit ``rustMonitor.SET_LIMIT``
and restores THRESHOLD/limit before its closing marker, so a fault's residue (a wrong counter, a wrong limit)
does not reach the next test; ``ripples_to`` is therefore empty for every fault below and the reason is in
each ``mechanism``.
"""
from pathlib import Path
from fth.registry import helper, planted_fault

U32_MAX = 4294967295


def _is(command_name, short):
    return command_name == short or command_name.endswith("." + short)


@helper("delta_in_range")
def delta_in_range(ctx, channel, lo, hi):
    """last - first sample of ``channel`` in the window lies in ``lo..hi`` (rate-group counters cannot be exact)."""
    rows = ctx.window.channel(channel)
    if len(rows) < 2:
        return False, f"fewer than two samples of {channel} in {ctx.window.label}", [], True
    a, b = rows[0], rows[-1]
    delta = b.value - a.value
    return lo <= delta <= hi, f"delta={delta} (first={a.value} {a.line}; last={b.value} {b.line})", [
        f"channels.jsonl:{a.idx}-{a.idx}", f"channels.jsonl:{b.idx}-{b.idx}"]

@helper("counter_advances")
def counter_advances(ctx, channel, by):
    """Advance of a change-driven counter channel over the window. Svc.TlmChan sends a channel only when it was
    written, and relay.Relayed / rustMonitor.Exceedances are written only when they move, so fth's
    ``delta(ch)`` (last - first sample inside the window) misses the step that produced the window's first
    sample. Baseline = the last sample before the START marker anywhere in the record, else the boot value 0
    (Relay::m_relayed(0), RustMonitor exceedances: 0). No sample in the window = never written = advance 0."""
    w = ctx.window
    if w.start_marker is None:
        return False, f"{w.label} has no START marker", [], True
    t0 = w.start_marker.t
    prior = [c for c in ctx.record.channels if c.short == channel and c.t < t0]
    base = prior[-1].value if prior else 0
    base_ref = f"channels.jsonl:{prior[-1].idx}-{prior[-1].idx}" if prior else "boot value 0, never sent before the window"
    rows = w.channel(channel)
    if not rows:
        return by == 0, f"no {channel} sample in {w.label}: never written (baseline={base}, {base_ref})", (
            [f"channels.jsonl:{prior[-1].idx}-{prior[-1].idx}"] if prior else [])
    last = rows[-1]
    advance = last.value - base
    return advance == by, f"advance={advance} (baseline={base}, {base_ref}; last={last.value} {last.line})", [
        f"channels.jsonl:{last.idx}-{last.idx}"
    ]



@helper("version_payloads_present")
def version_payloads_present(ctx, events):
    """Every named Svc.Version event in the window carries a non-empty ``version`` payload that is not UNKNOWN."""
    seen, bad, refs = [], [], []
    for name in events:
        rows = ctx.window.events(name)
        if not rows:
            return False, f"no {name} event in {ctx.window.label}", [], True
        for e in rows:
            v = (e.args or {}).get("version", "")
            seen.append(f"{name}=[{v}]")
            refs.append(f"events.jsonl:{e.idx}-{e.idx}")
            if not v or v.upper() == "UNKNOWN":
                bad.append(name)
    return not bad, "; ".join(seen), refs

def _raw_file(ctx, rel):
    """``record/raw/<rel>`` of the record under judgment, or None. The replayed record's own ``raw/`` first; at
    window close the live plugin judges an export of ``<run>/record/raw`` made in a temporary directory, so the
    run dir / replayed record dir are taken from the pytest argv fth recorded (``--fth-run-dir``,
    ``--fth-replay-record``; this module is the plan's helpers module, not stimulus)."""
    import sys
    roots = [Path(ctx.record.path) / "raw"]
    argv = sys.argv
    for k, sub in (("--fth-replay-record", ("raw",)), ("--fth-run-dir", ("record", "raw"))):
        for i, a in enumerate(argv):
            if a.startswith(k + "="):
                roots.append(Path(a.split("=", 1)[1]).joinpath(*sub))
            elif a == k and i + 1 < len(argv):
                roots.append(Path(argv[i + 1]).joinpath(*sub))
    for r in roots:
        if (r / rel).is_file():
            return r / rel
    return None


@helper("prmdb_record_equals")
def prmdb_record_equals(ctx, path, id, value):
    """The downlinked PrmDb.dat (record/raw/<path>) holds parameter ``id`` with the U32 ``value``.

    File format (Svc/PrmDb/docs/sdd.md §3.2, PrmDbImpl.cpp): U32 CRC32 over the bytes that follow it, then
    records of U8 delimiter 0xA5, U32 record size, FwPrmIdType (U32) id, serialized value. On a complete
    window an absent file is FAIL (no file was downlinked); FAIL on a CRC, delimiter, missing-id or value
    mismatch. UNVERIFIED is never returned here: a torn window is already UNVERIFIED by the completeness checks.
    """
    import struct
    import zlib
    f = _raw_file(ctx, path)
    if f is None:
        return False, f"{path} absent from record/raw — no file was downlinked", [], False
    data = f.read_bytes()
    if len(data) < 4:
        return False, f"{path} shorter than a CRC ({len(data)} bytes)", [f"{path}:0-{len(data)}"]
    # Svc.PrmDb stores ~Utils::Hash(Crc32).finalize() of the record bytes (PrmDbImpl.cpp PrmFileSave): CRC-32
    # poly 0xEDB88320, init 0xFFFFFFFF, no final inversion = zlib.crc32 ^ 0xFFFFFFFF (bench-verified A9)
    crc_stored, crc_computed = struct.unpack(">I", data[:4])[0], (zlib.crc32(data[4:]) ^ 0xFFFFFFFF) & 0xFFFFFFFF
    if crc_stored != crc_computed:
        return False, f"CRC32 mismatch: stored 0x{crc_stored:08x}, computed 0x{crc_computed:08x}", [f"{path}:0-4"]
    off, found = 4, None
    while off + 9 <= len(data):
        if data[off] != 0xA5:
            return False, f"bad delimiter 0x{data[off]:02x} at offset {off}", [f"{path}:{off}-{off+1}"]
        size = struct.unpack(">I", data[off + 1:off + 5])[0]
        pid = struct.unpack(">I", data[off + 5:off + 9])[0]
        body = data[off + 9:off + 5 + size]
        if pid == id:
            found = struct.unpack(">I", body[:4])[0] if len(body) >= 4 else None
        off += 5 + size
    if found is None:
        return False, f"id 0x{id:08x} not in {path} ({off} bytes scanned)", [f"{path}:0-{len(data)}"]
    return found == value, f"id 0x{id:08x} = {found} (expected {value}); CRC32 ok", [f"{path}:0-{len(data)}"]


@planted_fault("bump_wrong_amount", applies_to=["L2-1", "L2-3"], cls="harness-expectation",
      mechanism="harness sends BUMP(amount+1) while the plan expects the planned amounts: every Bumped payload, the final Counter and the relayed LastCount move by one; the next test opens with RESET, so nothing ripples",
      fails={
        "L2-1": ["event rustCounter.Bumped(value=7) count == 1", "event rustCounter.Bumped(value=12) count == 1",
                 "event rustCounter.ResetIssued precedes event rustCounter.Bumped(value=7)", "last(rustCounter.Counter) == 12"],
        "L2-3": ["event rustCounter.Bumped(value=3) count == 1", "last(rustMonitor.LastCount) == 3"],
      })
def bump_wrong_amount(test_id, command_name, args):
    if test_id in ("L2-1", "L2-3") and _is(command_name, "rustCounter.BUMP"):
        return [(args[0] + 1) & U32_MAX]
    return args


@planted_fault("skip_reset", applies_to=["L2-1", "L7-1"], cls="stimulus",
      mechanism="the planned RESET is replaced by CdhCore.cmdDisp.CMD_NO_OP: no ResetIssued, no count travels the port chain, and in L7-1 the counter stays at 3 (left by L5-1) so Counter is not 0 and no count 0 reaches LastCount (its last sample stays 10 from L2-5); L2-2 opens with its own RESET, so nothing ripples",
      fails={
        "L2-1": ["event rustCounter.ResetIssued count == 1", "event rustCounter.ResetIssued precedes event rustCounter.Bumped(value=7)"],
        "L7-1": ["event rustCounter.ResetIssued count == 1", "last(rustCounter.Counter) == 0", "last(rustMonitor.LastCount) == 0", "relay.Relayed advances by 1"],
      }, kind="insert")
def skip_reset(test_id, command_name, args):
    if test_id in ("L2-1", "L7-1") and _is(command_name, "rustCounter.RESET"):
        return [("CdhCore.cmdDisp.CMD_NO_OP", [])]
    return [(command_name, args)]


@planted_fault("threshold_not_set", applies_to=["L2-2"], cls="stimulus",
      mechanism="the planned THRESHOLD_PRM_SET(5) is sent as THRESHOLD_PRM_SET(100): the UUT keeps the default threshold and BUMP(9) does not saturate; the closing THRESHOLD_PRM_SET(100) restores the state L5-1 expects",
      fails={
        "L2-2": ["event rustCounter.Saturated(value=5, threshold=5) count == 1", "last(rustCounter.Counter) == 5", "event rustCounter.Bumped count == 0"],
      })
def threshold_not_set(test_id, command_name, args):
    if test_id == "L2-2" and _is(command_name, "rustCounter.THRESHOLD_PRM_SET") and args[0] == 5:
        return [100]
    return args


@planted_fault("bump_zero", applies_to=["L5-1"], cls="stimulus",
      mechanism="both planned BUMP(2) are sent as BUMP(0): the counter stays 0, so Bumped(2), Saturated(3, 3) and the precedes check all fail; L6-1 does not read the counter and L7-1 resets it",
      fails={
        "L5-1": ["event rustCounter.Bumped(value=2) count == 1", "event rustCounter.Saturated(value=3, threshold=3) count == 1", "last(rustCounter.Counter) == 3",
                 "event rustCounter.Bumped(value=2) precedes event rustCounter.Saturated(value=3, threshold=3)"],
      })
def bump_zero(test_id, command_name, args):
    if test_id == "L5-1" and _is(command_name, "rustCounter.BUMP"):
        return [0]
    return args


@planted_fault("limit_not_set", applies_to=["L2-5"], cls="stimulus",
      mechanism="the planned SET_LIMIT(4) is sent as SET_LIMIT(4000): the UUT limit is never exceeded by the count 10; the closing SET_LIMIT(4294967295) restores the limit L6-1 expects",
      fails={
        "L2-5": ["event rustMonitor.LimitSet(limit=4) count == 1", "event rustMonitor.LimitExceeded(count=10, limit=4) count == 1", "rustMonitor.Exceedances advances by 1"],
      })
def limit_not_set(test_id, command_name, args):
    if test_id == "L2-5" and _is(command_name, "rustMonitor.SET_LIMIT") and args[0] == 4:
        return [4000]
    return args


@planted_fault("negative_wrong_limit", applies_to=["L6-1"], cls="stimulus",
      mechanism="the negative control sends SET_LIMIT(1) instead of SET_LIMIT(0): the command is accepted (LimitSet, Limit 1), so the rejection never happens; the limit stays 1 into L7-1, where the only count is 0 and does not exceed it, so nothing ripples",
      fails={
        "L6-1": ["event rustMonitor.InvalidLimit(limit=0) count == 1", "event cmdDisp.OpCodeError(error=VALIDATION_ERROR) count == 1", "event rustMonitor.LimitSet count == 0",
                 "delta(rustMonitor.Limit) == 0", "last(rustMonitor.Limit) == 4294967295"],
      })
def negative_wrong_limit(test_id, command_name, args):
    if test_id == "L6-1" and _is(command_name, "rustMonitor.SET_LIMIT"):
        return [1]
    return args


@planted_fault("threshold_not_saved", applies_to=["L7-1"], cls="uut-state",
      mechanism="the planned FileHandling.prmDb.PRM_SAVE_FILE is replaced by CdhCore.cmdDisp.CMD_NO_OP: the run starts in a fresh run/FprimeRustReference/ with no PrmDb.dat, so the file is never written, PrmFileSaveComplete is absent, SendFile fails with FileOpenError and completes EXECUTION_ERROR (one OpCodeError), no FileSent occurs, no record/raw/files/fprime-downlink/PrmDb.dat exists and prmdb_record_equals reports FAIL; this is the fault that disturbs the final-state artifact",
      fails={
        "L7-1": ["event prmDb.PrmFileSaveComplete(records=1) count == 1", "event fileDownlink.FileSent count == 1", "PrmDb.dat record 0x10016000 == 42", "event cmdDisp.OpCodeError count == 0"],
      }, kind="insert")
def threshold_not_saved(test_id, command_name, args):
    if test_id == "L7-1" and _is(command_name, "FileHandling.prmDb.PRM_SAVE_FILE"):
        return [("CdhCore.cmdDisp.CMD_NO_OP", [])]
    return [(command_name, args)]
