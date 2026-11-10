"""D-1: pace rustTester.DUMP_COVERAGE windows over the instrumented twin until CoverageDumpDone (gcov-dump plan)."""
import pytest

MARK = "FprimeRustReference.rustTester.MARK"
DUMP = "FprimeRustReference.rustTester.DUMP_COVERAGE"
CHUNKS = 8          # chunks per window (two events fit one downlink frame; count > 8 is a VALIDATION_ERROR)
MAX_WINDOWS = 120   # 120 * 8 * 32 B = 30 KiB of gcda; the twin holds ~18 KiB


def test_D_1(bench, window, send, check, require):
    with window(1, 2):
        check("event rustTester.CoverageRejected count == 0")        # declared first: judged at window close from the record
        check("event rustTester.CoverageUnavailable count == 0")
        check("event cmdDisp.OpCodeError count == 0")
        send(MARK, [1])
        bench.wait_event("rustTester.Marker")
        state = {"done": False}
        windows = 1 if bench.dry_run else MAX_WINDOWS          # dry run: one window, nothing to wait for
        for start in range(0, CHUNKS * windows, CHUNKS):
            send(DUMP, [start, CHUNKS])
            bench.wait(0.2)                        # ground paces the walk: one window per command
            try:
                bench.wait_event("rustTester.CoverageDumpDone", timeout=0.5)
                state["done"] = True
                break
            except TimeoutError:
                continue
        require("coverage dump done seen", lambda w: state["done"])   # infrastructure: the walk reached the end
        send(MARK, [2])
        bench.wait_event("rustTester.Marker")
