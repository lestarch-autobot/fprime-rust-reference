# FprimeRustReference::RustMonitor

## Introduction

`RustMonitor` is a queued component implemented in Rust through `fprime-rust`. It receives counts
relayed by the C++ `Relay` component, compares them with a commanded limit, and publishes telemetry
when run from a rate group. It exercises the queued kind and its queue dispatch from Rust, a sync and an
async typed input port, a rejectable command, and events of two severities.

## Requirements

| ID | Requirement | Verification |
|---|---|---|
| REQ-RustMonitor-001 | SET_LIMIT with a non-zero limit shall store it, emit `LimitSet` and publish `Limit`. | Unit Test; Integration Test: L2-5 |
| REQ-RustMonitor-002 | SET_LIMIT with limit zero shall respond VALIDATION_ERROR, emit `InvalidLimit` and leave the limit unchanged. | Unit Test; Integration Test: L6-1 |
| REQ-RustMonitor-003 | Each count received shall be published on `LastCount`. | Unit Test; Integration Test: L2-3, L2-5, L7-1 |
| REQ-RustMonitor-004 | A count greater than the limit shall emit `LimitExceeded` and increment and publish `Exceedances`. | Unit Test; Integration Test: L2-5 |
| REQ-RustMonitor-005 | Each `run` shall deliver every queued count and command to its handler, then increment and publish `RunCount` and publish `Limit`. | Unit Test; Integration Test: L2-4, L4-1 |
| REQ-RustMonitor-006 | Until SET_LIMIT is accepted, the limit shall be 4294967295. | Unit Test; Integration Test: L2-5, L6-1 |
| REQ-RustMonitor-007 | SET_LIMIT received while the component queue is full shall be answered BUSY by the overflow hook, without assert or silent drop. | Unit Test |

Command table: `SET_LIMIT(limit: U32, 1..=4294967295)`. Event table: `LimitSet(limit)`, `InvalidLimit(limit)`,
`LimitExceeded(count, limit)`. Telemetry table: `RunCount`, `LastCount`, `Exceedances`, `Limit`.
Constants: initial limit `U32::MAX` = 4294967295 (REQ-RustMonitor-006).

## Design

```
                                    +---------------------+
 run : Svc.Sched ------------------>|                     |
 countIn : FprimeRustReference.Count|     RustMonitor     |
   (async, drop) ------------------>|      (queued)       |
                                    +---------------------+
   special ports: time get, command reg/recv/resp, event, text event, telemetry
```

| Port | Kind | Type | Purpose |
|---|---|---|---|
| run | sync input | Svc.Sched | queue drained by the generated entry; publish RunCount and Limit |
| countIn | async input, `drop` on queue full | FprimeRustReference.Count | count from relay |

| Command | Args | Effect |
|---|---|---|
| SET_LIMIT | limit: U32 | limit > 0: store, `LimitSet`, `Limit`, OK; limit = 0: `InvalidLimit`, VALIDATION_ERROR; queue full: BUSY (overflow hook) |

| Event | Severity | Args |
|---|---|---|
| LimitSet | activity low | limit: U32 |
| InvalidLimit | warning low | limit: U32 |
| LimitExceeded | warning high | count: U32, limit: U32 |

| Telemetry | Type | When |
|---|---|---|
| RunCount | U32 | every run (wraps at U32::MAX) |
| LastCount | U32 | every count |
| Exceedances | U32 | every exceedance (wraps at U32::MAX) |
| Limit | U32 | every run and every SET_LIMIT accepted |

Parameters: none.

### Configure

Instance: queued, queue `Default.QUEUE_SIZE` (10); `run` driven by rate group 1 (1 Hz, see RustCounter
§Configure). A queue-full count is dropped without report (F Prime counts it in `getNumMsgsDropped()` but emits nothing); the
reference accepts a missed comparison because counts arrive only from RustCounter commands and the GDS command
rate stays below the 10-per-run queue depth; a flight monitor would publish `ctx.num_msgs_dropped()` each run or
use `hook` with a dropped-count channel.
`SET_LIMIT` is declared `hook`: on a full queue (10 queued `SET_LIMIT`s plus counts within one 1 Hz run) its overflow hook
answers `BUSY` on the caller's thread — no assert, no silent drop (plan decision log 17, amended by DEVELOPMENT ruling 11 after
the `assert` was reproduced at `RustMonitorComponentAc.cpp:845` with 11 commands in one period). Source files: `RustMonitor.fpp`,
`RustMonitor.rs` only.

### Behavior

At construction limit = U32::MAX and all counts are 0. Counts and commands queue until `run`: the generated
`run` entry drains the queue (`dispatchAvailableMessages()`, fprime-rust §Behavior) so the queued `countIn` and
`SET_LIMIT` handlers run on the rate-group thread before `run_handler` is entered; all Rust state is therefore
touched on one thread and only one `&mut` is ever live. A dispatched count is
published as `LastCount`; when it exceeds the limit, `LimitExceeded` is emitted and `Exceedances`
incremented and published. SET_LIMIT validates non-zero before storing. After dispatch `run` increments and
publishes `RunCount` and publishes `Limit`.

```mermaid
sequenceDiagram
  participant RG as rateGroup1
  participant L as relay
  participant M as rustMonitor
  L->>M: countIn(count) (queued)
  RG->>M: run
  M->>M: generated run entry drains queue: countIn → LastCount; if count > limit: LimitExceeded, Exceedances
  M->>M: RunCount += 1; publish RunCount, Limit
```

## Tests

<!-- fth:tests:begin -->
_Generated by `fth render-sdd-tests` from the plan — do not edit by hand; the package renderer fails when this table and the plan disagree._

| Test | Purpose | Requirements |
|---|---|---|
| L0-1 | Confirms the running executable reports non-empty project, framework and library version strings for the delivery manifest check. | — |
| L0-2 | Proves the marker command round-trips through the command path so every later window can be bracketed. | — |
| L2-1 | Resets and bumps the Rust active component to prove async commands dispatch into Rust and events and telemetry come back out. | REQ-FprimeRust-008, REQ-FprimeRust-012, REQ-FprimeRust-013, REQ-FprimeRust-014, REQ-RustCounter-001, REQ-RustCounter-002, REQ-RustCounter-004 |
| L2-2 | Sets THRESHOLD to 5 and bumps past it to prove the Rust handler reads the parameter and saturates on it. | REQ-FprimeRust-015, REQ-RustCounter-003, REQ-RustCounter-006 |
| L2-3 | Bumps the Rust counter and follows the count through the C++ relay into the Rust monitor to prove typed ports in both directions. | REQ-FprimeRust-009, REQ-FprimeRust-010, REQ-FprimeRust-011, REQ-RustCounter-002, REQ-RustCounter-004, REQ-RustMonitor-003, REQ-Relay-001, REQ-Relay-002 |
| L2-4 | Lets the 1 Hz rate group tick the active Rust component and run the queued Rust component to prove C++ scheduler ports into Rust handlers. | REQ-FprimeRust-008, REQ-FprimeRust-009, REQ-FprimeRust-011, REQ-FprimeRust-014, REQ-RustCounter-005, REQ-RustMonitor-005 |
| L2-5 | Sets the monitor limit then bumps past it to prove a queued Rust component handles commands and raises a warning event from a relayed count. | REQ-FprimeRust-008, REQ-FprimeRust-012, REQ-FprimeRust-013, REQ-RustMonitor-001, REQ-RustMonitor-004, REQ-RustMonitor-006, REQ-RustMonitor-003 |
| L4-1 | Runs the deployment for 60 s to prove the Rust components keep servicing rate-group ports with no command errors or counter drift. | REQ-FprimeRust-014, REQ-RustCounter-005, REQ-RustMonitor-005 |
| L5-1 | Drives the counter into THRESHOLD saturation to prove the Rust warning event and the clamp. | REQ-FprimeRust-013, REQ-FprimeRust-015, REQ-RustCounter-003, REQ-RustCounter-006 |
| L6-1 | Sends an invalid limit to prove the Rust handler rejects it with a validation error and changes nothing else. | REQ-FprimeRust-012, REQ-FprimeRust-013, REQ-RustMonitor-002, REQ-RustMonitor-006 |
| L7-1 | Saves THRESHOLD 42 into the parameter file, downlinks the file into the record and resets the counter to leave the deployment in an independently verifiable state. | REQ-FprimeRust-010, REQ-FprimeRust-011, REQ-RustCounter-001, REQ-RustCounter-004, REQ-Relay-001, REQ-Relay-002, REQ-RustMonitor-003 |
<!-- fth:tests:end -->

## Unit-test list

| Test | Description | Requirements |
|---|---|---|
| UT-RM-1 | SET_LIMIT(4) then `run`: `LimitSet(4)`, `Limit` 4, response OK | REQ-RustMonitor-001 |
| UT-RM-2 | SET_LIMIT(0) then `run`: `InvalidLimit(0)`, response VALIDATION_ERROR, `Limit` unchanged (4294967295) | REQ-RustMonitor-002, REQ-RustMonitor-006 |
| UT-RM-3 | countIn(3) then `run` with limit 4: `LastCount` 3, no `LimitExceeded`; countIn(10) then `run`: `LimitExceeded(10, 4)`, `Exceedances` 1 | REQ-RustMonitor-003, REQ-RustMonitor-004 |
| UT-RM-4 | two counts queued, one `run`: both handled before `run_handler`, `RunCount` 1, `Limit` published; `run` with an empty queue: `RunCount` 2 | REQ-RustMonitor-005 |
| UT-RM-5 | ten `SET_LIMIT`s queued without dispatch fill the queue; the eleventh is answered BUSY by the overflow hook on the caller's thread, no assert; a following `run` dispatches the ten and the last limit is in effect | REQ-RustMonitor-007, REQ-FprimeRust-032 |
