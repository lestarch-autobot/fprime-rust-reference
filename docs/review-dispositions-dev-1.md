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
