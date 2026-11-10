# Review dispositions — DEVELOPMENT iteration 1 (anchor rulings)

Local review of `campaign/dev` (10 lenses, nothing posted to GitHub) raised 13 plan-change requests; the campaign
anchor ruled as below. Each ruling is applied on `campaign/dev` of fprime-rust and fprime-rust-reference; SDD and
plan edits are design amendments carried by the DEVELOPMENT child.

| # | request | ruling | applied in |
|---|---|---|---|
| 1 | UT-SHIM-6 expects `last_dispatch() == Empty` | ACCEPTED → `MSG_DISPATCH_OK` (verified: `Fw/Comp/QueuedComponentBase.cpp:47-57` returns the last status, OK after a drain) | `docs/sdd.md` UT-SHIM-6 row, `Context::last_dispatch()` doc |
| 2 | UT-INSP-1 "three platform tags and no other" | ACCEPTED → "at least the three tags" | `docs/sdd.md` UT-INSP-1 row |
| 3 | UT-RT-4 Miri over the queued fixture | ACCEPTED → "Miri over the runtime crate's safe wrappers" | `docs/sdd.md` UT-RT-4 row |
| 4 | alloc-error handler "aborts the same way" | ACCEPTED → "allocation failure aborts via std; no F Prime hook (stable Rust has no alloc_error_handler)" | `docs/sdd.md` §Behavior |
| 5 | R4 `native-static-libs` link list | ACCEPTED as a fixed per-platform list (Linux: `pthread dl m gcc_s c`; other platforms override through the `FPRIME_RUST_NATIVE_LIBS` cache variable); configure-time probe = future work | `docs/sdd.md` R4, `cmake/FprimeRust.cmake` |
| 6 | queue-full hook creates a second `&mut Implementation` / self-deadlock | DESIGN CHANGE: every overflow hook (`hook` ports, `hook` commands, internal-port hooks) is `fn X_overflow_hook(ctx: &mut Context, …)` — no `&mut self`, no lock; Context only (events, telemetry, `BUSY`) | `docs/sdd.md` §Behavior, name table, UT-SHIM-9; generator (`cpp_shim.py`, `rust_base.py`, `rust_template.py`); fixtures; regression test `tests/test_autocode.py::test_overflow_hooks_never_borrow_the_implementation` |
| 7 | REQ-024 panic message cannot reach the assertion hook | ACCEPTED → "A Rust panic shall reach the F Prime assertion hook with its file and line, its message logged via Fw::Logger, then abort." | `docs/sdd.md` requirements + UT-RT-3; reference `plan.json` requirement text |
| 8 | allocation-free claim | ACCEPTED relaxed → "generated non-string paths do not allocate; string parameter gets and string port returns build a bounded `String`" | `docs/sdd.md` §Behavior |
| 9 | unsized `string` parameters/returns and `ref` rejected | ACCEPTED | `docs/sdd.md` type table + rejected table |
| 10 | runtime marker is a C++ symbol | MOVE into the runtime crate (`#[no_mangle] pub extern "C" fn fprime_rust_runtime_marker()`); UT-BUILD-1 wording kept | `runtime/src/lib.rs`, `FprimeRust/FprimeRust.cpp` |
| 11 | RustMonitor `SET_LIMIT` asserts on a full queue (reproduced: `RustMonitorComponentAc.cpp:845`) | DESIGN CHANGE: `async command SET_LIMIT … hook`; the hook responds `BUSY` | reference `RustMonitor.fpp`, `RustMonitor.rs`, `docs/sdd.md`, README, `plan.json` decision 17 (amended) |
| 12 | D-01 waiver parity with fprime-python | ANCHOR RULING (Captain ruling R1): waiver stands; deviations recorded in `docs/sdd.md` §Deviations from fprime-python | `docs/sdd.md` |
| 13 | `.gitmodules` `branch = main` | KEEP | — |
| extra | `fprime-cpp-codegen` declared but unused (Captain ruling R5) | RULED: the C++ shim is emitted through `fprime_cpp_codegen` (`Body` / `CppDocBuilder` / `Output`); byte-identical output not required, compile + UT-SHIM-* passing is | `src/fprime_rust/autocode/cpp_shim.py` |
| extra | UT-BUILD-2 false negative when `lib/fprime` is not initialized | RULED: skip with a named reason when the framework path is absent | `tests/test_build.py` |

## Iteration 2 (delta review of the rulings)

| finding | lens | disposition |
|---|---|---|
| D2-1 hooks received the full `Context` (output/internal ports reachable; recursion/self-deadlock) | design | fixed: hooks take `&mut HookContext` (events + telemetry only); SDD name table/§Behavior updated |
| D2-2 no RustMonitor requirement for BUSY-on-full-queue; stale plan notes | design | fixed: REQ-RustMonitor-007 (SDD + plan.json), UT-RM-5 traces to it; plan `not_covered`/REQ-032 notes updated |
| D2-3 plan decision 11 still says the alloc-error handler aborts | design | fixed: amended per ruling 4 |
| D2-4 Linux native list dropped `rt util` | design | fixed: `pthread dl m rt util gcc_s c` (what `rustc --print native-static-libs` reports); SDD R4 updated |
| D2-5 `last_dispatch()` doc still promised `Empty` | design | fixed |
| D2-6 R5 partly applied: `extern "C"` blocks and helper definitions pass through `doc.lines` | design | accepted: fprime-cpp-codegen 0.2.0 has no C-linkage scope builder; `doc.lines` is its escape hatch; class, methods, members and includes are built through the library |
| iteration-2 fprime-code: `U32` loop index against `FwSizeType` queue depth in `RustMonitorTester.cpp` (CPP-3/28) | fprime-code | fixed |
| iteration-2 fprime-code observation: `libFprimeRust.a` path appended after the Rust archive without re-listing `Fw_Types`/`Fw_Logger` | fprime-code | accepted: those objects are already pulled by the shim and framework objects earlier on the line; all fixtures and the reference link; a configure-time probe stays future work |
| iteration-2 bench: a full RustMonitor drain (10 BUSY-free completions in one burst) overflowed `CdhCore.cmdDisp`'s 10-deep queue → `CommandDispatcherComponentAc.cpp:1132` assert | development bench | fixed in the reference: `CdhCoreConfig.QueueSizes.cmdDisp = 20` override (`ReferenceDeployment/CdhCoreConfig/`), plan decision 17 and RustMonitor SDD note it |
| iteration-2 architecture suggestion: SDD called `hook` "the safe queue-full policy" while the generated port-hook stub drops silently | architecture | fixed: wording qualified (non-blocking; the user's hook must report the loss) |
| iteration-2 correctness C2-1: `FPRIME_RUST_NATIVE_LIBS` docstring said "linked after the Rust archive" while the link line places them before the archive (harmless on glibc) | correctness | fixed: docstring corrected; ordering unchanged (preexisting, g++ re-adds the libc set) |
| iteration-2 security (future work / design, pre-existing): a full RustMonitor drain emits 10 `cmdResponse_out` in ~100 µs and overflowed `CdhCore.cmdDisp`'s 10-deep queue in 1 of 3 paced runs (`CommandDispatcherComponentAc.cpp:1132`) | security | fixed in the reference (same finding as the development bench row above): `CdhCoreConfig.QueueSizes.cmdDisp = 20`; F1 and F2 of iteration 1 re-verified fixed by the reviewer (bench BUSY ×2, Miri clean on the tip) |

## DEVELOPMENT step 8 re-entry (D1–D4): local delta review, four lenses

Delta reviewed: UUT 9ecd1f9..campaign/dev (fixtures, runtime-enum generator fix) and reference c069f0e..campaign/dev
(RustTester, coverage twin, RustCounter `ThresholdValid`). Local only; nothing posted. must-fix = fixed unless noted.

### Design lens (No-Go → fixed)
1. Window cap derived from the wrong queue (events queue 10, drop policy): FIXED — `CdhCoreConfig.QueueSizes.events = 40`
   (17-event worst case documented in the RustTester SDD); plan delta: check `EventsDropped` in the dump plan.
2. REQ-RustTester-007 false (autocode instrumented, ctor/dtor not attributed): FIXED — requirement narrowed to the
   hand-written `RustTester.cpp` (REQ-011), ctor/dtor attributed, SDD states the autocode is instrumented and counted
   with the third gcovr filter — harness autocode in the judged figure, flagged for anchor ratification.
3. Integration-test traceability ahead of the plan: FIXED — RustTester/RustCounter-007 verification cells trimmed to
   `Unit Test`; the plan deltas are reported to the harness child as text (test/int is not this child's scope).
4. REQ-RustCounter-007 vs tick: FIXED — `tick_handler` publishes `ThresholdValid` with `Counter` too; telemetry table.
5. CPP-37 silent reset/rejections: FIXED — `CoverageReset(objects)` id 5, `CoverageRejected(reason: DumpReject)` id 6
   (`NO_SNAPSHOT`, `OVERFLOW`); requirements/UT-RT-5/6 updated. Contract events untouched.
6. Repeated-window identity untested: FIXED — UT-RT-4 re-reads a window after 50 `MARK`s.
7. Snapshot overflow without requirement: FIXED — REQ-RustTester-007 (Inspection).
8. SDD shape: FIXED — `ID | Requirement | Verification`, one sentence per row, cells `Unit Test`/`Inspection`, no-ports line.
9. GCC floor reason: FIXED — the five-argument `__gcov_info_to_gcda` is GCC ≥ 12; host GCC 11 libgcov lacks it.
10. "static 512 KiB" / `-Waddress`: FIXED — member wording, `coverageAvailable()` under `#if`.
11. `CmdResponse` repr type: FIXED (see correctness 1).
12. fth exclude option: future work (harness). 13. `.gcda` merge at exit: FIXED in the SDD (delete before a run).

### F Prime code lens (No-Go → fixed)
1. `Fw.CmdResponse : U8`, `CLEARED`/`DISPATCHED_UNTRACKED`: FIXED — `repr() -> u8`, `from_repr(u8)`, both variants;
   `tests/test_cli.py` CmdResponse case; `ActiveFix` fixture gains `T_RESP: Fw.CmdResponse` so the mapping compiles in
   `fprime-util check` (UUT non-fixture change, reported).
2. Template default for runtime enums (`ParamValid::UNINIT`): FIXED — `EnumInfo.variant_name()` (UpperCamel for
   runtime enums) + `test_runtime_enum_defaults_use_runtime_variant_names`.
3. UT-RT-6 could pass with a no-op zero: FIXED — walk 1, zero, walk A, zero, walk B: A == B, A ≤ walk 1, CRC differs.
4. Reset/rejection events: FIXED (design 5). 5. Queue bound: FIXED (design 1). 6. No-profile constraint vs autocode:
   doc waiver (design 2); compiling the Ac with `-fno-profile-arcs` not done — the UT twin instruments the test
   executable on purpose so UT-RT-4..6 have moving counters.
7. White-box tests in the default build: FIXED — `friend class RustTesterTester`, UT-RT-7 (crc32 KAT), UT-RT-8
   (injected snapshot: packing, padding, 32-multiple object, Done).
8. Oracle on object boundaries: FIXED (scan all `CoverageObject` events). 9. Uninitialised locals / narrowing /
   `nameCrc`: FIXED. 10. `alignas(8) m_arena`: FIXED. 11. SDD exit-write contradiction: FIXED.
12. REQ-RustCounter-007 wording/table: FIXED. 13. ActiveFix trace: FIXED (REQ-FprimeRust-006); UT-SHIM rows in the
   UUT SDD and a non-zero drop case: not done (outside tests/project; non-drop ports assert on overflow in a UT).
14. GNU ≥ 12 check / CI twin job: future work.

### Correctness lens (No-Go → fixed)
1. `CmdResponse` repr/u8 + variants: FIXED (code lens 1). 2. walkAll boundary oracle: FIXED (code lens 8).
3. crc32 KAT + `nameCrc != 0`: FIXED (UT-RT-7, walkAll). 4. UT-RT-5 proves invalidation: FIXED (window served before
   the zero, rejected after). 5. `RUST_TESTER_NO_PROFILE` unconditional: declined — harmless in OFF (no profile
   flags), and the F Prime UT coverage figure of harness code is not a judged metric. 6. Re-snapshot on every
   `start = 0`: declined — a stale snapshot would silently serve old counters; SDD now forbids retrying window 0
   mid-walk. 7. Exit-time `.gcda`: FIXED (“may write”, only with libgcov's exit handler linked).
8. Fixture with runtime-enum types: FIXED (ActiveFix `T_RESP`). 9. DEFER ordering note: FIXED (function header).

### Security lens (Go, with 1)
1. `m_arena` alignment: FIXED (`alignas(8)`). 2. Arena bound before rounding, single-allocation invariant: FIXED.
3. Burst vs events queue: FIXED (design 1). 4. 530 KiB `.bss` in the OFF image: FIXED — OFF keeps 1 KiB / 4 objects /
   64 B (enough for UT-RT-8). 5. False `const` on the section entries + registration discipline: FIXED (`struct
   gcov_info* const []`, no `const_cast`, comment). 6. `-fprofile-update=atomic`: future work (SDD notes the tearing).
7. Null `__gcov_init`/`__gcov_reset` in a mixed image: future work (unreachable with strong references in ON).
8. Lenient `from_code` vs strict `from_repr`: future work (documented FFI boundary is `code`/`from_code`).

### Anchor ruling D2' (persistence across processes)
Smoke showed the dump only sees the dump process's counters (16.9 % after a zero vs 53 % without). RULED and APPLIED:
native persistence through libgcov's hosted dump/merge — `init` registers the section once (`__gcov_init`) and
installs an `atexit` flush; `MARK` → `Marker` then `__gcov_dump(); __gcov_reset();`; a chunk-0 window flushes and
snapshots the on-disk `.gcda` files (paths from libgcov's filename callback, `nameCrc` = CRC-32 of that path);
`count = 0` → `__gcov_reset()` + delete the files (`CoverageReset`); OFF image unchanged (weak symbols,
`CoverageUnavailable`). New `DumpReject::FILE_ERROR`; REQ-RustTester-012/013; UT-RT-9; UT-RT-6 flushes 50 markers
first. Proof: two-process scenario (see the re-entry report). Supersedes the earlier "no exit-time dump" note.

#### D2' local lenses (correctness Go, F Prime code Go-after-1–2, security No-Go→fixed)
- code 1 / security 2 / correctness 2 — `removeFile` status discarded: FIXED (only `OP_OK`/`DOESNT_EXIST` accepted; a
  reset that cannot delete answers `CoverageRejected(FILE_ERROR)` + `EXECUTION_ERROR`; REQ-RustTester-010 reworded).
- code 2 — `std::atexit` status unchecked: FIXED (`FW_ASSERT(status == 0)`).
- security 1 — `Os::File::open` asserts on a path ≥ `FileNameStringSize`: FIXED (length checked first → `FILE_ERROR`).
- security 5 — exit-path re-entrancy with `GCOV_EXIT_AT_ERROR`: FIXED (static guard in `flushCounters`).
- correctness 3 — empty file accepted: FIXED (`size == 0` → `FILE_ERROR`).
- code 3/5/6/12, correctness 7 — stale comments, `-Waddress` tautology (`RUST_TESTER_HAVE`), `free`/`read` names,
  lost increments between dump and reset: FIXED (code + SDD).
- code 9 — REQ-012 verification: FIXED (UT-RT-9 second marker cumulative; REQ-012 `Unit Test; Integration Test`, the
  two-process proof is the system evidence). code 15 — unlink before reset: FIXED.
- security 3/4/6/7/8, correctness 1/4/5/6, code 10/11/13/14 — failed libgcov dump invisible (stderr only),
  `GCOV_PREFIX*` unsupported, content not validated beyond non-empty, dump/reset window loses increments, one
  process per cache, `Runs: 0` after an in-process reset, FILE_ERROR UT via chmod, path caching: documented in the
  SDD "Limits of the hosted scheme" (future work); `init` hiding the base init documented in the header.

### Anchor ruling D2'' (destructor coverage)
Harness run: destructors (`~RustCounter`/`~RustMonitor` Box drop, `~Relay`) never counted — the atexit flush was
registered from `init()`, after the static topology instances were constructed, so LIFO ran it before their
destructors. APPLIED: a startup constructor (`constructor(101)`, originally an `init_priority(101)` guard object —
changed on the F´-code lens finding that the object's compiler-generated initializer carried counters) in
`RustTester.cpp` registers libgcov and the flush before any default-priority static (ON only); `init()` stays
idempotent; REQ-RustTester-013 reworded.
Verified: the topology instances are namespace-scope statics in `FprimeRustReferenceTopologyAc.cpp`, `Main.cpp`
returns from `main` after the SIGTERM handler; two-process proof shows `gcov -f` hits on the three destructors.

### Anchor ruling (same cycle): deterministic command burst
The harness child ruled a deployment-side burst is the only replay-faithful way to prove the SET_LIMIT overflow
hook (fth's sealed `send` row = one fprime-cli process per command). APPLIED: `RustTester.BURST_CMD_U32(count,
opcode, arg)` (opcode 2), `cmdBurstOut: Fw.Com -> CdhCore.cmdDisp.seqCmdBuff[1]`, `seqCmdStatus[1] ->
cmdBurstStatus`, `BurstSent` event (id 7), `BurstResponsesOk`/`BurstResponsesError` telemetry (ids 0/1);
REQ-RustTester-014..017, UT-RT-10..12; residual flake (~1e-4 per burst) and the test-deployment-only nature of a
ground-settable replayed opcode recorded in the SDD.

Local lenses on the D2''+burst delta (children, nothing posted):
- security No-Go -> fixed: the burst's OK response went onto cmdDisp's hook-less `compCmdStat` behind 16 copies
  (assert risk) -> respond before the loop, `MAX_BURST` 12; SDD "nothing is interpreted" was wrong -> hazard
  paragraph (async commands without hook assert when a burst exceeds their queue) and plan rule; generation in
  the port context so late responses of an earlier burst are not counted; `burstOpcode` naming.
- F´ code Go after 1-4 -> fixed: `init_priority` object replaced by a `constructor(101)` free function (the
  object's `_GLOBAL__sub_I` stub was instrumented inside the judged filter); friend comment paste; SDD ports /
  commands / event 7 / telemetry table; UT-RT-10..12 REQUIREMENT/COMMENT banners; `import Fw.Channel`; one
  packet per burst (async port serializes on enqueue); 120-col; REQ-013 what-only; Packets grouping;
  declined: removing `registerObjects()` from `init()` (anchor asked for an idempotent `init`; documented as a
  no-op).
- correctness No-Go (SDD text only) -> fixed: the "copies 11 and 12 -> BUSY, flake 1e-4" model ignored
  `rustMonitor.countIn` sharing the 10-deep queue (a `BUMP`/`RESET` in the preceding second makes copy 10 `BUSY`
  too; measured 2/2/2 vs 3/3/3/2/3) -> precondition + plan oracle `LimitSet + BUSY == count`, `BUSY >= count-10`,
  `CommandDroppedQueueOverflow == 0`, `Ok + Error == count`; late-response counting -> generation in the context
  (also security #4 / F´ #12). Verified by the lens: guard ordering (dtor counted 1/1 with the guard, 0/1
  without), libgcov registers no `gcov_exit` of its own under `-fprofile-info-section`, 144/144 burst copies
  accounted for live (110 `LimitSet` + 34 `BUSY`, 0 drops).
