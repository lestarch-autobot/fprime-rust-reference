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

Window rule: rows from the **last** `CdhCore.cmdDisp.NoOpStringReceived(id=START)` row that precedes the first `CdhCore.cmdDisp.NoOpStringReceived(id=END)` row following it, through that END row plus the END `CdhCore.cmdDisp.CMD_NO_OP_STRING` command's own `cmdDisp` rows (at most two), inclusive; ordering key `t = epoch·1e6 + fsw`, archive index on ties. Channel rows are those with `t` between the first and last window event. `commands_sent` = commands from the matching START sending (its last sending if re-sent) through the END sending, inclusive.

Standard `complete` block (STD): both markers present; commands_sent == count(event CdhCore.cmdDisp.OpCodeDispatched); RustReference samples >= 2.
Standard absence block (ABS): event CdhCore.cmdDisp.MalformedCommand count == 0; event CdhCore.cmdDisp.InvalidCommand count == 0; event CdhCore.cmdDisp.TooManyCommands count == 0.
Packets (named channel sets for `<packet> samples >= n`): RustReference = rustCounter.Counter, rustCounter.TickCount, rustMonitor.RunCount, rustMonitor.LastCount, rustMonitor.Exceedances, rustMonitor.Limit, relay.Relayed.

ID:        L0-1
Title:     Image identity
Purpose:   Confirms the running executable reports the project, framework and library versions recorded in the delivery manifest.
Covers:    — (entry gate; image_version policy required)
Setup:     Image installed by flash, GDS attached, ≥ 2 s after start.
Procedure: MARK(10); bracket (2 samples); CdhCore.version.VERSION(ALL); bracket; MARK(11).
Window:    Marker(id=10) … Marker(id=11)
Pass rule: named checks declared in the integration test (test_L0_1). metrics: event CdhCore.version.ProjectVersion count == 1 (the auditor compares the ProjectVersion / LibraryVersions payloads (40-char strings) with the manifest repos[].describe of fprime-rust-reference and fprime-rust); event CdhCore.version.FrameworkVersion count == 1; event CdhCore.cmdDisp.OpCodeError count == 0.
Evidence:  ProjectVersion, FrameworkVersion and LibraryVersions events whose payloads equal the manifest describe strings.
Planted faults:none

ID:        L0-2
Title:     Marker round-trip
Purpose:   Proves the marker command round-trips through the command path so every later window can be bracketed.
Covers:    — (entry gate; harness completeness)
Setup:     As L0-1.
Procedure: MARK(20); bracket (2 samples); MARK(42); bracket; MARK(21).
Window:    Marker(id=20) … Marker(id=21)
Pass rule: named checks declared in the integration test (test_L0_2). metrics: event CdhCore.cmdDisp.NoOpStringReceived count == 3; event CdhCore.cmdDisp.OpCodeError count == 0.
Evidence:  Three NoOpStringReceived events in order (20, 42, 21).
Planted faults:none

ID:        L2-1
Title:     Rust command, event and telemetry paths
Purpose:   Resets and bumps the Rust active component to prove commands dispatch into Rust and events and telemetry come back out.
Covers:    REQ-FprimeRust-008, REQ-FprimeRust-012, REQ-FprimeRust-013, REQ-FprimeRust-014, REQ-FprimeRust-029
Setup:     L0-2 passed; THRESHOLD at default 100.
Procedure: MARK(30); bracket (2 samples); rustCounter.RESET(); rustCounter.BUMP(7); rustCounter.BUMP(5); bracket (2 samples); MARK(31).
Window:    Marker(id=30) … Marker(id=31)
Pass rule: named checks declared in the integration test (test_L2_1). metrics: event rustCounter.ResetIssued count == 1; event rustCounter.Bumped(value=7) count == 1; event rustCounter.Bumped(value=12) count == 1; event rustCounter.ResetIssued precedes event rustCounter.Bumped(value=7); last(rustCounter.Counter) == 12; event rustCounter.Saturated count == 0; event CdhCore.cmdDisp.OpCodeError count == 0.
Evidence:  ResetIssued, Bumped(7), Bumped(12); Counter 12; no Saturated; no command error.
Planted faults:bump_wrong_amount, skip_reset

ID:        L2-2
Title:     Parameter read from Rust
Purpose:   Sets THRESHOLD to 5 and bumps past it to prove the Rust handler reads the parameter and saturates on it.
Covers:    REQ-FprimeRust-015, REQ-FprimeRust-029
Setup:     L2-1 passed.
Procedure: MARK(40); bracket (2 samples); rustCounter.RESET(); rustCounter.THRESHOLD_PRM_SET(5); rustCounter.BUMP(9); rustCounter.THRESHOLD_PRM_SET(100); bracket (2 samples); MARK(41).
Window:    Marker(id=40) … Marker(id=41)
Pass rule: named checks declared in the integration test (test_L2_2). metrics: event rustCounter.Saturated(value=5, threshold=5) count == 1 (THRESHOLD is re-read on every BUMP, so the set value takes effect without a restart); last(rustCounter.Counter) == 5; event rustCounter.Bumped count == 0; event CdhCore.cmdDisp.OpCodeError count == 0.
Evidence:  Saturated(5, 5); Counter 5; no Bumped.
Planted faults:threshold_not_set

ID:        L2-3
Title:     Rust to C++ to Rust port chain
Purpose:   Bumps the Rust counter and follows the count through the C++ relay into the Rust monitor to prove typed ports in both directions.
Covers:    REQ-FprimeRust-009, REQ-FprimeRust-010, REQ-FprimeRust-011
Setup:     L2-1 passed; rustMonitor limit at initial U32::MAX.
Procedure: MARK(50); bracket (2 samples); rustCounter.RESET(); rustCounter.BUMP(3); bracket (2 samples); MARK(51).
Window:    Marker(id=50) … Marker(id=51)
Pass rule: named checks declared in the integration test (test_L2_3). metrics: event rustCounter.Bumped(value=3) count == 1; delta(relay.Relayed) == 2 (RESET and BUMP each send countOut once: Rust→C++ counted by the C++ relay, C++→Rust seen as LastCount); last(rustMonitor.LastCount) == 3; event rustMonitor.LimitExceeded count == 0; event CdhCore.cmdDisp.OpCodeError count == 0.
Evidence:  Bumped(3); Relayed +2; LastCount 3.
Planted faults:bump_wrong_amount

ID:        L2-4
Title:     Rate-group ports into Rust
Purpose:   Lets the 1 Hz rate group tick the active Rust component and run the queued Rust component to prove C++ scheduler ports into Rust handlers.
Covers:    REQ-FprimeRust-008, REQ-FprimeRust-009, REQ-FprimeRust-011, REQ-FprimeRust-014
Setup:     L0-2 passed.
Procedure: MARK(60); bracket (2 samples); wait 5 s; bracket (2 samples); MARK(61).
Window:    Marker(id=60) … Marker(id=61)
Pass rule: named checks declared in the integration test (test_L2_4). metrics: event CdhCore.cmdDisp.OpCodeError count == 0; delta(rustCounter.TickCount) in 4..8 (rate group 1 runs at 1 Hz; 5 s window plus bracket sampling admits 4..8 ticks); delta(rustMonitor.RunCount) in 4..8.
Evidence:  TickCount and RunCount each advance 4..8 over the window.
Planted faults:none

ID:        L2-5
Title:     Queued Rust command and limit exceedance
Purpose:   Sets the monitor limit then bumps past it to prove a queued Rust component handles commands and raises a warning event from a relayed count.
Covers:    REQ-FprimeRust-008, REQ-FprimeRust-012, REQ-FprimeRust-013
Setup:     L2-3 passed.
Procedure: MARK(70); bracket (2 samples); rustMonitor.SET_LIMIT(4); rustCounter.RESET(); rustCounter.BUMP(10); wait 2 s; rustMonitor.SET_LIMIT(4294967295); bracket (2 samples); MARK(71).
Window:    Marker(id=70) … Marker(id=71)
Pass rule: named checks declared in the integration test (test_L2_5). metrics: event rustMonitor.LimitSet(limit=4) count == 1; event rustMonitor.LimitExceeded(count=10, limit=4) count == 1 (the count 10 reaches the queued component on the next run, within the 2 s wait); delta(rustMonitor.Exceedances) == 1; event rustMonitor.LimitSet count == 2; last(rustMonitor.Limit) == 4294967295; event CdhCore.cmdDisp.OpCodeError count == 0.
Evidence:  LimitSet(4), LimitExceeded(10, 4), Exceedances +1, limit restored.
Planted faults:limit_not_set

ID:        L4-1
Title:     Endurance 60 s
Purpose:   Runs the deployment for 60 s to prove the Rust components keep servicing rate-group ports with no command errors or counter drift.
Covers:    REQ-FprimeRust-014, REQ-FprimeRust-029
Setup:     L2-4 passed.
Procedure: MARK(80); bracket (2 samples); wait 60 s; bracket (2 samples); MARK(81).
Window:    Marker(id=80) … Marker(id=81)
Pass rule: named checks declared in the integration test (test_L4_1). metrics: delta(rustCounter.Counter) == 0; event rustCounter.Saturated count == 0; event CdhCore.cmdDisp.OpCodeError count == 0; delta(rustCounter.TickCount) in 55..4294967295 (1 Hz over 60 s with scheduler jitter margin); delta(rustMonitor.RunCount) in 55..4294967295.
Evidence:  ≥ 55 ticks and runs; Counter unchanged.
Planted faults:none

ID:        L5-1
Title:     Saturation fault path
Purpose:   Drives the counter into THRESHOLD saturation to prove the Rust warning event and the clamp.
Covers:    REQ-FprimeRust-013, REQ-FprimeRust-015
Setup:     L2-2 passed.
Procedure: MARK(90); bracket (2 samples); rustCounter.RESET(); rustCounter.THRESHOLD_PRM_SET(3); rustCounter.BUMP(2); rustCounter.BUMP(2); rustCounter.THRESHOLD_PRM_SET(100); bracket (2 samples); MARK(91).
Window:    Marker(id=90) … Marker(id=91)
Pass rule: named checks declared in the integration test (test_L5_1). metrics: event rustCounter.Bumped(value=2) count == 1; event rustCounter.Saturated(value=3, threshold=3) count == 1; last(rustCounter.Counter) == 3; event rustCounter.Bumped(value=2) precedes event rustCounter.Saturated(value=3, threshold=3); event CdhCore.cmdDisp.OpCodeError count == 0.
Evidence:  Bumped(2) then Saturated(3, 3); Counter 3.
Planted faults:bump_zero

ID:        L6-1
Title:     Negative control: SET_LIMIT(0) rejected
Purpose:   Sends an invalid limit to prove the Rust handler rejects it with a validation error and changes nothing else.
Covers:    REQ-FprimeRust-012, REQ-FprimeRust-013
Setup:     L2-5 passed; limit at U32::MAX.
Procedure: MARK(100); bracket (2 samples); rustMonitor.SET_LIMIT(0); bracket (2 samples); MARK(101).
Window:    Marker(id=100) … Marker(id=101)
Pass rule: named checks declared in the integration test (test_L6_1). metrics: event rustMonitor.InvalidLimit(limit=0) count == 1; event CdhCore.cmdDisp.OpCodeError(error=VALIDATION_ERROR) count == 1 (negative control: the rejection is visible at the dispatcher and nothing else happened); event rustMonitor.LimitSet count == 0; delta(rustMonitor.Limit) == 0; delta(rustMonitor.Exceedances) == 0; event rustCounter.Bumped count == 0.
Evidence:  InvalidLimit(0), one OpCodeError(VALIDATION_ERROR), Limit and Exceedances unchanged.
Planted faults:negative_wrong_limit

ID:        L7-1
Title:     Final state
Purpose:   Resets the counter and reads the installed executable back out of band to leave the deployment in an independently verifiable state.
Covers:    REQ-FprimeRust-010, REQ-FprimeRust-011, REQ-FprimeRust-029
Setup:     All earlier tests run.
Procedure: MARK(110); bracket (2 samples); rustCounter.RESET(); bracket (2 samples); MARK(111); action: bench dump reads the installed executable back to {readback}.
Window:    Marker(id=110) … Marker(id=111)
Pass rule: named checks declared in the integration test (test_L7_1). metrics: event rustCounter.ResetIssued count == 1; last(rustCounter.Counter) == 0; delta(relay.Relayed) == 1 (the reset count 0 travels Rust→C++→Rust one more time); event CdhCore.cmdDisp.OpCodeError count == 0.
Evidence:  ResetIssued; Counter 0; Relayed +1; readback sha256 equals the manifest image.
Planted faults:skip_reset

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
| REQ-FprimeRust-008 | Integration Test: L2-1, L2-4, L2-5 | Integration Test |
| REQ-FprimeRust-009 | Integration Test: L2-3, L2-4 | Integration Test |
| REQ-FprimeRust-010 | Integration Test: L2-3, L7-1 | Integration Test |
| REQ-FprimeRust-011 | Integration Test: L2-3, L2-4, L7-1 | Integration Test |
| REQ-FprimeRust-012 | Integration Test: L2-1, L2-5, L6-1 | Integration Test |
| REQ-FprimeRust-013 | Integration Test: L2-1, L2-5, L5-1, L6-1 | Integration Test |
| REQ-FprimeRust-014 | Integration Test: L2-1, L2-4, L4-1 | Integration Test |
| REQ-FprimeRust-015 | Integration Test: L2-2, L5-1 | Integration Test |
| REQ-FprimeRust-016 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-017 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-018 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-019 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-020 | waived: Inspection | Inspection |
| REQ-FprimeRust-021 | waived: Inspection | Inspection |
| REQ-FprimeRust-022 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-023 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-024 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-025 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-026 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-027 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-028 | waived: Unit Test | Unit Test |
| REQ-FprimeRust-029 | Integration Test: L2-1, L2-2, L4-1, L7-1 | Integration Test |

## 5 Metrics
- L0-1: event CdhCore.version.ProjectVersion count == 1; event CdhCore.version.FrameworkVersion count == 1; event CdhCore.cmdDisp.OpCodeError count == 0
- L0-2: event CdhCore.cmdDisp.NoOpStringReceived count == 3; event CdhCore.cmdDisp.OpCodeError count == 0
- L2-1: event rustCounter.ResetIssued count == 1; event rustCounter.Bumped(value=7) count == 1; event rustCounter.Bumped(value=12) count == 1; event rustCounter.ResetIssued precedes event rustCounter.Bumped(value=7); last(rustCounter.Counter) == 12; event rustCounter.Saturated count == 0; event CdhCore.cmdDisp.OpCodeError count == 0
- L2-2: event rustCounter.Saturated(value=5, threshold=5) count == 1; last(rustCounter.Counter) == 5; event rustCounter.Bumped count == 0; event CdhCore.cmdDisp.OpCodeError count == 0
- L2-3: event rustCounter.Bumped(value=3) count == 1; delta(relay.Relayed) == 2; last(rustMonitor.LastCount) == 3; event rustMonitor.LimitExceeded count == 0; event CdhCore.cmdDisp.OpCodeError count == 0
- L2-4: event CdhCore.cmdDisp.OpCodeError count == 0; delta(rustCounter.TickCount) in 4..8; delta(rustMonitor.RunCount) in 4..8
- L2-5: event rustMonitor.LimitSet(limit=4) count == 1; event rustMonitor.LimitExceeded(count=10, limit=4) count == 1; delta(rustMonitor.Exceedances) == 1; event rustMonitor.LimitSet count == 2; last(rustMonitor.Limit) == 4294967295; event CdhCore.cmdDisp.OpCodeError count == 0
- L4-1: delta(rustCounter.Counter) == 0; event rustCounter.Saturated count == 0; event CdhCore.cmdDisp.OpCodeError count == 0; delta(rustCounter.TickCount) in 55..4294967295; delta(rustMonitor.RunCount) in 55..4294967295
- L5-1: event rustCounter.Bumped(value=2) count == 1; event rustCounter.Saturated(value=3, threshold=3) count == 1; last(rustCounter.Counter) == 3; event rustCounter.Bumped(value=2) precedes event rustCounter.Saturated(value=3, threshold=3); event CdhCore.cmdDisp.OpCodeError count == 0
- L6-1: event rustMonitor.InvalidLimit(limit=0) count == 1; event CdhCore.cmdDisp.OpCodeError(error=VALIDATION_ERROR) count == 1; event rustMonitor.LimitSet count == 0; delta(rustMonitor.Limit) == 0; delta(rustMonitor.Exceedances) == 0; event rustCounter.Bumped count == 0
- L7-1: event rustCounter.ResetIssued count == 1; last(rustCounter.Counter) == 0; delta(relay.Relayed) == 1; event CdhCore.cmdDisp.OpCodeError count == 0

## 6 Fault menu
Generated from `planted_faults.py` planted fault declarations (`fth planted-faults-menu`): bump_wrong_amount (harness-expectation; L2-1, L2-3); bump_zero (uut-state; L5-1); limit_not_set (uut-state; L2-5); negative_wrong_limit (stimulus; L6-1); skip_reset (stimulus; L2-1, L7-1); threshold_not_set (uut-state; L2-2).

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
8. Author (2026-11-10): REQ-FprimeRust-018 (one archive) is verified by a build unit test (one libfprime_rust_<Deployment>.a on the link line, one copy of the Rust runtime symbols), not by a system test; L0-1 is an entry gate.
9. Author (2026-11-10): max planted faults per run = 2; menu holds 6 faults ≥ 2×2; every system-covered requirement has ≥2 covering tests with disjoint planted-fault sets.
