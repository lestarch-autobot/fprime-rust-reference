# FprimeRustReference::RustTester — Software Design Document

`RustTester` is the harness tester component of the F Prime Rust reference deployment
(fprime-system-test-harness §1(4): a tester component in the test tree that ships with its own unit tests;
the reference deployment is the chosen test deployment, so it is wired into this topology as `rustTester`).
It is plain C++ and **not** part of the unit under test: it gives the ground a run-start marker the fth auditor
recognises and pages the gcov counters of the instrumented twin image (§3a) back through events.

## Requirements

| Requirement | Description | Verification |
|---|---|---|
| REQ-RustTester-001 | `MARK(id)` shall emit `Marker(id)` synchronously in the handler and then respond `OK`. | Unit Test (UT-RT-1); Integration Test: run-start marker of every plan test |
| REQ-RustTester-002 | `DUMP_COVERAGE(start, count)` with `1 <= count <= 8` shall emit the global 32-byte gcda chunks `start..start+count-1` as `CoverageChunk(object, offset, length, data0..data7)` (bytes MSB-first, zero padded), `CoverageObject(object, nameCrc, bytes)` with an object's last chunk (`nameCrc` = CRC-32/IEEE of the compiler's `.gcda` path), and `CoverageDumpDone(objects, totalBytes, crc)` (CRC-32 over all gcda bytes in object order) when the window reaches the end of the stream. | Unit Test (UT-RT-4); Integration Test: dump plan, reassembled by `fth coverage-system` |
| REQ-RustTester-003 | A window starting at chunk 0 shall snapshot the whole gcda stream; later windows shall read that snapshot so every window of one walk describes the same counter state. A window with `start > 0` and no snapshot shall respond `EXECUTION_ERROR` without events. | Unit Test (UT-RT-4, UT-RT-5) |
| REQ-RustTester-004 | In an image built without `FPRIME_RUST_REFERENCE_COVERAGE`, `DUMP_COVERAGE` shall emit `CoverageUnavailable` and respond `OK`. | Unit Test (UT-RT-3) |
| REQ-RustTester-005 | `DUMP_COVERAGE` with `count > 8` shall respond `VALIDATION_ERROR` without events. | Unit Test (UT-RT-2) |
| REQ-RustTester-006 | `DUMP_COVERAGE(start, 0)` shall zero the arc counters of every instrumented object, invalidate the snapshot and respond `OK`. | Unit Test (UT-RT-6) |
| REQ-RustTester-007 | The dump path (handlers, walk, callbacks, CRC) shall not be instrumented (`no_profile_instrument_function`), so executing it between windows does not move the counters it reports. | Inspection: `RUST_TESTER_NO_PROFILE` on every function of `RustTester.cpp` |

## Design

Passive component; `Fw.Command`, `Fw.Event` interfaces and `timeCaller`. Commands: `MARK(id: U32)` opcode 0,
`DUMP_COVERAGE(start: U16, count: U8)` opcode 1. Events (all `activity low` except `CoverageUnavailable`,
`warning low`), text formats are the `fth/system_coverage.py` `_TEXT` contract verbatim:

| Id | Event | Format |
|---|---|---|
| 0 | `Marker(id: U32)` | `Marker {}` |
| 1 | `CoverageChunk(object, offset, length, data0..data7: U32)` | `coverage chunk obj {} off {} len {}: {x} {x} {x} {x} {x} {x} {x} {x}` |
| 2 | `CoverageObject(object, nameCrc, bytes: U32)` | `coverage object {} name-crc {x}: {} bytes` |
| 3 | `CoverageDumpDone(objects, totalBytes, crc: U32)` | `coverage dump done: {} objects, {} bytes, crc {x}` |
| 4 | `CoverageUnavailable` | `coverage not compiled into this image` |

(`id` is an FPP keyword; the model spells the argument `$id`, the dictionary and the C++ see `id`.)

### Coverage image (twin)

`FPRIME_RUST_REFERENCE_COVERAGE` (CMake option, default OFF, `Components/CMakeLists.txt`) adds
`-fprofile-arcs -ftest-coverage -fprofile-info-section=gcov_info -fprofile-update=single` to the judged
scope — `FprimeRustReference_Components_{RustCounter,RustMonitor}` (the generated Rust shim C++ and their
autocode), `Relay`, `RustTester` and the `FprimeRust` glue library — and links `gcov` into them. OFF touches
nothing: the RfR image is the OFF build. The info section needs GCC >= 12 (`__gcov_info_to_gcda`); the section
name `gcov_info` is a C identifier so GNU ld provides `__start_gcov_info`/`__stop_gcov_info` without a linker
script. Libgcov symbols are declared weak in `RustTester.cpp`, so the OFF image links on any GCC and reports
`CoverageUnavailable`.

### Dump walk

Because `-fprofile-info-section` disables libgcov's constructor registration, nothing is written at exit and
the image never touches the filesystem. A chunk-0 window walks `__start_gcov_info..__stop_gcov_info`, calling
`__gcov_info_to_gcda(info, filenameCallback, dumpCallback, allocateCallback, this)` per object into a static
512 KiB snapshot (`m_snapshot`, up to 64 objects; a stream that does not fit answers `EXECUTION_ERROR`). The
snapshot is necessary on this deployment: the rate groups keep ticking `rustCounter`/`rustMonitor` between
the ground-paced windows, so live counters would make the per-window chunks and the `Done` CRC disagree
(`fth coverage-system` refuses such a record). The allocate callback hands out 8-byte aligned blocks of a
16 KiB static arena (libgcov asks only for top-n counter scratch, unused by `-fprofile-arcs`); no dynamic
allocation anywhere. Zeroing registers the section's objects with libgcov once per process via `__gcov_init` (hidden
visibility, statically linkable) and calls `__gcov_reset()`. Side effect: after a `count = 0`, libgcov's exit
destructor also writes the registered objects' `.gcda` beside their `.gcno` when the process exits (an image that
never received a zero writes nothing); `fth coverage-system` overwrites them with the reconstructed stream. GCC >= 12
omits all-zero counter records, so a stream dumped after a zero is shorter than the stream before it.

`count` is capped at 8 so one window is at most 10 events (8 chunks + object + done), well under the 20-deep
`CdhCore.cmdDisp` queue; the ground paces one command per window.

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
| UT-RT-2 | `DUMP_COVERAGE(0, 9)`: `VALIDATION_ERROR`, no events | REQ-RustTester-005 |
| UT-RT-3 | uninstrumented build: `DUMP_COVERAGE(0, 8)` and `(0, 0)` emit `CoverageUnavailable`, respond `OK` (skipped in the instrumented build) | REQ-RustTester-004 |
| UT-RT-4 | instrumented build: windows of 8 from chunk 0 to `Done`; chunk lengths sum to the object `bytes` and `totalBytes`, CRC-32 over the chunk bytes equals the `Done` crc; a window past the end answers `Done` again (skipped when uninstrumented) | REQ-RustTester-002, REQ-RustTester-003 |
| UT-RT-5 | instrumented build: after `count = 0`, `DUMP_COVERAGE(1, 8)` is `EXECUTION_ERROR` without events | REQ-RustTester-003 |
| UT-RT-6 | instrumented build: walk, `count = 0`, walk: same objects, stream no longer than before (GCC >= 12 omits all-zero counter records), different CRC (counters zeroed) | REQ-RustTester-006 |

The instrumented unit-test build is a second UT cache: `fprime-util generate --ut -DFPRIME_RUST_REFERENCE_COVERAGE=ON
--build-cache build-fprime-automatic-native-ut-cov` with a GCC >= 12 (`CC`/`CXX`), then
`fprime-util check --build-cache build-fprime-automatic-native-ut-cov`.
