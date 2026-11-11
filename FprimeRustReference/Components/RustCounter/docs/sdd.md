# FprimeRustReference::RustCounter

## Introduction

`RustCounter` is an active component implemented in Rust through `fprime-rust`. It keeps a counter
that commands reset and bump, saturating at a parameter; a rate-group port counts ticks; every command
completion is reported on an output port to a C++ component. It exists to exercise the Rust integration
end to end in the reference deployment.

## Requirements

| ID | Requirement | Verification |
|---|---|---|
| REQ-RustCounter-001 | RESET shall set the counter to zero and emit `ResetIssued`. | Unit Test; Integration Test: L2-1, L7-1 |
| REQ-RustCounter-002 | A BUMP within THRESHOLD shall add its amount to the counter and emit `Bumped` with the new value. | Unit Test; Integration Test: L2-1, L2-3 |
| REQ-RustCounter-003 | A BUMP past THRESHOLD shall set the counter to THRESHOLD and emit `Saturated`, never wrapping. | Unit Test; Integration Test: L2-2, L5-1 |
| REQ-RustCounter-004 | Every RESET or BUMP shall publish `Counter` and send it on `countOut`, changed or not. | Unit Test; Integration Test: L2-1, L2-3, L7-1 |
| REQ-RustCounter-005 | Each `tick` shall increment `TickCount` and publish `TickCount` and `Counter`. | Unit Test; Integration Test: L2-4, L4-1 |
| REQ-RustCounter-006 | A THRESHOLD update shall take effect on the next BUMP. | Unit Test; Integration Test: L2-2, L5-1 |
| REQ-RustCounter-007 | RustCounter shall publish `ThresholdValid`, the validity of THRESHOLD as read by its handler, with every `Counter` sample (RESET, BUMP and tick). | Unit Test |

Command table (REQ-RustCounter-001..003): `RESET`, `BUMP(amount: U32, 0..=4294967295)`. Event table:
`ResetIssued`, `Bumped(value)`, `Saturated(value, threshold)`. Telemetry table: `Counter`, `TickCount`, `ThresholdValid`.
Parameter table: `THRESHOLD` (U32, 0..=4294967295, default 100; 0 pins the counter at 0). `THRESHOLD_PRM_SET/SAVE`
and persistence in `PrmDb.dat` are owned by the autocoded parameter base and `Svc.PrmDb` (plan decision log 11); L7-1
uses them as final-state evidence, not as a RustCounter requirement.

## Design

```
                    +---------------------+
 tick : Svc.Sched ->|                     |-> countOut : FprimeRustReference.Count
                    |     RustCounter     |
                    |      (active)       |
                    +---------------------+
   special ports: time get, command reg/recv/resp, event, text event, telemetry, param get/set
```

| Port | Kind | Type | Purpose |
|---|---|---|---|
| tick | async input | Svc.Sched | rate-group tick, counts ticks |
| countOut | output | FprimeRustReference.Count | report counter after each command |

| Command | Args | Effect |
|---|---|---|
| RESET | — | counter = 0; `ResetIssued`; `Counter`; `countOut` |
| BUMP | amount: U32 | if counter + amount > THRESHOLD (computed with `saturating_add`): counter = THRESHOLD, `Saturated(counter, THRESHOLD)`; else counter += amount, `Bumped(counter)` (amount 0 and the equality case are `Bumped`); then `Counter`; `countOut` |

| Event | Severity | Args |
|---|---|---|
| ResetIssued | activity low | — |
| Bumped | activity low | value: U32 |
| Saturated | warning low | value: U32, threshold: U32 |

| Telemetry | Type | When |
|---|---|---|
| Counter | U32 | every RESET/BUMP and every tick |
| TickCount | U32 | every tick (wraps at U32::MAX) |
| ThresholdValid | Fw.ParamValid | with every `Counter` sample: DEFAULT until THRESHOLD is set, VALID after `THRESHOLD_PRM_SET` (id 2) |

| Parameter | Type | Default |
|---|---|---|
| THRESHOLD | U32 | 100 |

### Configure

Instance: active, queue `Default.QUEUE_SIZE`, stack `Default.STACK_SIZE`, priority 18; `tick` driven by
rate group 1 (1 Hz: `Main.cpp` `startRateGroups(Fw::TimeInterval(1, 0))`, divider 1 in
`FprimeRustReferenceTopology.cpp`). Source files: `RustCounter.fpp`, `RustCounter.rs` only.

### Behavior

At construction counter = 0 and tick count = 0; nothing is published until the first handler call. BUMP
reads THRESHOLD (`paramGet_THRESHOLD`; the FPP default applies when prmDb has no value and the validity is
published as `ThresholdValid` with every report, REQ-RustCounter-007),
computes `counter.saturating_add(amount)`, and chooses `Saturated` (clamping to THRESHOLD, also when
THRESHOLD was lowered below the current counter) or `Bumped`; RESET clears. Each command completion writes
`Counter` and sends the value on `countOut`; each tick increments `TickCount` and writes `TickCount` and
`Counter`. Both commands respond `OK`; there is no rejectable input. All port and command handlers run on the
component's thread; `parameter_updated` (default no-op) would run on the command caller's thread.

```mermaid
sequenceDiagram
  participant G as cmdDisp
  participant R as rustCounter
  participant L as relay
  G->>R: BUMP(amount)
  R->>R: paramGet THRESHOLD; sum = counter.saturating_add(amount)
  R->>G: Bumped(sum) or Saturated(THRESHOLD, THRESHOLD)
  R->>L: countOut(counter)
  R->>G: cmdResponse OK
```

## Tests

<!-- fth:tests:begin -->
_Generated by `fth render-sdd-tests` from the plan — do not edit by hand; the package renderer fails when this table and the plan disagree._

| Test | Purpose | Requirements |
|---|---|---|
| L0-1 | Confirms the running executable reports non-empty project, framework and library version strings for the delivery manifest check. | — |
| L0-2 | Proves the harness tester's MARK command round-trips through the command path (rustTester.Marker) so every later window can be bracketed. | — |
| L2-1 | Resets and bumps the Rust active component to prove async commands dispatch into Rust and events and telemetry come back out. | REQ-FprimeRust-008, REQ-FprimeRust-012, REQ-FprimeRust-013, REQ-FprimeRust-014, REQ-RustCounter-001, REQ-RustCounter-002, REQ-RustCounter-004 |
| L2-2 | Sets THRESHOLD to 5 and bumps past it to prove the Rust handler reads the parameter and saturates on it. | REQ-FprimeRust-015, REQ-RustCounter-003, REQ-RustCounter-006, REQ-RustCounter-007 |
| L2-3 | Bumps the Rust counter and follows the count through the C++ relay into the Rust monitor to prove typed ports in both directions. | REQ-FprimeRust-009, REQ-FprimeRust-010, REQ-FprimeRust-011, REQ-RustCounter-002, REQ-RustCounter-004, REQ-RustMonitor-003, REQ-Relay-001, REQ-Relay-002 |
| L2-4 | Lets the 1 Hz rate group tick the active Rust component and run the queued Rust component to prove C++ scheduler ports into Rust handlers. | REQ-FprimeRust-008, REQ-FprimeRust-009, REQ-FprimeRust-011, REQ-FprimeRust-014, REQ-RustCounter-005, REQ-RustMonitor-005 |
| L2-5 | Sets the monitor limit then bumps past it to prove a queued Rust component handles commands and raises a warning event from a relayed count. | REQ-FprimeRust-008, REQ-FprimeRust-012, REQ-FprimeRust-013, REQ-RustMonitor-001, REQ-RustMonitor-004, REQ-RustMonitor-006, REQ-RustMonitor-003 |
| L4-1 | Runs the deployment for 60 s to prove the Rust components keep servicing rate-group ports with no command errors or counter drift. | REQ-FprimeRust-014, REQ-RustCounter-005, REQ-RustMonitor-005 |
| L5-1 | Drives the counter into THRESHOLD saturation to prove the Rust warning event and the clamp. | REQ-FprimeRust-013, REQ-FprimeRust-015, REQ-RustCounter-003, REQ-RustCounter-006, REQ-RustCounter-007 |
| L6-1 | Sends an invalid limit to prove the Rust handler rejects it with a validation error and changes nothing else. | REQ-FprimeRust-012, REQ-FprimeRust-013, REQ-RustMonitor-002, REQ-RustMonitor-006 |
| L6-2 | Bursts twelve SET_LIMIT commands into the queued Rust monitor within one dispatch to prove the component queue fills at ten and the Rust overflow hook answers BUSY for the rest, with no assert, no silent drop and a later SET_LIMIT accepted. | REQ-FprimeRust-032, REQ-RustMonitor-001, REQ-RustMonitor-007 |
| L7-1 | Saves THRESHOLD 42 into the parameter file, downlinks the file into the record and resets the counter to leave the deployment in an independently verifiable state. | REQ-FprimeRust-010, REQ-FprimeRust-011, REQ-RustCounter-001, REQ-RustCounter-004, REQ-Relay-001, REQ-Relay-002, REQ-RustMonitor-003 |
<!-- fth:tests:end -->

## Unit-test list

| Test | Description | Requirements |
|---|---|---|
| UT-RC-1 | RESET after BUMP(5): `ResetIssued`, `Counter` 0, `countOut(0)`, response OK | REQ-RustCounter-001, REQ-RustCounter-004 |
| UT-RC-2 | THRESHOLD 10: BUMP(4) → `Bumped(4)`; BUMP(6) → `Bumped(10)` (equality); BUMP(1) → `Saturated(10, 10)`, counter 10; BUMP(0) → `Bumped(10)` | REQ-RustCounter-002, REQ-RustCounter-003 |
| UT-RC-3 | counter 1, BUMP(4294967295) → `Saturated(100, 100)`, no panic | REQ-RustCounter-003 |
| UT-RC-4 | THRESHOLD set to 3 while counter is 5: next BUMP(1) → `Saturated(3, 3)`, counter 3 | REQ-RustCounter-006 |
| UT-RC-5 | three `tick` calls: `TickCount` 1, 2, 3 and `Counter` written each time; no `countOut` | REQ-RustCounter-005 |
| UT-RC-6 | `loadParameters` with no stored value then BUMP: `ThresholdValid` DEFAULT; after PRM_SET THRESHOLD and BUMP: `ThresholdValid` VALID | REQ-RustCounter-007 |
