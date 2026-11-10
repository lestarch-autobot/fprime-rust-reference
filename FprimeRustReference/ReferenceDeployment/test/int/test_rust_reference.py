"""test/int/test_rust_reference.py — stimulus + named checks for the FprimeRustReference system-test plan.

One pytest per plan test, in plan order. Each test sends its stimulus inside ``with window(start, end):``
and declares the test's named checks there: ``check("<plan predicate>")`` (plan grammar) or
``check("<text>", fn)`` for a registered ``@helper`` of ``planted_faults.py`` (the same helper text the
plan's ``metrics`` carry, so the plan and the stimulus judge the same name). ``require(STD)`` holds the
infrastructure preconditions (markers, every command dispatched, >= 2 RustReference samples): not holding
makes the test UNVERIFIED; a missing UUT event or sample is a FAIL. The checks are judged over the
record when the window closes (the auditor replays the same functions); waits are pacing only.

Commands are sent by their full dictionary name (``fprime-cli`` refuses suffix names such as
``rustCounter.RESET``); the record's short names (``rustCounter.Bumped``, ``cmdDisp.OpCodeError``) are
what the checks name. ``send`` hands every non-marker command to the planted fault drawn for *this* test.
"""
import pytest

# ---- dictionary names (FprimeRustReferenceTopologyDictionary.json) -----------------------------
MARK = "CdhCore.cmdDisp.CMD_NO_OP_STRING"            # plan markers.command; never handed to a planted fault
VERSION = "CdhCore.version.VERSION"
RESET = "FprimeRustReference.rustCounter.RESET"
BUMP = "FprimeRustReference.rustCounter.BUMP"
THRESHOLD_SET = "FprimeRustReference.rustCounter.THRESHOLD_PRM_SET"
THRESHOLD_SAVE = "FprimeRustReference.rustCounter.THRESHOLD_PRM_SAVE"
SET_LIMIT = "FprimeRustReference.rustMonitor.SET_LIMIT"
PRM_SAVE_FILE = "FileHandling.prmDb.PRM_SAVE_FILE"
SEND_FILE = "FileHandling.fileDownlink.SendFile"

SAMPLE = "rustCounter.TickCount"   # one row per 1 Hz RustReference packet: the plan's "bracket (n samples)"
STD = "STD"                        # plan.json std_complete: both markers, every command dispatched, >= 2 samples
ABS = "ABS"                        # plan.json abs.ABS: no MalformedCommand / InvalidCommand / TooManyCommands
NO_OPCODE_ERROR = "event cmdDisp.OpCodeError count == 0"
U32_MAX = 4294967295

VERSION_EVENTS = ["version.ProjectVersion", "version.FrameworkVersion", "version.LibraryVersions"]


def bracket(bench, n=2):
    bench.wait_samples(SAMPLE, n)


def plan_helper(fth_plan, name, **args):
    """The registered ``@helper`` ``name`` of the plan's helpers module, bound to its plan args."""
    fn = fth_plan.registry.helpers[name]
    return lambda h: fn(h, **args)


# ---- L0: harness self-test -------------------------------------------------------------

def test_L0_1(window, send, bench, check, require, fth_plan):
    with window(10, 11):
        require(STD)
        require("commands_sent == 3")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 2")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        send(VERSION, ["ALL"])
        bench.wait_event("version.LibraryVersions")
        bracket(bench)
        check(ABS)
        check("event version.ProjectVersion count == 1")
        check("event version.FrameworkVersion count == 1")
        check("event version.LibraryVersions count == 1")
        check("version payloads of ProjectVersion, FrameworkVersion, LibraryVersions are non-empty and not UNKNOWN",
              plan_helper(fth_plan, "version_payloads_present", events=VERSION_EVENTS))
        check(NO_OPCODE_ERROR)


def test_L0_2(window, send, bench, check, require):
    with window(20, 21):
        require(STD)
        require("commands_sent == 3")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 2")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        send(MARK, [42])
        bench.wait_event("cmdDisp.NoOpStringReceived")
        bracket(bench)
        check(ABS)
        check("event cmdDisp.NoOpStringReceived count == 3")
        check(NO_OPCODE_ERROR)


# ---- L2: command, parameter, port chain, rate group, queued component -----------------------

@pytest.mark.req("REQ-FprimeRust-008", "REQ-FprimeRust-012", "REQ-FprimeRust-013", "REQ-FprimeRust-014",
                 "REQ-RustCounter-001", "REQ-RustCounter-002", "REQ-RustCounter-004")
def test_L2_1(window, send, bench, check, require):
    with window(30, 31):
        require(STD)
        require("commands_sent == 5")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 4")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        send(RESET, [])
        bench.wait_event("rustCounter.ResetIssued")
        send(BUMP, [7])                                   # bump_wrong_amount / skip_reset act here if L2-1 was drawn
        bench.wait_event("rustCounter.Bumped")
        send(BUMP, [5])
        bench.wait_event("rustCounter.Bumped")
        bracket(bench)
        check(ABS)
        check("event rustCounter.ResetIssued count == 1")
        check("event rustCounter.Bumped(value=7) count == 1")
        check("event rustCounter.Bumped(value=12) count == 1")
        check("event rustCounter.ResetIssued precedes event rustCounter.Bumped(value=7)")
        check("last(rustCounter.Counter) == 12")
        check("event rustCounter.Saturated count == 0")
        check(NO_OPCODE_ERROR)


@pytest.mark.req("REQ-FprimeRust-015", "REQ-RustCounter-003", "REQ-RustCounter-006")
def test_L2_2(window, send, bench, check, require):
    with window(40, 41):
        require(STD)
        require("commands_sent == 6")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 5")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        send(RESET, [])
        bench.wait_event("rustCounter.ResetIssued")
        send(THRESHOLD_SET, [5])                          # threshold_not_set acts here if L2-2 was drawn
        send(BUMP, [9])
        bench.wait_event("rustCounter.Saturated", timeout=10.0)
        send(THRESHOLD_SET, [100])
        bracket(bench)
        check(ABS)
        check("event rustCounter.Saturated(value=5, threshold=5) count == 1")
        check("last(rustCounter.Counter) == 5")
        check("event rustCounter.Bumped count == 0")
        check(NO_OPCODE_ERROR)


@pytest.mark.req("REQ-FprimeRust-009", "REQ-FprimeRust-010", "REQ-FprimeRust-011", "REQ-RustCounter-002",
                 "REQ-RustCounter-004", "REQ-RustMonitor-003", "REQ-Relay-001", "REQ-Relay-002")
def test_L2_3(window, send, bench, check, require, fth_plan):
    with window(50, 51):
        require(STD)
        require("commands_sent == 4")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 3")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        send(RESET, [])
        bench.wait_event("rustCounter.ResetIssued")
        send(BUMP, [3])                                   # bump_wrong_amount acts here if L2-3 was drawn
        bench.wait_event("rustCounter.Bumped")
        bench.wait(2.0)
        bracket(bench)
        check(ABS)
        check("event rustCounter.Bumped(value=3) count == 1")
        check("relay.Relayed advances by 2", plan_helper(fth_plan, "counter_advances", channel="relay.Relayed", by=2))
        check("last(rustMonitor.LastCount) == 3")
        check("event rustMonitor.LimitExceeded count == 0")
        check(NO_OPCODE_ERROR)


@pytest.mark.req("REQ-FprimeRust-008", "REQ-FprimeRust-009", "REQ-FprimeRust-011", "REQ-FprimeRust-014",
                 "REQ-RustCounter-005", "REQ-RustMonitor-005")
def test_L2_4(window, send, bench, check, require, fth_plan):
    with window(60, 61):
        require(STD)
        require("commands_sent == 2")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 1")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        bench.wait(5.0)
        bracket(bench)
        check(ABS)
        check("delta(rustCounter.TickCount) in 7..10", plan_helper(fth_plan, "delta_in_range", channel="rustCounter.TickCount", lo=7, hi=10))
        check("delta(rustMonitor.RunCount) in 7..10", plan_helper(fth_plan, "delta_in_range", channel="rustMonitor.RunCount", lo=7, hi=10))
        check(NO_OPCODE_ERROR)


@pytest.mark.req("REQ-FprimeRust-008", "REQ-FprimeRust-012", "REQ-FprimeRust-013", "REQ-RustMonitor-001",
                 "REQ-RustMonitor-004", "REQ-RustMonitor-006", "REQ-RustMonitor-003")
def test_L2_5(window, send, bench, check, require, fth_plan):
    with window(70, 71):
        require(STD)
        require("commands_sent == 6")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 5")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        send(SET_LIMIT, [4])                              # limit_not_set acts here if L2-5 was drawn
        bench.wait_event("rustMonitor.LimitSet")
        send(RESET, [])
        bench.wait_event("rustCounter.ResetIssued")
        send(BUMP, [10])
        bench.wait_event("rustCounter.Bumped")
        bench.wait(2.0)
        send(SET_LIMIT, [U32_MAX])
        bench.wait_event("rustMonitor.LimitSet")
        bench.wait(2.0)
        bracket(bench)
        check(ABS)
        check("event rustMonitor.LimitSet(limit=4) count == 1")
        check("event rustMonitor.LimitExceeded(count=10, limit=4) count == 1")
        check("rustMonitor.Exceedances advances by 1", plan_helper(fth_plan, "counter_advances", channel="rustMonitor.Exceedances", by=1))
        check("last(rustMonitor.LastCount) == 10")
        check("event rustMonitor.LimitSet count == 2")
        check("first(rustMonitor.Limit) == 4294967295")
        check("last(rustMonitor.Limit) == 4294967295")
        check(NO_OPCODE_ERROR)


# ---- L4: endurance ---------------------------------------------------------------------

@pytest.mark.req("REQ-FprimeRust-014", "REQ-RustCounter-005", "REQ-RustMonitor-005")
def test_L4_1(window, send, bench, check, require, fth_plan):
    with window(80, 81):
        require(STD)
        require("commands_sent == 2")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 1")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        bench.wait(60.0)
        bracket(bench)
        check(ABS)
        check("delta(rustCounter.TickCount) in 55..65", plan_helper(fth_plan, "delta_in_range", channel="rustCounter.TickCount", lo=55, hi=65))
        check("delta(rustMonitor.RunCount) in 55..65", plan_helper(fth_plan, "delta_in_range", channel="rustMonitor.RunCount", lo=55, hi=65))
        check("delta(rustCounter.Counter) == 0")
        check("event rustCounter.Saturated count == 0")
        check(NO_OPCODE_ERROR)


# ---- L5: saturation fault path ----------------------------------------------------------

@pytest.mark.req("REQ-FprimeRust-013", "REQ-FprimeRust-015", "REQ-RustCounter-003", "REQ-RustCounter-006")
def test_L5_1(window, send, bench, check, require):
    with window(90, 91):
        require(STD)
        require("commands_sent == 7")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 6")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        send(RESET, [])
        bench.wait_event("rustCounter.ResetIssued")
        send(THRESHOLD_SET, [3])
        send(BUMP, [2])                                   # bump_zero acts on both BUMPs if L5-1 was drawn
        bench.wait_event("rustCounter.Bumped", timeout=10.0)
        send(BUMP, [2])
        bench.wait_event("rustCounter.Saturated", timeout=10.0)
        send(THRESHOLD_SET, [100])
        bracket(bench)
        check(ABS)
        check("event rustCounter.Bumped(value=2) count == 1")
        check("event rustCounter.Saturated(value=3, threshold=3) count == 1")
        check("last(rustCounter.Counter) == 3")
        check("event rustCounter.Bumped(value=2) precedes event rustCounter.Saturated(value=3, threshold=3)")
        check(NO_OPCODE_ERROR)


# ---- L6: negative control ----------------------------------------------------------------

@pytest.mark.req("REQ-FprimeRust-012", "REQ-FprimeRust-013", "REQ-RustMonitor-002", "REQ-RustMonitor-006")
def test_L6_1(window, send, bench, check, require, fth_plan):
    with window(100, 101):
        require(STD)
        require("commands_sent == 3")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 2")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        send(SET_LIMIT, [0], wait_completion=False)       # negative_wrong_limit acts here if L6-1 was drawn;
        bench.wait_event("cmdDisp.OpCodeDispatched")      # a rejected command completes with OpCodeError, never OpCodeCompleted
        bench.wait(2.0)
        bracket(bench)
        check(ABS)
        check("event rustMonitor.InvalidLimit(limit=0) count == 1")
        check("event cmdDisp.OpCodeError(error=VALIDATION_ERROR) count == 1")
        check("event rustMonitor.LimitSet count == 0")
        check("delta(rustMonitor.Limit) == 0")
        check("last(rustMonitor.Limit) == 4294967295")
        check("rustMonitor.Exceedances advances by 0", plan_helper(fth_plan, "counter_advances", channel="rustMonitor.Exceedances", by=0))
        check("event rustCounter.Bumped count == 0")


# ---- L7: final state ---------------------------------------------------------------------

@pytest.mark.req("REQ-FprimeRust-010", "REQ-FprimeRust-011", "REQ-RustCounter-001", "REQ-RustCounter-004",
                 "REQ-Relay-001", "REQ-Relay-002", "REQ-RustMonitor-003")
def test_L7_1(window, send, bench, check, require, fth_plan):
    with window(110, 111):
        require(STD)
        require("commands_sent == 7")                    # sender log: every stimulus command and both markers
        check("event cmdDisp.OpCodeDispatched count == 6")  # the UUT dispatched each of them but the START marker,
                                                          # whose dispatch row precedes its marker event (A5)
        bracket(bench)
        send(THRESHOLD_SET, [42])
        send(THRESHOLD_SAVE, [])
        send(PRM_SAVE_FILE, [])                           # threshold_not_saved acts here if L7-1 was drawn
        bench.wait(2.0)
        send(SEND_FILE, ["PrmDb.dat", "PrmDb.dat"], wait_completion=False)  # -> record/raw/files/fprime-downlink/PrmDb.dat;
        bench.wait_event("cmdDisp.OpCodeDispatched")      # with no file it completes with OpCodeError, never OpCodeCompleted
        bench.wait(5.0)
        send(RESET, [])                                   # skip_reset acts here if L7-1 was drawn
        bench.wait_event("rustCounter.ResetIssued")
        bracket(bench)
        check(ABS)
        check("event prmDb.PrmFileSaveComplete(records=1) count == 1")
        check("event fileDownlink.FileSent count == 1")
        check("PrmDb.dat record 0x10016000 == 42",
              plan_helper(fth_plan, "prmdb_record_equals", path="files/fprime-downlink/PrmDb.dat", id=0x10016000, value=42))
        check("event rustCounter.ResetIssued count == 1")
        check("last(rustCounter.Counter) == 0")
        check("last(rustMonitor.LastCount) == 0")
        check("relay.Relayed advances by 1", plan_helper(fth_plan, "counter_advances", channel="relay.Relayed", by=1))
        check(NO_OPCODE_ERROR)
