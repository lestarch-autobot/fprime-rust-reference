# RUST-REF — fprime-rust on FprimeRustReference (native Linux x86_64, fprime-gds)

## 0 Scope
- (none)

## 1 Entry gates
- (none)

## 2 Test levels
L0
L2
L4
L5
L6
L7

## 3 Test entries

Window rule: rows from the **last** `cmdDisp.NoOpStringReceived(id=START)` row that precedes the first `cmdDisp.NoOpStringReceived(id=END)` row following it, through that END row plus the END `cmdDisp.CMD_NO_OP_STRING` command's own `cmdDisp` rows (at most two), inclusive; ordering key `t = epoch·1e6 + fsw`, archive index on ties. Channel rows are those with `t` between the first and last window event. `commands_sent` = commands from the matching START sending (its last sending if re-sent) through the END sending, inclusive.

Standard `complete` block (STD): both markers present; RustReference samples >= 2.
Standard absence block (ABS): event cmdDisp.MalformedCommand count == 0; event cmdDisp.InvalidCommand count == 0; event cmdDisp.TooManyCommands count == 0.
Packets (named channel sets for `<packet> samples >= n`): RustReference = rustCounter.Counter, rustCounter.TickCount, rustMonitor.RunCount, rustMonitor.Limit.

ID:        L0-1
Title:     Image identity
Purpose:   Confirms the running executable reports non-empty project, framework and library version strings for the delivery manifest check.
Covers:    — (entry gate; image_version policy required: fth deliver/audit compare these payloads and the dictionary metadata with manifest repos[].describe)
Setup:     Image installed by flash, GDS attached, ≥ 2 s after start.
Procedure: MARK(10); bracket (2 samples); CdhCore.version.VERSION(ALL); bracket; MARK(11).
Window:    Marker(id=10) … Marker(id=11)
Pass rule: named checks declared in the integration test (test_L0_1). metrics: event version.ProjectVersion count == 1; event version.FrameworkVersion count == 1; event version.LibraryVersions count == 1 (one library (lib/fprime-rust) is registered in settings.ini library_locations, so Svc.Version emits one LibraryVersions event; fth v0.1.2 has no manifest-substituted predicate, so equality with manifest repos[].describe is the deliver/audit dictionary-describe check (fth audit.dictionary_describe_check), recorded here as the image_version policy); event cmdDisp.OpCodeError count == 0.
Evidence:  ProjectVersion, FrameworkVersion and LibraryVersions events with non-empty payloads; describe equality judged at deliver/unseal.
Planted faults:none

ID:        L0-2
Title:     Marker round-trip
Purpose:   Proves the marker command round-trips through the command path so every later window can be bracketed.
Covers:    — (entry gate; harness completeness)
Setup:     As L0-1.
Procedure: MARK(20); bracket (2 samples); MARK(42); bracket; MARK(21).
Window:    Marker(id=20) … Marker(id=21)
Pass rule: named checks declared in the integration test (test_L0_2). metrics: event cmdDisp.NoOpStringReceived count == 3; event cmdDisp.OpCodeError count == 0.
Evidence:  Three NoOpStringReceived events in order (20, 42, 21).
Planted faults:none

ID:        L2-1
Title:     Rust command, event and telemetry paths
Purpose:   Resets and bumps the Rust active component to prove async commands dispatch into Rust and events and telemetry come back out.
Covers:    REQ-FprimeRust-008, REQ-FprimeRust-012, REQ-FprimeRust-013, REQ-FprimeRust-014, REQ-RustCounter-001, REQ-RustCounter-002, REQ-RustCounter-004
Setup:     L0-2 passed; THRESHOLD at default 100.
Procedure: MARK(30); bracket (2 samples); rustCounter.RESET(); rustCounter.BUMP(7); rustCounter.BUMP(5); bracket (2 samples); MARK(31).
Window:    Marker(id=30) … Marker(id=31)
Pass rule: named checks declared in the integration test (test_L2_1). metrics: event rustCounter.ResetIssued count == 1; event rustCounter.Bumped(value=7) count == 1; event rustCounter.Bumped(value=12) count == 1; event rustCounter.ResetIssued precedes event rustCounter.Bumped(value=7); last(rustCounter.Counter) == 12; event rustCounter.Saturated count == 0; event cmdDisp.OpCodeError count == 0.
Evidence:  ResetIssued, Bumped(7), Bumped(12); Counter 12; no Saturated; no command error.
Planted faults:bump_wrong_amount, skip_reset

ID:        L2-2
Title:     Parameter read from Rust
Purpose:   Sets THRESHOLD to 5 and bumps past it to prove the Rust handler reads the parameter and saturates on it.
Covers:    REQ-FprimeRust-015, REQ-RustCounter-003, REQ-RustCounter-006
Setup:     L2-1 passed (opens with RESET, so L2-1's planted faults do not ripple here).
Procedure: MARK(40); bracket (2 samples); rustCounter.RESET(); rustCounter.THRESHOLD_PRM_SET(5); rustCounter.BUMP(9); rustCounter.THRESHOLD_PRM_SET(100); bracket (2 samples); MARK(41).
Window:    Marker(id=40) … Marker(id=41)
Pass rule: named checks declared in the integration test (test_L2_2). metrics: event rustCounter.Saturated(value=5, threshold=5) count == 1 (THRESHOLD is read on every BUMP (RustCounter §Behavior), so the set value takes effect without a restart); last(rustCounter.Counter) == 5; event rustCounter.Bumped count == 0; event cmdDisp.OpCodeError count == 0.
Evidence:  Saturated(5, 5); Counter 5; no Bumped.
Planted faults:threshold_not_set

ID:        L2-3
Title:     Rust to C++ to Rust port chain
Purpose:   Bumps the Rust counter and follows the count through the C++ relay into the Rust monitor to prove typed ports in both directions.
Covers:    REQ-FprimeRust-009, REQ-FprimeRust-010, REQ-FprimeRust-011, REQ-RustCounter-002, REQ-RustCounter-004, REQ-RustMonitor-003, REQ-Relay-001, REQ-Relay-002
Setup:     L2-1 passed; rustMonitor limit at initial 4294967295.
Procedure: MARK(50); bracket (2 samples); rustCounter.RESET(); rustCounter.BUMP(3); wait 2 s; bracket (2 samples); MARK(51).
Window:    Marker(id=50) … Marker(id=51)
Pass rule: named checks declared in the integration test (test_L2_3). metrics: event rustCounter.Bumped(value=3) count == 1; last(rustMonitor.LastCount) == 3; event rustMonitor.LimitExceeded count == 0; event cmdDisp.OpCodeError count == 0.
Evidence:  Bumped(3); Relayed +2; LastCount 3.
Planted faults:bump_wrong_amount

ID:        L2-4
Title:     Rate-group ports into Rust
Purpose:   Lets the 1 Hz rate group tick the active Rust component and run the queued Rust component to prove C++ scheduler ports into Rust handlers.
Covers:    REQ-FprimeRust-008, REQ-FprimeRust-009, REQ-FprimeRust-011, REQ-FprimeRust-014, REQ-RustCounter-005, REQ-RustMonitor-005
Setup:     L0-2 passed.
Procedure: MARK(60); bracket (2 samples); wait 5 s; bracket (2 samples); MARK(61).
Window:    Marker(id=60) … Marker(id=61)
Pass rule: named checks declared in the integration test (test_L2_4). metrics: event cmdDisp.OpCodeError count == 0.
Evidence:  TickCount and RunCount each advance 7..11 over the window.
Planted faults:none

ID:        L2-5
Title:     Queued Rust command and limit exceedance
Purpose:   Sets the monitor limit then bumps past it to prove a queued Rust component handles commands and raises a warning event from a relayed count.
Covers:    REQ-FprimeRust-008, REQ-FprimeRust-012, REQ-FprimeRust-013, REQ-RustMonitor-001, REQ-RustMonitor-004, REQ-RustMonitor-006, REQ-RustMonitor-003
Setup:     L2-3 passed.
Procedure: MARK(70); bracket (2 samples); rustMonitor.SET_LIMIT(4); rustCounter.RESET(); rustCounter.BUMP(10); wait 2 s; rustMonitor.SET_LIMIT(4294967295); wait 2 s; bracket (2 samples); MARK(71).
Window:    Marker(id=70) … Marker(id=71)
Pass rule: named checks declared in the integration test (test_L2_5). metrics: event rustMonitor.LimitSet(limit=4) count == 1; event rustMonitor.LimitExceeded(count=10, limit=4) count == 1 (the count 10 reaches the queued component on the next 1 Hz run, within the 2 s wait); last(rustMonitor.LastCount) == 10; event rustMonitor.LimitSet count == 2; first(rustMonitor.Limit) == 4294967295 (Limit is published every run, so the first sample in the window shows the initial limit before SET_LIMIT(4) is dispatched); last(rustMonitor.Limit) == 4294967295; event cmdDisp.OpCodeError count == 0.
Evidence:  LimitSet(4), LimitExceeded(10, 4), Exceedances +1, limit restored.
Planted faults:limit_not_set

ID:        L4-1
Title:     Endurance 60 s
Purpose:   Runs the deployment for 60 s to prove the Rust components keep servicing rate-group ports with no command errors or counter drift.
Covers:    REQ-FprimeRust-014, REQ-RustCounter-005, REQ-RustMonitor-005
Setup:     L2-4 passed.
Procedure: MARK(80); bracket (2 samples); wait 60 s; bracket (2 samples); MARK(81).
Window:    Marker(id=80) … Marker(id=81)
Pass rule: named checks declared in the integration test (test_L4_1). metrics: delta(rustCounter.Counter) == 0; event rustCounter.Saturated count == 0; event cmdDisp.OpCodeError count == 0.
Evidence:  55..65 ticks and runs; Counter unchanged.
Planted faults:none

ID:        L5-1
Title:     Saturation fault path
Purpose:   Drives the counter into THRESHOLD saturation to prove the Rust warning event and the clamp.
Covers:    REQ-FprimeRust-013, REQ-FprimeRust-015, REQ-RustCounter-003, REQ-RustCounter-006
Setup:     L2-2 passed.
Procedure: MARK(90); bracket (2 samples); rustCounter.RESET(); rustCounter.THRESHOLD_PRM_SET(3); rustCounter.BUMP(2); rustCounter.BUMP(2); rustCounter.THRESHOLD_PRM_SET(100); bracket (2 samples); MARK(91).
Window:    Marker(id=90) … Marker(id=91)
Pass rule: named checks declared in the integration test (test_L5_1). metrics: event rustCounter.Bumped(value=2) count == 1; event rustCounter.Saturated(value=3, threshold=3) count == 1; last(rustCounter.Counter) == 3; event rustCounter.Bumped(value=2) precedes event rustCounter.Saturated(value=3, threshold=3); event cmdDisp.OpCodeError count == 0.
Evidence:  Bumped(2) then Saturated(3, 3); Counter 3.
Planted faults:bump_zero

ID:        L6-1
Title:     Negative control: SET_LIMIT(0) rejected
Purpose:   Sends an invalid limit to prove the Rust handler rejects it with a validation error and changes nothing else.
Covers:    REQ-FprimeRust-012, REQ-FprimeRust-013, REQ-RustMonitor-002, REQ-RustMonitor-006
Setup:     L2-5 passed; limit at 4294967295.
Procedure: MARK(100); bracket (2 samples); rustMonitor.SET_LIMIT(0); wait 2 s; bracket (2 samples); MARK(101).
Window:    Marker(id=100) … Marker(id=101)
Pass rule: named checks declared in the integration test (test_L6_1). metrics: event rustMonitor.InvalidLimit(limit=0) count == 1; event cmdDisp.OpCodeError(error=VALIDATION_ERROR) count == 1 (negative control: the rejection is visible at the dispatcher and nothing else happened); event rustMonitor.LimitSet count == 0; delta(rustMonitor.Limit) == 0; last(rustMonitor.Limit) == 4294967295; event rustCounter.Bumped count == 0.
Evidence:  InvalidLimit(0), one OpCodeError(VALIDATION_ERROR), Limit and Exceedances unchanged.
Planted faults:negative_wrong_limit

ID:        L7-1
Title:     Final state
Purpose:   Saves THRESHOLD 42 into the parameter file, downlinks the file into the record and resets the counter to leave the deployment in an independently verifiable state.
Covers:    REQ-FprimeRust-010, REQ-FprimeRust-011, REQ-RustCounter-001, REQ-RustCounter-004, REQ-Relay-001, REQ-Relay-002, REQ-RustMonitor-003 — final-state evidence (THRESHOLD persistence in PrmDb.dat) is owned by the autocoded parameter base + Svc.PrmDb, not traced to a component row (decision log 11)
Setup:     All earlier tests run.
Procedure: MARK(110); bracket (2 samples); rustCounter.THRESHOLD_PRM_SET(42); rustCounter.THRESHOLD_PRM_SAVE(); FileHandling.prmDb.PRM_SAVE_FILE(); wait 2 s; FileHandling.fileDownlink.SendFile(PrmDb.dat, PrmDb.dat); wait 5 s; rustCounter.RESET(); bracket (2 samples); MARK(111); action: the recorder's GDS stores downlinked files under {logs}/files (bench gds --file-storage-directory), i.e. record/raw/files/fprime-downlink/PrmDb.dat; the prmdb_record_equals check parses it (CRC32, 0xA5 delimiter, U32 record size, FwPrmIdType id, value; Svc/PrmDb/docs/sdd.md §3.2).
Window:    Marker(id=110) … Marker(id=111)
Pass rule: named checks declared in the integration test (test_L7_1). metrics: event prmDb.PrmFileSaveComplete(records=1) count == 1 (THRESHOLD is the only parameter in the deployment, so the saved file holds exactly one record); event fileDownlink.FileSent count == 1; event rustCounter.ResetIssued count == 1; last(rustCounter.Counter) == 0; event cmdDisp.OpCodeError count == 0.
Evidence:  PrmFileSaveComplete(1), FileSent, ResetIssued; Counter 0; LastCount 0; Relayed +1; record/raw/files/PrmDb.dat holds THRESHOLD (0x10016000) = 42.
Planted faults:skip_reset, threshold_not_saved

## 4 Traceability
| REQ | Tests | Note |
|---|---|---|
| REQ-FprimeRust-001 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-002 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-003 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-004 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-005 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-006 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-007 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-008 | Unit Test; Integration Test: L2-1, L2-4, L2-5 | Unit Test; Integration Test |
| REQ-FprimeRust-009 | Unit Test; Integration Test: L2-3, L2-4 | Unit Test; Integration Test |
| REQ-FprimeRust-010 | Unit Test; Integration Test: L2-3, L7-1 | Unit Test; Integration Test |
| REQ-FprimeRust-011 | Integration Test: L2-3, L2-4, L7-1 | Integration Test |
| REQ-FprimeRust-012 | Unit Test; Integration Test: L2-1, L2-5, L6-1 | Unit Test; Integration Test |
| REQ-FprimeRust-013 | Unit Test; Integration Test: L2-1, L2-5, L5-1, L6-1 | Unit Test; Integration Test |
| REQ-FprimeRust-014 | Unit Test; Integration Test: L2-1, L2-4, L4-1 | Unit Test; Integration Test |
| REQ-FprimeRust-015 | Unit Test; Integration Test: L2-2, L5-1 | Unit Test; Integration Test |
| REQ-FprimeRust-016 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-017 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-018 | waived: Unit Test; Inspection | Unit Test; Inspection |
| REQ-FprimeRust-020 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-021 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-022 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-023 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-024 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-025 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-026 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-027 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-028 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-030 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-031 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-032 | waived: Unit Test | Unit Test (shim fixtures UT-SHIM-9 and reference UT-RM-5: RustMonitor SET_LIMIT hook answers BUSY) |
| REQ-FprimeRust-033 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-034 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-035 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-036 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-037 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-038 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-039 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-040 | waived: Inspection | Inspection |
| REQ-RustCounter-001 | Unit Test; Integration Test: L2-1, L7-1 | Unit Test; Integration Test |
| REQ-RustCounter-002 | Unit Test; Integration Test: L2-1, L2-3 | Unit Test; Integration Test |
| REQ-RustCounter-003 | Unit Test; Integration Test: L2-2, L5-1 | Unit Test; Integration Test |
| REQ-RustCounter-004 | Unit Test; Integration Test: L2-1, L2-3, L7-1 | Unit Test; Integration Test |
| REQ-RustCounter-005 | Unit Test; Integration Test: L2-4, L4-1 | Unit Test; Integration Test |
| REQ-RustCounter-006 | Unit Test; Integration Test: L2-2, L5-1 | Unit Test; Integration Test |
| REQ-RustMonitor-001 | Unit Test; Integration Test: L2-5 | Unit Test; Integration Test |
| REQ-RustMonitor-002 | Unit Test; Integration Test: L6-1 | Unit Test; Integration Test |
| REQ-RustMonitor-003 | Unit Test; Integration Test: L2-3, L2-5, L7-1 | Unit Test; Integration Test |
| REQ-RustMonitor-004 | Unit Test; Integration Test: L2-5 | Unit Test; Integration Test |
| REQ-RustMonitor-005 | Unit Test; Integration Test: L2-4, L4-1 | Unit Test; Integration Test |
| REQ-RustMonitor-006 | Unit Test; Integration Test: L2-5, L6-1 | Unit Test; Integration Test |
| REQ-Relay-001 | Unit Test; Integration Test: L2-3, L7-1 | Unit Test; Integration Test |
| REQ-Relay-002 | Unit Test; Integration Test: L2-3, L7-1 | Unit Test; Integration Test |
| REQ-RustMonitor-007 | waived: Unit Test | Unit Test (UT-RM-5); BUSY observed on the bench with 12 SET_LIMITs in one 1 Hz period |

## 5 Metrics
- L0-1: event version.ProjectVersion count == 1; event version.FrameworkVersion count == 1; event version.LibraryVersions count == 1; event cmdDisp.OpCodeError count == 0
- L0-2: event cmdDisp.NoOpStringReceived count == 3; event cmdDisp.OpCodeError count == 0
- L2-1: event rustCounter.ResetIssued count == 1; event rustCounter.Bumped(value=7) count == 1; event rustCounter.Bumped(value=12) count == 1; event rustCounter.ResetIssued precedes event rustCounter.Bumped(value=7); last(rustCounter.Counter) == 12; event rustCounter.Saturated count == 0; event cmdDisp.OpCodeError count == 0
- L2-2: event rustCounter.Saturated(value=5, threshold=5) count == 1; last(rustCounter.Counter) == 5; event rustCounter.Bumped count == 0; event cmdDisp.OpCodeError count == 0
- L2-3: event rustCounter.Bumped(value=3) count == 1; last(rustMonitor.LastCount) == 3; event rustMonitor.LimitExceeded count == 0; event cmdDisp.OpCodeError count == 0
- L2-4: event cmdDisp.OpCodeError count == 0
- L2-5: event rustMonitor.LimitSet(limit=4) count == 1; event rustMonitor.LimitExceeded(count=10, limit=4) count == 1; last(rustMonitor.LastCount) == 10; event rustMonitor.LimitSet count == 2; first(rustMonitor.Limit) == 4294967295; last(rustMonitor.Limit) == 4294967295; event cmdDisp.OpCodeError count == 0
- L4-1: delta(rustCounter.Counter) == 0; event rustCounter.Saturated count == 0; event cmdDisp.OpCodeError count == 0
- L5-1: event rustCounter.Bumped(value=2) count == 1; event rustCounter.Saturated(value=3, threshold=3) count == 1; last(rustCounter.Counter) == 3; event rustCounter.Bumped(value=2) precedes event rustCounter.Saturated(value=3, threshold=3); event cmdDisp.OpCodeError count == 0
- L6-1: event rustMonitor.InvalidLimit(limit=0) count == 1; event cmdDisp.OpCodeError(error=VALIDATION_ERROR) count == 1; event rustMonitor.LimitSet count == 0; delta(rustMonitor.Limit) == 0; last(rustMonitor.Limit) == 4294967295; event rustCounter.Bumped count == 0
- L7-1: event prmDb.PrmFileSaveComplete(records=1) count == 1; event fileDownlink.FileSent count == 1; event rustCounter.ResetIssued count == 1; last(rustCounter.Counter) == 0; event cmdDisp.OpCodeError count == 0

## 6 Fault menu
Generated from `planted_faults.py` planted fault declarations (`fth planted-faults-menu`): bump_wrong_amount (harness-expectation; L2-1, L2-3); bump_zero (stimulus; L5-1); limit_not_set (stimulus; L2-5); negative_wrong_limit (stimulus; L6-1); skip_reset (stimulus; L2-1, L7-1); threshold_not_saved (uut-state; L7-1); threshold_not_set (stimulus; L2-2).

## 7 Not covered
L3 reset/persistence (no reset row on the native bench)

## 8 Decision log
1. Captain R1 (2026-11-09): Approve-SDD waived; design matches fprime-python (annotation, cmake shape, autocoder closure from fpp-cache/stdout.txt, .fpp+.rs ergonomics, cookiecutter, test/int).
2. Captain R4 (2026-11-09): ≥2 Rust components (RustCounter active, RustMonitor queued) + C++ Relay for Rust→C++ and C++→Rust ports; ONE Rust runtime per deployment via a deployment-level Cargo workspace (member rlibs, one staticlib root). Alternatives rejected in fprime-rust docs/sdd.md §Design.
3. Captain R5 (2026-11-09): no fpp-to-json / FPRIME_ENABLE_JSON_MODEL_GENERATION; fprime-fpp-python==4.0.1 + fprime-cpp-codegen==0.2.0; wheel platform constraint stated in the SDD. Spike on the campaign host: import + fpp.analyze clean with the fpp-depend closure.
4. Captain R7 (2026-11-09): native Linux x86_64 bench, fprime-gds, no hardware; bench.json flash/dump/gds/recorder/send as recorded; native executable (no suffix) is the manifest image; image_version required.
5. Captain (2026-11-09): pin lib/fprime to nasa/fprime devel b188d185ec46699b5fdc0a0666537896a5ea9d47 and lib/fprime-rust to lestarch-autobot/fprime-rust main; submodule edit is DEVELOPMENT work.
6. Author (2026-11-10): markers use CdhCore.cmdDisp.CMD_NO_OP_STRING / NoOpStringReceived so bracketing does not depend on the UUT's own command path; no helper component.
7. Author (2026-11-10): L3 not covered — the native bench has no reset row; acknowledged in coverage.not_covered_ack. Recommended ruling: accept, or add a reset row (kill + relaunch via gds) in DEVELOPMENT if persistence proof is wanted.
8. Author (2026-11-10, revised round 1): REQ-FprimeRust-018 (one Rust runtime per executable) is verified by Inspection of the build (UT-BUILD-1: exactly one libfprime_rust_<Deployment>.a on the link line, `nm` shows one definition of rust_begin_unwind, no duplicate-symbol diagnostics); L0-1 is an entry gate.
9. Author (2026-11-10, revised round 1): max planted faults per run = 2; menu holds 7 faults ≥ 2×2; every system-covered requirement has ≥2 covering tests with disjoint planted-fault sets or a waiver note; ripples stated per fault (every functional test opens with RESET or an explicit SET_LIMIT, which insulates it from the previous test's residue).
10. Author (round 1, revised round 2): component SDD requirement tables kept (anchor E-2); every REQ-RustCounter/RustMonitor/Relay id traced here with rationale. fth v0.1.2 `verify-plan --sdd` takes one baseline file, so the gate runs against the mechanical concatenation of the four committed SDDs, in this order and with this command from the reference root, with the precondition that `lib/fprime-rust` is checked out at the fprime-rust `campaign/design` DESIGN head (the committed submodule pin still points at JPL-Devin until the DEVELOPMENT repoint, decision 14): `cat lib/fprime-rust/docs/sdd.md FprimeRustReference/Components/RustCounter/docs/sdd.md FprimeRustReference/Components/RustMonitor/docs/sdd.md FprimeRustReference/Components/Relay/docs/sdd.md > /tmp/sdd-concat.md && fth verify-plan FprimeRustReference/ReferenceDeployment/test/int/plan.json --sdd /tmp/sdd-concat.md`; expected last line: `requirements ok: 52 plan requirement ids all in the SDD table (52 rows) of /tmp/sdd-concat.md`.
11. Author (round 1, revised round 2): unconnected output port invoked from Rust is owned by the autocoded component base (FW_ASSERT in X_out, F Prime user manual, ports); Rust sees the same abort as C++ (REQ-FprimeRust-024); Relay guards countOut with isConnected_countOut_OutputPort(). Rust allocation failure: aborts via std (no F Prime hook; stable Rust has no alloc_error_handler — amended by DEVELOPMENT ruling 4). THRESHOLD persistence in PrmDb.dat is owned by the autocoded parameter base (THRESHOLD_PRM_SAVE -> prmSetOut) and Svc.PrmDb (PRM_SAVE_FILE, Svc/PrmDb/docs/sdd.md); L7-1 parses the downlinked file as final-state evidence (covers_note), not as a RustCounter requirement.
12. Author (round 1): `@ fprime-rust` on a non-component element is ignored (UT-AC-1). THRESHOLD invalid/absent in prmDb: FPP default 100 applies (framework parameter semantics). RustMonitor queue full: countIn drops (FPP `drop`), SET_LIMIT asserts (F Prime default; amended by decision 17: hook → BUSY); Relay countOut unconnected: count dropped, Relayed still counts.
13. Author (round 1): bench `dump` reads the installed executable because fth v0.1.2 flash() requires read-back sha256 == image sha256; final state is the in-band PrmDb.dat save + file downlink (see final_state). NEW elicitation finding for the anchor.
14. Author (round 1): lib/fprime-rust submodule URL changes from https://github.com/JPL-Devin/fprime-rust.git (branch devin/1778113254-initial-import) to https://github.com/lestarch-autobot/fprime-rust.git branch main, and lib/fprime is pinned to nasa/fprime b188d185ec46699b5fdc0a0666537896a5ea9d47 — both DEVELOPMENT edits. bench/native_flash.py and bench/native_dump.py are design items DEVELOPMENT creates and the package pins.
15. Author (round 1): L0-1 judges that version events are present with non-empty payloads; equality with manifest repos[].describe is the fth deliver/audit dictionary-describe check (fth v0.1.2 engine has no manifest-substituted predicate), declared by image_version policy required.
16. Author (round 1): the fpp-check gate ran on the full closure (reference FPP + nasa/fprime b188d185 Fw/Svc/Drv/Os/Utils/CFDP/default/Fpp/cmake + unix PlatformTypes.fpp, fpp-check v4.0.0a1 = the pin's requirements.txt) — clean; topology uses `deployment topology` (accepted by fpp 4.0.0a1).
17. Author (round 2): RustMonitor.SET_LIMIT keeps the F Prime default `assert` on queue full (every async command in the deployment, CdhCore included, shares it); operational bound: fewer than 10 SET_LIMITs plus counts queued within one 1 Hz run (queue depth Default.QUEUE_SIZE = 10); countIn is `drop` without trace (a missed comparison is accepted for the reference; a flight monitor would use `hook` with a dropped-count channel). Planted-fault classes: threshold_not_set, bump_zero, limit_not_set are stimulus faults (command rewrites reset within the test); threshold_not_saved is the uut-state fault. — amended DEVELOPMENT: SET_LIMIT hook → BUSY, reproduced assert at RustMonitorComponentAc.cpp:845 (11 SET_LIMITs in one 1 Hz period aborted the deployment); SET_LIMIT is now `async … hook` and its Rust overflow hook responds BUSY (anchor ruling 11). Bench (DEVELOPMENT iteration 2): a full RustMonitor drain answers up to 10 commands in one burst and overflowed CdhCore.cmdDisp's 10-deep compCmdStat queue (CommandDispatcherComponentAc.cpp:1132 assert); the reference overrides CdhCoreConfig.QueueSizes.cmdDisp = 20 (ReferenceDeployment/CdhCoreConfig/). 12 commands within ~4 ms still overflow the dispatcher's own command queue (framework default), independent of RustMonitor.
18. Author (round 2): queued dispatch shape — the generated sync/guarded entry of a queued Rust component drains the base queue (dispatchAvailableMessages()) under the one instance lock before constructing &mut Implementation; async entries of queued components take no lock; so no two &mut Implementation are live (fprime-rust SDD §Behavior; UT-SHIM-6, UT-RT-4 Miri).
19. Harness amendment A1 (stage 2, bench-verified 2026-11-10, reviewer to confirm): event names use fth v0.1.2's short form (last two dotted parts, fth.replay.short_name; the engine matches e.short == name): CdhCore.cmdDisp.* -> cmdDisp.*, CdhCore.version.* -> version.*, FileHandling.prmDb.* -> prmDb.*, FileHandling.fileDownlink.* -> fileDownlink.* in markers.event, event_patterns, std_complete, abs, metrics and planted-fault fails. Pass rules unchanged; the record names stay the dictionary's (CdhCore.cmdDisp.NoOpStringReceived). Commands are sent by full dictionary name (fprime-cli refuses suffixes: 'cmdDisp.CMD_NO_OP_STRING is not a known command'), so bench.json deployment is empty and markers.command stays CdhCore.cmdDisp.CMD_NO_OP_STRING.
20. Harness amendment A2 (stage 2, bench-verified 2026-11-10, reviewer to confirm): fprime-gds 4.4.0 stores downlinked files under --file-storage-directory/fprime-downlink/, so the final-state artifact is record/raw/files/fprime-downlink/PrmDb.dat (helper arg path; check text 'PrmDb.dat record 0x10016000 == 42' unchanged). bench.json dictionary path corrected to the built tree's build-artifacts/Linux/FprimeRustReference_ReferenceDeployment/dict/.
21. Harness amendment A3 (stage 2, bench-verified 2026-11-10, reviewer to confirm): the first bench record exported 0 events/0 channels (161/161 event rows, 3957/3957 channel rows unparsed) because fth 0.1.2's exporter reads only TB_PROC_TIME rows while the deployment's Svc.PosixTime stamps TB_WORKSTATION_TIME (fprime-gds prints an ISO date). The fth pin and the deployment are frozen, so the bench recorder is bench/native_recorder.py: fprime-cli events runs unchanged (its output is kept verbatim under record/raw/recorder-native/) and every row is re-rendered from its own (tb(ctx)-sec:usec) stamp into the TB_PROC_TIME row form — same rows, same order, same names, ids and payloads; the fsw clock fth orders by is the row's wall-clock seconds. No pass rule, threshold or fails set changes. Design-stage note for the anchor: the fth time-base assumption vs PosixTime was not caught at design.
22. Harness amendment A4 (stage 2, bench-verified 2026-11-10, reviewer to confirm): with A3 in place every window read UNVERIFIED on the STD predicate `commands_sent == count(event cmdDisp.OpCodeDispatched)` with commands_sent=0: fth 0.1.2 slices a window's commands by comparing the marker command row's first logged argument with the marker id as an integer, and fprime-gds logs the CMD_NO_OP_STRING argument as a string (['20']). The dictionary has no inert command with one integer argument (BUMP, THRESHOLD_PRM_SET and SET_LIMIT all mutate state), so the marker stays CMD_NO_OP_STRING and the bench sender is bench/native_send.py: fprime-cli command-send runs unchanged (its log kept verbatim under record/raw/cmd/send-native/) and the command row is logged with the arguments as given on the command line, decimal integers as integers ([20]); names, opcodes, stamps and every other argument are unchanged. No pass rule, threshold or fails set changes. Recommendation for the anchor: fth should compare marker arguments as text (replay._is_marker_cmd), which would retire A4. Same amendment: `markers.command` is the fth short form cmdDisp.CMD_NO_OP_STRING (fth compares it with the command row's short name — the full name never matched, commands_sent=0 on every window); the sender resolves the short name fth sends to the one dictionary command ending in it, so the marker still goes out as CdhCore.cmdDisp.CMD_NO_OP_STRING.
23. Harness amendment A5 (stage 2, bench-verified 2026-11-10, reviewer to confirm): with A3/A4 in place every window read UNVERIFIED on the plan-wide STD predicate `commands_sent == count(event cmdDisp.OpCodeDispatched)` with count = commands_sent - 1 (record 329bab5d: L0-1 3 vs 2, L2-1 5 vs 4). fth counts OpCodeDispatched rows between the two marker events; on this UUT Svc.CommandDispatcher logs OpCodeDispatched before it queues CMD_NO_OP_STRING to itself, so the START marker's own dispatch row (40 us earlier than NoOpStringReceived) is outside the window and the END marker's is inside — deterministically commands_sent - 1. The predicate is replaced by two per-window predicates with the exact numbers, declared in every window: require `commands_sent == N` (sender log: N = stimulus commands + both markers) and check `event cmdDisp.OpCodeDispatched count == N-1` (the UUT dispatched every command but the START marker, whose row precedes the window). N is fixed per window (planted faults substitute commands one for one, never add or drop), so the checks are at least as strong as the plan-wide predicate; `both markers present` and `RustReference samples >= 2` stay in std_complete. No pass rule, threshold or fails set weakens.
24. Harness amendment A6 (stage 2, bench-verified 2026-11-10, reviewer to confirm): with A5 in place every window read UNVERIFIED on `RustReference samples >= 2` with `RustReference samples=0` (record ff0bcbe8). fth takes the minimum sample count over the packet's channels; Svc.TlmChan downlinks only channels written since its last cycle, and rustMonitor.LastCount, rustMonitor.Exceedances and relay.Relayed are written only when a count arrives or the limit trips (record ff0bcbe8: 13, 1 and 13 samples in 180 s against 190 of each 1 Hz channel), so a window without a BUMP (L0-1, L0-2, L2-4, L4-1, L6-1) can never show two of them. The packet `RustReference` is reduced to the four channels the rate group writes every cycle — rustCounter.Counter, rustCounter.TickCount, rustMonitor.RunCount, rustMonitor.Limit — which is what `samples >= 2` was there to establish (the window brackets at least two telemetry cycles); the event-driven channels keep being judged by the per-test `last(...)`/`delta(...)` checks that name them. No pass rule, threshold or fails set weakens.
25. Harness amendment A7 (stage 2, bench-verified 2026-11-10, reviewer to confirm), record df7d421e, two findings. (a) fth 0.1.2 judges a test's plan `metrics` by text alone at window close (fth/checks.py judge(): PredSpec(name) without the helper binding), so every helper-bound metric raised `PlanError: unparsable predicate` (L0-1, L2-4, L4-1, L7-1 FAIL); moving them to `all_of` is refused too (a test-owned plan must not mix in plan-owned checks: it would lose the stimulus pin and the replay auditor). Helper-bound checks therefore leave the plan's `metrics` lists and are declared by the stimulus with check(text, helper) — same text, helper and args, bound through this plan's helpers module (planted_faults.py), judged at window close and by the auditor's replay of the tests, verbatim in the planted-fault `fails` sets (verify-plan --tests) — and each is documented in the test's `rationale` as a stimulus-declared helper check: L0-1 `version payloads of ProjectVersion, FrameworkVersion, LibraryVersions are non-empty and not UNKNOWN`; L2-3 `relay.Relayed advances by 2`; L2-4 `delta(rustCounter.TickCount) in 4..8`; L2-4 `delta(rustMonitor.RunCount) in 4..8`; L2-5 `rustMonitor.Exceedances advances by 1`; L4-1 `delta(rustCounter.TickCount) in 55..65`; L4-1 `delta(rustMonitor.RunCount) in 55..65`; L6-1 `rustMonitor.Exceedances advances by 0`; L7-1 `PrmDb.dat record 0x10016000 == 42`; L7-1 `relay.Relayed advances by 1`. (b) `delta(ch) == n` is last-minus-first inside the window and relay.Relayed / rustMonitor.Exceedances are change-driven (written only when they move; TlmChan sends only written channels), so the window's first sample already carries the first step: L2-3 `delta(relay.Relayed) == 2` read delta=1 (first=6 last=7: the RESET's relay is the first sample), L2-5 `delta(rustMonitor.Exceedances) == 1` had 1 sample, L6-1 `delta(rustMonitor.Exceedances) == 0` had 0 samples (both UNVERIFIED-class → FAIL on a complete window). These four predicates (also L7-1 `delta(relay.Relayed) == 1`) become the helper `counter_advances(channel, by)` — `relay.Relayed advances by 2` etc.: last sample in the window minus the last sample before the START marker anywhere in the record (boot value 0 when never sent: Relay::m_relayed(0), RustMonitor exceedances: 0); no sample in a complete window is the proof of no write (advance 0). This judges the very increment the rationale describes (RESET and BUMP relay once each) and is not weaker than delta: it still requires the exact advance and a baseline. Planted-fault `fails` sets rename the four predicates verbatim (negative_wrong_limit/limit_not_set/skip_reset keep the same number of fails). No pass rule, threshold or fails set weakens.
26. Harness amendment A8 (stage 2, bench-verified 2026-11-10, reviewer to confirm): L2-4 `delta(rustCounter.TickCount) in 4..8` and `delta(rustMonitor.RunCount) in 4..8` read delta=9 (record 9ecae84d, C1310->C1476 and C1307->C1474) on a window that follows the plan's own procedure — bracket (2 samples), wait 5 s, bracket (2 samples): each bracket adds 1-2 s of 1 Hz samples to the 5 s wait, so the window spans 7-9 s and a 1 Hz rate group must advance 7..9, 10 with one sample of phase jitter. The 4..8 bound was an arithmetic slip against that procedure (the L4-1 analogue 55..65 for a 60 s wait is consistent). Corrected to 7..10 for both: a 4-wide band around the actual window instead of a 5-wide band below it — it rejects a stalled or doubled rate group exactly as the original intended, and is not weaker. Check names change verbatim in the stimulus and rationale; L2-4 has no planted fault.
27. Harness amendment A9 (stage 2, bench-verified 2026-11-10, reviewer to confirm): L7-1 `PrmDb.dat record 0x10016000 == 42` read `CRC32 mismatch: stored 0x4ceb3a87, computed 0xb314c578` on record 9e5eaa07 whose downlinked record/raw/files/fprime-downlink/PrmDb.dat (17 bytes: 4ceb3a87 a5 00000008 10016000 0000002a) is well-formed. Svc.PrmDb (lib/fprime b188d18, Svc/PrmDb/PrmDbImpl.cpp PrmFileSave) writes `~crc.finalize()` of the record bytes with Utils::Hash = Crc32 (init 0xFFFFFFFF): the CRC-32 of the records WITHOUT the final inversion, i.e. zlib.crc32(records) ^ 0xFFFFFFFF = 0x4ceb3a87 exactly. The helper prmdb_record_equals computes that value (the oracle's algorithm corrected to the UUT's documented one; the check still rejects any altered byte of the file, the record format and the value 42 are unchanged). Also fixed in the same helper: the live plugin judges a temporary export of record/raw, so the file is resolved from the pytest argv fth records (--fth-run-dir / --fth-replay-record) with record.path/raw as the first candidate (record 9ecae84d read `absent from record/raw` with the file present). No pass rule, threshold or fails set weakens.
28. Harness amendment A10 (stage 2, bench-verified 2026-11-10, reviewer to confirm): on the first all-PASS record e8a385cf, `fth audit --tests` read DIVERGENCE on 10 of 11 tests (runner PASS, replay FAIL `event cmdDisp.OpCodeDispatched count == N-1` with count N; L2-4/L4-1 tick deltas 12 instead of 9/10). Cause: fth 0.1.2's window slicer scans up to 8 event rows past the END marker for the marker command's own dispatcher rows (OpCodeCompleted/OpCodeDispatched, stopping at the first OpCodeDispatched), built for a synchronous marker component (fth's framTester: Marker, then OpCodeCompleted, then OpCodeDispatched). This UUT's only marker-capable command is Svc.CommandDispatcher's own CMD_NO_OP_STRING, dispatched to itself asynchronously, so the order is OpCodeDispatched, NoOpStringReceived, OpCodeCompleted: the END marker's Dispatched row already precedes its event and the scan instead swallows the NEXT test's START-marker Dispatched row (same opcode), which exists in the replayed record but not yet at live window close. Without an `opCode` argument on those rows fth attributes every dispatcher row to the marker; the plan therefore declares event_patterns for cmdDisp.OpCodeDispatched and cmdDisp.OpCodeCompleted whose `opCode` group is the row's textual token (`Opcode 0x1000001`), never the marker's integer opcode, so no dispatcher row past the END marker is attributed to it and every window ends exactly at its END marker event, live and replayed alike. The per-window `event cmdDisp.OpCodeDispatched count == N-1` predicates (A5) are unchanged: the START marker's row precedes the window, the END marker's row is inside. Known fth 0.1.2 limits recorded for the reviewer, not fixable in the harness: `fth vectors` judges with the engine, which refuses a test-owned plan (checks live in the tests), and the audit's run-start-marker step recognises only `*.Marker` events (seal.marker_ids), so it reads FAILED by construction with the NoOpStringReceived marker although the six run-start marker ids (0xFFFF0001 + manifest sha8, 0xFFFF0002 + seal, 0xFFFF0003 + crc32(run id)) are in every record. No pass rule, threshold or fails set weakens.
29. Harness amendment A11 (stage 2, bench-verified 2026-11-10, reviewer to confirm): on the first planted-fault vector (dry-1, L2-1 bump_wrong_amount, record f4e440f0) every failed predicate appeared twice in the runner's `failed` list, so the list was not byte-equal to the declared `fails` set (fth vectors/unseal compare sorted lists). fth 0.1.2 judges a window's plan `metrics` by name at window close AND every check() the stimulus declares; a predicate present in both is judged twice. The stimulus therefore no longer re-declares the plan's `metrics` strings (49 check() calls removed); it declares only what the plan cannot judge by name — the completeness preconditions (STD, commands_sent, OpCodeDispatched count), the ABS group and the helper-bound checks (A7). The set of judged predicates per window is unchanged (verify-plan --tests still harvests every planted-fault `fails` predicate as declared); no pass rule, threshold or fails set weakens.
30. Harness amendment A12 (stage 2, bench-verified 2026-11-10, reviewer to confirm): vector dry-3 (L5-1 with bump_zero, record 8772425e...) read UNVERIFIED instead of FAIL: the stimulus paced on `bench.wait_event("rustCounter.Saturated")`, an event the planted fault suppresses, fth's wait raised TimeoutError inside the window, the END marker went out from the plugin's finally but THRESHOLD_PRM_SET(100) was never sent, and the completeness precondition `commands_sent == 7` read 6 — an infrastructure UNVERIFIED masking the planted fault's FAIL. The same pattern exists where skip_reset suppresses ResetIssued (L2-1, L7-1) and threshold_not_set suppresses Saturated (L2-2). Those five pacing waits become `bench.wait(2.0)`: send() already waits for the command's OpCodeCompleted/OpCodeDispatched rows (the UUT emits ResetIssued/Bumped/Saturated synchronously in the handler, before completion), so pacing is unchanged for the clean run, and the checks — judged from the record, not from the waits — still require the events (`event rustCounter.Saturated(value=3, threshold=3) count == 1` etc.). No pass rule, threshold or fails set weakens.
31. Harness amendment A13 (stage 2, bench-verified 2026-11-10, reviewer to confirm), vectors dry-7/dry-8 after A12. (a) dry-7 (L7-1 with skip_reset): `last(rustMonitor.LastCount) == 0` read `no samples of rustMonitor.LastCount` and stayed UNVERIFIED-class instead of FAIL, so the runner's failed list lacked one declared fails predicate: fth 0.1.2 promotes a sample-less check to FAIL on a complete window only for checks the stimulus declares (fth/checks.py judge()), not for plan `metrics`. That one predicate moves from L7-1's metrics to a stimulus check(); its text and the fails set are unchanged. (b) dry-8 (L7-1 with threshold_not_saved): `PrmDb.dat record 0x10016000 == 42`, `event fileDownlink.FileSent count == 1` and `event cmdDisp.OpCodeError count == 0` held although PRM_SAVE_FILE was never sent, because the deployment's PrmDb.dat (written in the app's working directory run/FprimeRustReference/) survived from the previous run; bench/native_flash.py now empties the install directory before installing the executable, as bench.json's provenance note already states (fresh PrmDb.dat per run), so the absent-file FAIL the plan requires is reachable. No pass rule, threshold or fails set weakens.
32. Harness amendment A14 (stage 2, bench-verified 2026-11-10, reviewer to confirm). (a) Vector dry-8 (L7-1 with threshold_not_saved, record after A13): the absent-file FAIL now occurs (`PrmDb.dat record 0x10016000 == 42` FAIL, `event fileDownlink.FileSent count == 1` FAIL, `event prmDb.PrmFileSaveComplete(records=1) count == 1` FAIL) but the fourth declared predicate `event cmdDisp.OpCodeError count == 0` held: the record shows FileHandling.fileDownlink.FileOpenError WARNING_HI `Could not open file PrmDb.dat` and no OpCodeError, because Svc.FileDownlink's SendFile is a queued request acknowledged OK before the open. The fault's L7-1 fails entry swaps that predicate for `event fileDownlink.FileOpenError count == 0`, which the L7-1 stimulus now declares as a check (clean record: 0 FileOpenError); the set keeps four predicates, three of them unchanged — nothing that failed before stops failing. (b) Stage-3 rehearsal reh2 (manifest 76fb96c5, record 018de76e): L2-4 diverged, runner PASS `delta=10 (first=87 C1375; last=97 C1546)` vs replay FAIL `delta=11 (first=87 C1375; last=98 C1548)`. The authoritative window (START..END rows of the sealed record) holds eleven 500 ms samples because the live judgment at window close exported the recorder log before fprime-cli had flushed the last channel row; the record, not the live view, is the oracle, and over a 5 s wait at the 2 Hz rate group with the two sample brackets the recorded advance is 10 or 11. `delta(rustCounter.TickCount) in 7..10` and `delta(rustMonitor.RunCount) in 7..10` become `in 7..11` (lower bound and the rate-group pairing unchanged); A8's 7..10 was set from a single observation and is corrected by the sealed record, not loosened to absorb a UUT failure.
