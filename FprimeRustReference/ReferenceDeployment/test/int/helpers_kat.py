"""Known-answer tests (KATs) for the helper predicates of ``planted_faults.py`` (A15, reviewer round 1 F-1/S-1).

Plain pytest, no fth fixtures; NOT collected by ``fth run``/``fth verify-plan`` (the file name is outside the
``test_*.py`` glob) — run explicitly from the deployment directory::

    python -m pytest test/int/helpers_kat.py -q

Every helper is driven through real fth objects (``Record``/``Window``/``HelperContext``) built from synthetic rows,
so the pass AND the fail path of each predicate has a known answer. The last test guards the stimulus-declared
rationale predicates of ``plan.json`` against a dropped ``check()``.
"""
import json
import struct
import sys
import zlib
from pathlib import Path

import pytest
from fth.engine import HelperContext
from fth.registry import load_helpers_module
from fth.replay import ChannelRow, EventRow, Record, Window

HERE = Path(__file__).resolve().parent
MARKER = "rustTester.Marker"
HELPERS = load_helpers_module(HERE / "planted_faults.py").helpers

# dev-clean record raw/files/fprime-downlink/PrmDb.dat (17 bytes): CRC32, 0xA5, size 8, id 0x10016000, U32 42
PRMDB_GOOD = bytes.fromhex("4ceb3a87" "a5" "00000008" "10016000" "0000002a")
PRM_PATH = "files/fprime-downlink/PrmDb.dat"
PRM_ID = 0x10016000


def _event(idx, t, short, args):
    return EventRow(idx, t, 0, t, "FprimeRustReference." + short, short, 0, "ACTIVITY_LO", "", dict(args), {})


def _sample(idx, t, short, value):
    return ChannelRow(idx, t, 0, t, "FprimeRustReference." + short, short, 0, value, value)


def _ctx(events, channels, path=Path("."), start=1, end=2):
    rec = Record(path, list(events), list(channels), [], marker_event=MARKER, marker_command=None, patterns={},
                 marker_arg="id")
    w = Window(rec, start, end, label="W")
    return HelperContext(rec, None, None, w, {"W": w})


def _window(samples, channel="rustCounter.TickCount", prior=(), markers=True):
    """START marker at t=10, END marker at t=20; ``samples`` are values at t=11.., ``prior`` values before t=10."""
    ev = [_event(1, 10.0, MARKER, {"id": 1}), _event(2, 20.0, MARKER, {"id": 2})] if markers else []
    ch = [_sample(i + 1, 1.0 + i, channel, v) for i, v in enumerate(prior)]
    ch += [_sample(len(ch) + i + 1, 11.0 + i, channel, v) for i, v in enumerate(samples)]
    return _ctx(ev, ch)


def _crc(body):
    return struct.pack(">I", (zlib.crc32(body) ^ 0xFFFFFFFF) & 0xFFFFFFFF) + body


# -- delta_in_range -------------------------------------------------------------------------
@pytest.mark.parametrize("first,last,lo,hi,holds", [
    (87, 94, 7, 11, True),    # delta == lo
    (87, 98, 7, 11, True),    # delta == hi
    (87, 93, 7, 11, False),   # delta == lo - 1
    (87, 99, 7, 11, False),   # delta == hi + 1
    (98, 87, 7, 11, False),   # negative delta (counter went backwards / wrapped)
    (0, 0, 0, 0, True),       # exact zero bound
])
def test_delta_in_range_bounds(first, last, lo, hi, holds):
    r = HELPERS["delta_in_range"](_window([first, first + 1, last]), channel="rustCounter.TickCount", lo=lo, hi=hi)
    assert r[0] is holds, r[1]
    assert len(r) == 3 or r[3] is False
    assert r[2], "evidence refs expected"


def test_delta_in_range_one_sample_is_unverified():
    r = HELPERS["delta_in_range"](_window([87]), channel="rustCounter.TickCount", lo=7, hi=11)
    assert r[0] is False and r[3] is True


def test_delta_in_range_other_channel_is_unverified():
    r = HELPERS["delta_in_range"](_window([87, 90]), channel="rustMonitor.RunCount", lo=0, hi=100)
    assert r[0] is False and r[3] is True


# -- counter_advances -----------------------------------------------------------------------
@pytest.mark.parametrize("prior,samples,by,holds", [
    ((3,), (4,), 1, True),       # baseline 3 before START, last 4 inside -> advance 1
    ((3,), (4,), 2, False),      # advance 1 != 2
    ((3,), (4,), 0, False),      # advance 1 != 0
    ((), (1,), 1, True),         # no prior sample: boot value 0 -> advance 1
    ((), (2,), 1, False),        # boot value 0 -> advance 2 != 1
    ((5,), (), 0, True),         # nothing written in the window: advance 0
    ((5,), (), 1, False),        # nothing written but 1 expected
    ((), (), 0, True),           # never written at all
    ((3,), (4, 5), 2, True),     # last sample counts, not the first
])
def test_counter_advances(prior, samples, by, holds):
    r = HELPERS["counter_advances"](_window(samples, channel="relay.Relayed", prior=prior), channel="relay.Relayed", by=by)
    assert r[0] is holds, r[1]
    assert len(r) == 3 or r[3] is False


def test_counter_advances_without_start_marker_is_unverified():
    r = HELPERS["counter_advances"](_window([4], channel="relay.Relayed", markers=False), channel="relay.Relayed", by=1)
    assert r[0] is False and r[3] is True


# -- version_payloads_present ---------------------------------------------------------------
VERSION_EVENTS = ["version.FrameworkVersion", "version.ProjectVersion", "version.LibraryVersions"]


def _version_ctx(payloads):
    ev = [_event(1, 10.0, MARKER, {"id": 1})]
    ev += [_event(i + 2, 11.0 + i, name, {"version": v}) for i, (name, v) in enumerate(payloads)]
    ev.append(_event(len(ev) + 1, 20.0, MARKER, {"id": 2}))
    return _ctx(ev, [])


def test_version_payloads_present_good():
    r = HELPERS["version_payloads_present"](_version_ctx(list(zip(VERSION_EVENTS, ["v4.0.0", "1.0.0", "[fprime-rust 0.1]"]))),
                                            events=VERSION_EVENTS)
    assert r[0] is True and len(r[2]) == 3


@pytest.mark.parametrize("bad", ["", "UNKNOWN", "unknown"])
def test_version_payloads_present_bad_payload_fails(bad):
    r = HELPERS["version_payloads_present"](_version_ctx(list(zip(VERSION_EVENTS, ["v4.0.0", bad, "x"]))),
                                            events=VERSION_EVENTS)
    assert r[0] is False and (len(r) == 3 or r[3] is False)


def test_version_payloads_present_absent_event_fails_not_unverified():
    r = HELPERS["version_payloads_present"](_version_ctx(list(zip(VERSION_EVENTS[:2], ["v4.0.0", "1.0.0"]))),
                                            events=VERSION_EVENTS)
    assert r[0] is False and (len(r) == 3 or r[3] is False), "a missing UUT event in a complete window is FAIL (S-3)"


# -- prmdb_record_equals --------------------------------------------------------------------
def _prm_ctx(tmp_path, data):
    if data is not None:
        f = tmp_path / "raw" / PRM_PATH
        f.parent.mkdir(parents=True)
        f.write_bytes(data)
    return _ctx([_event(1, 10.0, MARKER, {"id": 1}), _event(2, 20.0, MARKER, {"id": 2})], [], path=tmp_path)


def _prm(tmp_path, data, value=42, id=PRM_ID):
    return HELPERS["prmdb_record_equals"](_prm_ctx(tmp_path, data), path=PRM_PATH, id=id, value=value)


def test_prmdb_good_bytes_crc_matches_svc_prmdb():
    assert _crc(PRMDB_GOOD[4:]) == PRMDB_GOOD, "the CRC rule must reproduce the real Svc.PrmDb file"


def test_prmdb_record_equals_pass(tmp_path):
    r = _prm(tmp_path, PRMDB_GOOD)
    assert r[0] is True and "CRC32 ok" in r[1] and r[2]


def test_prmdb_wrong_value_fails(tmp_path):
    body = bytearray(PRMDB_GOOD[4:]); body[-1] = 0x2B
    r = _prm(tmp_path, _crc(bytes(body)))
    assert r[0] is False and "expected 42" in r[1]


def test_prmdb_expected_other_value_fails(tmp_path):
    r = _prm(tmp_path, PRMDB_GOOD, value=43)
    assert r[0] is False and "= 42 (expected 43)" in r[1]


def test_prmdb_flipped_crc_byte_fails(tmp_path):
    data = bytearray(PRMDB_GOOD); data[0] ^= 0x01
    r = _prm(tmp_path, bytes(data))
    assert r[0] is False and "CRC32 mismatch" in r[1]


def test_prmdb_flipped_body_byte_fails_crc(tmp_path):
    data = bytearray(PRMDB_GOOD); data[-1] = 0x2B  # value changed, CRC not recomputed
    r = _prm(tmp_path, bytes(data))
    assert r[0] is False and "CRC32 mismatch" in r[1]


def test_prmdb_bad_delimiter_fails(tmp_path):
    body = bytearray(PRMDB_GOOD[4:]); body[0] = 0xA4
    r = _prm(tmp_path, _crc(bytes(body)))
    assert r[0] is False and "bad delimiter" in r[1]


def test_prmdb_missing_id_fails(tmp_path):
    body = bytearray(PRMDB_GOOD[4:]); body[5:9] = struct.pack(">I", PRM_ID + 1)
    r = _prm(tmp_path, _crc(bytes(body)))
    assert r[0] is False and "not in" in r[1]


def test_prmdb_short_file_fails(tmp_path):
    r = _prm(tmp_path, PRMDB_GOOD[:3])
    assert r[0] is False and "shorter than a CRC" in r[1]


def test_prmdb_absent_file_is_fail_not_unverified(tmp_path):
    r = _prm(tmp_path, None)
    assert r[0] is False and r[2] == [] and r[3] is False and "absent" in r[1]


def test_prmdb_two_records_finds_the_id(tmp_path):
    other = bytes([0xA5]) + struct.pack(">II", 8, 0x10016001) + struct.pack(">I", 7)
    r = _prm(tmp_path, _crc(other + PRMDB_GOOD[4:]))
    assert r[0] is True


# -- plan consistency (S-1): every stimulus-declared rationale predicate is declared by its test ---------------
def test_stimulus_declared_rationale_predicates_are_declared_checks():
    from fth.junit import harvest_check_names
    plan = json.loads((HERE / "plan.json").read_text(encoding="utf-8"))
    declared = harvest_check_names(HERE / "plan.json", HERE, HERE / "bench.json")
    missing = []
    for t in plan["tests"]:
        for r in t.get("rationale", []):
            if r.get("text", "").startswith("stimulus-declared") and r["pred"] not in declared.get(t["id"], []):
                missing.append(f"{t['id']}: {r['pred']}")
    assert not missing, "rationale predicates no check() declares: " + "; ".join(missing)
