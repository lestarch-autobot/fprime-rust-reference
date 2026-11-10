"""Planted faults and helper checks for RUST-REF (FprimeRustReference on the native host).

Loaded by ``fth`` through ``plan.json: helpers_module``. Each fault receives ``(test_id, command_name, args)``
and returns rewritten ``args`` (``kind="rewrite"``) or a list of ``(command_name, args)`` to send instead
(``kind="insert"``). ``fails`` names, per test and verbatim, every check of the plan the fault breaks.
The fault menu is generated FROM these decorators (``fth planted-faults-menu plan.json``).
Design-lane declarations: the stimulus (``test_rust_reference.py``) is written in DEVELOPMENT.

Insulation: every functional test opens with ``rustCounter.RESET`` or an explicit ``rustMonitor.SET_LIMIT``
and restores THRESHOLD/limit before its closing marker, so a fault's residue (a wrong counter, a wrong limit)
does not reach the next test; ``ripples_to`` is therefore empty for every fault below and the reason is in
each ``mechanism``.
"""
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
        "L7-1": ["event rustCounter.ResetIssued count == 1", "last(rustCounter.Counter) == 0", "last(rustMonitor.LastCount) == 0", "delta(relay.Relayed) == 1"],
      }, kind="insert")
def skip_reset(test_id, command_name, args):
    if test_id in ("L2-1", "L7-1") and _is(command_name, "rustCounter.RESET"):
        return [("CdhCore.cmdDisp.CMD_NO_OP", [])]
    return [(command_name, args)]


@planted_fault("threshold_not_set", applies_to=["L2-2"], cls="uut-state",
      mechanism="the planned THRESHOLD_PRM_SET(5) is sent as THRESHOLD_PRM_SET(100): the UUT keeps the default threshold and BUMP(9) does not saturate; the closing THRESHOLD_PRM_SET(100) restores the state L5-1 expects",
      fails={
        "L2-2": ["event rustCounter.Saturated(value=5, threshold=5) count == 1", "last(rustCounter.Counter) == 5", "event rustCounter.Bumped count == 0"],
      })
def threshold_not_set(test_id, command_name, args):
    if test_id == "L2-2" and _is(command_name, "rustCounter.THRESHOLD_PRM_SET") and args[0] == 5:
        return [100]
    return args


@planted_fault("bump_zero", applies_to=["L5-1"], cls="uut-state",
      mechanism="both planned BUMP(2) are sent as BUMP(0): the counter stays 0, so Bumped(2), Saturated(3, 3) and the precedes check all fail; L6-1 does not read the counter and L7-1 resets it",
      fails={
        "L5-1": ["event rustCounter.Bumped(value=2) count == 1", "event rustCounter.Saturated(value=3, threshold=3) count == 1", "last(rustCounter.Counter) == 3",
                 "event rustCounter.Bumped(value=2) precedes event rustCounter.Saturated(value=3, threshold=3)"],
      })
def bump_zero(test_id, command_name, args):
    if test_id == "L5-1" and _is(command_name, "rustCounter.BUMP"):
        return [0]
    return args


@planted_fault("limit_not_set", applies_to=["L2-5"], cls="uut-state",
      mechanism="the planned SET_LIMIT(4) is sent as SET_LIMIT(4000): the UUT limit is never exceeded by the count 10; the closing SET_LIMIT(4294967295) restores the limit L6-1 expects",
      fails={
        "L2-5": ["event rustMonitor.LimitSet(limit=4) count == 1", "event rustMonitor.LimitExceeded(count=10, limit=4) count == 1", "delta(rustMonitor.Exceedances) == 1"],
      })
def limit_not_set(test_id, command_name, args):
    if test_id == "L2-5" and _is(command_name, "rustMonitor.SET_LIMIT") and args[0] == 4:
        return [4000]
    return args


@planted_fault("negative_wrong_limit", applies_to=["L6-1"], cls="stimulus",
      mechanism="the negative control sends SET_LIMIT(1) instead of SET_LIMIT(0): the command is accepted (LimitSet, Limit 1), so the rejection never happens; the limit stays 1 into L7-1, where the only count is 0 and does not exceed it, so nothing ripples",
      fails={
        "L6-1": ["event rustMonitor.InvalidLimit(limit=0) count == 1", "event CdhCore.cmdDisp.OpCodeError(error=VALIDATION_ERROR) count == 1", "event rustMonitor.LimitSet count == 0",
                 "delta(rustMonitor.Limit) == 0", "last(rustMonitor.Limit) == 4294967295"],
      })
def negative_wrong_limit(test_id, command_name, args):
    if test_id == "L6-1" and _is(command_name, "rustMonitor.SET_LIMIT"):
        return [1]
    return args


@planted_fault("threshold_not_saved", applies_to=["L7-1"], cls="uut-state",
      mechanism="the planned FileHandling.prmDb.PRM_SAVE_FILE is replaced by CdhCore.cmdDisp.CMD_NO_OP: the parameter file on the UUT is never written, so PrmFileSaveComplete is absent, the downlinked PrmDb.dat (if any earlier file exists) does not hold THRESHOLD 42, and the auditor's final-state parse fails; this is the fault that disturbs the final-state artifact",
      fails={
        "L7-1": ["event FileHandling.prmDb.PrmFileSaveComplete(records=1) count == 1", "event FileHandling.fileDownlink.FileSent count == 1"],
      }, kind="insert")
def threshold_not_saved(test_id, command_name, args):
    if test_id == "L7-1" and _is(command_name, "FileHandling.prmDb.PRM_SAVE_FILE"):
        return [("CdhCore.cmdDisp.CMD_NO_OP", [])]
    return [(command_name, args)]
