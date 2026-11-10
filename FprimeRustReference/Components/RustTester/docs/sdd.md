# FprimeRustReference::RustTester — Software Design Document

`RustTester` is the harness tester component of the F Prime Rust reference deployment
(fprime-system-test-harness §1(4): a tester component in the test tree that ships with its own unit tests;
the reference deployment is the chosen test deployment, so it is wired into this topology as `rustTester`).
It is plain C++ and **not** part of the unit under test: it gives the ground a run-start marker the fth auditor
recognises and pages the gcov counters of the instrumented twin image (§3a) back through events.

## Requirements

| ID | Requirement | Verification |
|---|---|---|
| REQ-RustTester-001 | `MARK(id)` shall emit `Marker(id)` synchronously in the handler and then respond `OK`. | Unit Test |
| REQ-RustTester-002 | `DUMP_COVERAGE(start, count)` with `1 <= count <= 8` shall emit the global 32-byte gcda chunks `start..start+count-1` as `CoverageChunk(object, offset, length, data0..data7)`, bytes MSB-first and zero padded. | Unit Test |
| REQ-RustTester-003 | With an object's last chunk `DUMP_COVERAGE` shall emit `CoverageObject(object, nameCrc, bytes)`, `nameCrc` the CRC-32/IEEE of the compiler's `.gcda` path. | Unit Test |
| REQ-RustTester-004 | When a window reaches the end of the stream `DUMP_COVERAGE` shall emit `CoverageDumpDone(objects, totalBytes, crc)`, `crc` the CRC-32 over all gcda bytes in object order. | Unit Test |
| REQ-RustTester-005 | A window with `start = 0` shall flush the counters (libgcov dump + reset) and snapshot the on-disk `.gcda` file of every instrumented object; later windows shall read that snapshot, so every window of one walk describes the same counter state. | Unit Test |
| REQ-RustTester-006 | A window with `start > 0` and no snapshot shall emit `CoverageRejected(NO_SNAPSHOT)` and respond `EXECUTION_ERROR`. | Unit Test |
| REQ-RustTester-007 | A stream that does not fit the snapshot buffer or the object table shall emit `CoverageRejected(OVERFLOW)`, an unreadable `.gcda` file `CoverageRejected(FILE_ERROR)`, each with `EXECUTION_ERROR`. | Inspection |
| REQ-RustTester-008 | In an image built without `FPRIME_RUST_REFERENCE_COVERAGE`, `DUMP_COVERAGE` shall emit `CoverageUnavailable` and respond `OK`. | Unit Test |
| REQ-RustTester-009 | `DUMP_COVERAGE` with `count > 8` shall respond `VALIDATION_ERROR` without events. | Unit Test |
| REQ-RustTester-010 | `DUMP_COVERAGE(start, 0)` shall delete every instrumented object's on-disk `.gcda` file, zero the arc counters, invalidate the snapshot and emit `CoverageReset(objects)` with `OK`; a file that cannot be deleted shall answer `CoverageRejected(FILE_ERROR)` + `EXECUTION_ERROR`. | Unit Test |
| REQ-RustTester-011 | Every function of the hand-written `RustTester.cpp` shall carry `no_profile_instrument_function`, so the harness logic itself contributes no counters. | Inspection |
| REQ-RustTester-012 | In the coverage twin, `MARK` shall flush the counters into the on-disk `.gcda` files (libgcov dump) and zero them (reset) after emitting `Marker`, so coverage accumulated before a marker survives the process. | Unit Test; Integration Test |
| REQ-RustTester-013 | In the coverage twin, `init` shall register every object of the gcov information section with libgcov once per process and arrange a flush at normal process exit. | Inspection |

No general ports: the component has only the command, event and time special ports.

## Design

Passive component; `Fw.Command`, `Fw.Event` interfaces and `timeCaller`. Commands: `MARK(id: U32)` opcode 0,
`DUMP_COVERAGE(start: U16, count: U8)` opcode 1. Events (all `activity low` except `CoverageUnavailable` and `CoverageRejected`,
`warning low`), text formats are the `fth/system_coverage.py` `_TEXT` contract verbatim:

| Id | Event | Format |
|---|---|---|
| 0 | `Marker(id: U32)` | `Marker {}` |
| 1 | `CoverageChunk(object, offset, length, data0..data7: U32)` | `coverage chunk obj {} off {} len {}: {x} {x} {x} {x} {x} {x} {x} {x}` |
| 2 | `CoverageObject(object, nameCrc, bytes: U32)` | `coverage object {} name-crc {x}: {} bytes` |
| 3 | `CoverageDumpDone(objects, totalBytes, crc: U32)` | `coverage dump done: {} objects, {} bytes, crc {x}` |
| 4 | `CoverageUnavailable` | `coverage not compiled into this image` |
| 5 | `CoverageReset(objects: U32)` | `coverage counters zeroed: {} objects` |
| 6 | `CoverageRejected(reason: DumpReject {NO_SNAPSHOT, OVERFLOW, FILE_ERROR})` (warning low) | `coverage dump rejected: {}` |

(`id` is an FPP keyword; the model spells the argument `$id`, the dictionary and the C++ see `id`.)

### Coverage image (twin)

`FPRIME_RUST_REFERENCE_COVERAGE` (CMake option, default OFF, `Components/CMakeLists.txt`) adds
`-fprofile-arcs -ftest-coverage -fprofile-info-section=gcov_info -fprofile-update=single` to the judged
scope — `FprimeRustReference_Components_{RustCounter,RustMonitor}` (the generated Rust shim C++ and their
autocode), `Relay`, `RustTester` and the `FprimeRust` glue library — and links `gcov` into them. OFF touches
nothing: the RfR image is the OFF build. The info section exists since GCC 11, but the five-argument `__gcov_info_to_gcda` (allocate callback) used
here is GCC >= 12, and the host's GCC 11 libgcov does not export the function at all; the section
name `gcov_info` is a C identifier so GNU ld provides `__start_gcov_info`/`__stop_gcov_info` without a linker
script. Libgcov symbols are declared weak in `RustTester.cpp`, so the OFF image links on any GCC and reports
`CoverageUnavailable`.

### Dump walk and native persistence

Native persistence = libgcov's hosted on-disk dump/merge (the MCU analogue is counters in NOLOAD RAM): the fth
`dev-campaign` runs the plan on the twin and the dump plan as two separate processes, and with
`-fprofile-info-section` nothing is registered or written by itself, so the twin does it explicitly:

- `init` registers the section's objects with libgcov once per process (`__gcov_init`) and installs an `atexit`
  flush, so a clean exit (SIGINT/SIGTERM handled by `Main.cpp`) writes the tail after the last marker.
- `MARK` emits `Marker(id)` and then calls `__gcov_dump(); __gcov_reset();` — the in-memory counters are merged
  into each object's `.gcda` at the path the compiler recorded (`gcov_info->filename`, under the twin's build
  cache) and restart at zero, so the files are cumulative without double counting (increments between the dump
  and the reset are lost; a SIGKILL or assert-abort loses everything since the last marker). About ten small
  files per marker on the host. The persistence across processes is proven by the two-process system test, not
  by a unit test.
- A chunk-0 window flushes the same way, then reads each object's `.gcda` from disk (`Os::File`) into the
  512 KiB snapshot (file-scope storage in `RustTester.cpp`, about 528 KiB of `.bss` with the arena and object
  table, up to 64 objects, single instance asserted; the OFF image keeps 1 KiB / 4 objects / 64 B for the
  white-box unit test — the sizes live in the `.cpp`, never in the header, so the topology's translation unit
  sees the same class layout). The object's path is obtained from libgcov itself through the filename callback
  of `__gcov_info_to_gcda` (the data callback is unused), so the component does not depend on the `gcov_info`
  layout; `nameCrc` is the CRC-32 of that path, which is exactly what `fth coverage-system` matches against
  `<gcno>.gcda` under `--gcno-root`. A stream that does not fit answers `CoverageRejected(OVERFLOW)`, an
  unreadable file `CoverageRejected(FILE_ERROR)`, both with `EXECUTION_ERROR`.
- `count = 0` resets the counters (`__gcov_reset`) and deletes every object's `.gcda` (`Os::FileSystem::removeFile`;
  a missing file is already in the wanted state), then emits `CoverageReset(objects)`. The dump plan uses it only
  before the coverage run; the dump run never resets.

The snapshot is still necessary: the rate groups keep ticking `rustCounter`/`rustMonitor` between the
ground-paced windows, so live counters would make the per-window chunks and the `Done` CRC disagree (`fth
coverage-system` refuses such a record). The allocate callback hands out 8-byte aligned blocks of a 16 KiB
arena (libgcov asks only for top-n counter scratch, unused by `-fprofile-arcs`); no dynamic allocation anywhere.
GCC >= 12 omits all-zero counter records, so a stream dumped after a zero is shorter than the stream before it.
Limits of the hosted scheme (documented, not guarded): a libgcov dump that fails (unwritable file) is reported
by libgcov on stderr only and the following reset still zeroes the in-memory counters; `GCOV_PREFIX` /
`GCOV_PREFIX_STRIP` relocate libgcov's writes while the component reads and deletes the compiler-recorded path,
so they are unsupported for the twin; a path at or beyond `FileNameStringSize` answers `FILE_ERROR` instead of
asserting; file content is not validated beyond "not empty" (size and CRC are what the ground checks); after a reset in a
running process the new files' summary says `Runs: 0` (libgcov counts the run once per process; counters are
correct); one twin process per build cache. The on-disk files are also what gcov/gcovr read: delete `*.gcda` under the gcno root only before the coverage
run, never between the coverage run and the dump run, and run `fth coverage-system` after the dump process has
exited (it overwrites the files with the reconstructed stream).

`count` is capped at 8 so one window is at most 17 events (8 chunks, up to 8 `CoverageObject` for tiny objects,
one `CoverageDumpDone`); they go to `CdhCore.events` (Svc.EventManager), whose queue drops on overflow, so the
reference raises `CdhCoreConfig.QueueSizes.events` from 10 to 40 (`ReferenceDeployment/CdhCoreConfig/`) and the
dump plan should check `EventsDropped` did not move. The ground paces one command per window. Every window with
`start = 0` takes a fresh snapshot, so the ground must not retry window 0 in the middle of a walk (restart the walk
instead): fth rejects a record whose repeated chunks differ. The twin uses `-fprofile-update=single`, so counters
of the active `rustCounter` thread may move or tear while a snapshot is taken (benign; `-fprofile-update=atomic`
is the future option).

What is counted: only the hand-written `RustTester.cpp` is excluded from the counters (REQ-011); the component's
FPP autocode (`RustTesterComponentAc.cpp`, dispatch, `log_*`, `cmdResponse_out`) is compiled with the module and
is therefore instrumented and runs on every window — the snapshot (REQ-005), not the attribute, is what keeps a
walk consistent. With the third gcovr filter below that autocode is part of the reported figure (harness autocode
in the judged scope, flagged for ratification; `fth coverage-system` has no exclude option).

### gcovr mapping (for `fth coverage-system` / `fth dev-campaign`)

`--gcno-root <coverage build cache>` (the twin's cache, e.g. `build-fprime-automatic-native-cov`),
`--project-root <checkout>`, `--gcov <gcov of the GCC that built the twin>` and three `--filter` values, each a
directory relative to the checkout:

| `--filter` | Sources it admits | `.gcno` location under the gcno root |
|---|---|---|
| `FprimeRustReference/Components/` | hand-written `Relay/Relay.cpp`, `RustTester/RustTester.cpp` | `FprimeRustReference/Components/<Comp>/CMakeFiles/FprimeRustReference_Components_<Comp>.dir/<Comp>.cpp.gcno` |
| `lib/fprime-rust/FprimeRust/` | the `FprimeRust` glue (`FprimeRust.cpp`/`.hpp`) | `FprimeRust/CMakeFiles/FprimeRust.dir/FprimeRust.cpp.gcno` |
| `<cache>/FprimeRustReference/Components/` | the generated Rust shim C++ `RustCounter/RustCounter.cpp`, `RustMonitor/RustMonitor.cpp` — and, in the same directories, the FPP autocode `*ComponentAc.cpp/.hpp` of the four components | `FprimeRustReference/Components/<Comp>/CMakeFiles/FprimeRustReference_Components_<Comp>.dir/<file>.cpp.gcno` |

The generated shim sources live in the build cache, so without the third filter they are invisible to gcovr; with
it the FPP autocode is counted too (`fth coverage-system` has no exclude option). The compiler records absolute
`.gcda` paths next to the `.gcno`; `fth coverage-system` matches `CoverageObject.nameCrc` against the CRC-32 of
`<gcno path>.gcda` and writes the reassembled counters there.

Smoke on this host (gcc 15.3, `fprime-cli` paced 100 windows of 8, 9 objects / 15776 gcda bytes, CRC OK): two
filters → lines 28.6 % (16/56), branches 8.8 %; three filters → lines 53.2 % (575/1081), branches 33.3 % (321/965).

## Unit tests

| Test | Description | Requirements |
|---|---|---|
| UT-RT-1 | `MARK(42)`: one `Marker(42)`, then `OK` | REQ-RustTester-001 |
| UT-RT-2 | `DUMP_COVERAGE(0, 9)`: `VALIDATION_ERROR`, no events | REQ-RustTester-009 |
| UT-RT-3 | uninstrumented build: `DUMP_COVERAGE(0, 8)` and `(0, 0)` emit `CoverageUnavailable`, respond `OK` (skipped in the instrumented build) | REQ-RustTester-008 |
| UT-RT-4 | instrumented build: windows of 8 from chunk 0 to `Done`; chunk lengths sum to the object `bytes` and `totalBytes`, CRC-32 over the chunk bytes equals the `Done` crc; window `(8, 8)` repeated after 50 `MARK`s is byte-identical; a window past the end answers `Done` again (skipped when uninstrumented) | REQ-RustTester-002, -003, -004, -005 |
| UT-RT-5 | instrumented build: `count = 0` emits `CoverageReset`; then `DUMP_COVERAGE(1, 8)` is `EXECUTION_ERROR` with `CoverageRejected(NO_SNAPSHOT)` | REQ-RustTester-006, -010 |
| UT-RT-6 | instrumented build: 50 `MARK`s (flushed to disk), walk 1, `count = 0` (`CoverageReset(objects)`), walk A, `count = 0`, walk B: same objects, A and B strictly shorter than walk 1 (zeroed counters are omitted from the stream) with different CRCs | REQ-RustTester-010, -012 |
| UT-RT-7 | `crc32` known answer: `0xcbf43926` over `"123456789"`, continuation equals one pass | REQ-RustTester-003, -004 |
| UT-RT-8 | white-box (every build): an injected 40-byte + 32-byte snapshot pages as chunks with MSB-first words and zero padding, `CoverageObject` with each last chunk (also when the object is a multiple of 32), `Done` only once the window reaches the end | REQ-RustTester-002, -003, -004 |
| UT-RT-9 | instrumented build: after `count = 0` the first object's `.gcda` is absent; one `MARK` writes it (size > 0), a second `MARK` keeps it (size >= before, cumulative merge); `count = 0` deletes it again | REQ-RustTester-012, -010 |

In the instrumented UT cache the test executable (autocode + tester) is instrumented too, so UT-RT-4..6 have counters
that move with `MARK` and reset. The instrumented unit-test build is a second UT cache: `fprime-util generate --ut -DFPRIME_RUST_REFERENCE_COVERAGE=ON
--build-cache build-fprime-automatic-native-ut-cov` with a GCC >= 12 (`CC`/`CXX`), then
`fprime-util check --build-cache build-fprime-automatic-native-ut-cov`.
