# fprime-rust-reference

Reference deployment that demonstrates the
[`fprime-rust`](https://github.com/lestarch-autobot/fprime-rust) autocoder by
hosting two Rust components (`RustCounter`, an active counter with RESET/BUMP commands and a
`THRESHOLD` parameter; `RustMonitor`, a queued limit monitor with SET_LIMIT, whose overflow hook answers BUSY on a full queue) and a C++ `Relay`
between them inside a standard F Prime topology.  The structure mirrors
[`fprime-python-reference`](https://github.com/fprime-community/fprime-python-reference):

```
fprime-rust-reference/
├── CMakeLists.txt              # Loads F Prime and registers fprime-rust target
├── settings.ini                # framework_path + library_locations
├── lib/
│   ├── fprime/                 # submodule  -> github.com/nasa/fprime
│   └── fprime-rust/            # submodule  -> github.com/lestarch-autobot/fprime-rust
├── FprimeRustReference/
│   ├── Components/             # project-specific components
│   │   ├── RustCounter/        # active Rust component  (RustCounter.rs)
│   │   ├── RustMonitor/        # queued Rust component (RustMonitor.rs)
│   │   └── Relay/              # C++ component forwarding counts to the monitor
│   └── ReferenceDeployment/    # topology + Main.cpp
├── bench/                      # native_flash.py / native_dump.py for the test bench
└── requirements.txt
```

## Quick start

```bash
git clone --recurse-submodules https://github.com/lestarch-autobot/fprime-rust-reference.git
cd fprime-rust-reference

# 1. Set up an F Prime venv and install fprime-rust
python3 -m venv fprime-venv
source fprime-venv/bin/activate
pip install -r requirements.txt

# 2. Generate + build the deployment (cargo must be on PATH)
fprime-util generate
fprime-util build -j$(nproc)

# 3. Run the binary alongside fprime-gds
fprime-gds --gui none &                                          # one terminal
./build-artifacts/Linux/FprimeRustReference/bin/FprimeRustReference  # another terminal

# 4. Talk to the Rust components
fprime-cli command-send FprimeRustReference.rustCounter.RESET
fprime-cli command-send FprimeRustReference.rustCounter.BUMP --arguments 7
fprime-cli command-send FprimeRustReference.rustMonitor.SET_LIMIT --arguments 4
fprime-cli channels    # Counter, TickCount, Limit, LastCount, Exceedances, Relayed
```

## What's in a Rust component?

```
FprimeRustReference/Components/RustCounter/
├── RustCounter.fpp        # FPP model — annotated `@ fprime-rust`
├── RustCounter.rs         # User-authored Rust handlers (impl Component for RustCounter)
├── docs/sdd.md            # design + unit-test list
├── test/ut/               # GTest unit tests over the generated shim (fprime-util check)
└── CMakeLists.txt         # register_fprime_module(... SOURCES RustCounter.rs)
```

Each build regenerates the C++ shim, the Rust base module (`rustcounter_base.rs`) and a
`RustCounter.template.rs` in the module build directory; all Rust components of the deployment are
linked from one `libfprime_rust_<Deployment>.a`. See the
[`fprime-rust` README](https://github.com/lestarch-autobot/fprime-rust) for the full design.

## System test harness (fth)

The system-test plan for the reference deployment lives in
`FprimeRustReference/ReferenceDeployment/test/int/` (`plan.json` + rendered `plan.md`, `planted_faults.py`
helpers and planted-fault menu, `bench.json` native bench, `test_rust_reference.py` stimulus, one pytest per
plan test L0-1..L7-1) and runs under the F Prime testing harness `fth` pinned by the campaign
(`fprime-testing-harness` 855dd4f = v0.1.2). `bench/native_recorder.py` and `bench/native_send.py` adapt
`fprime-cli` to the record layout `fth` exports (see A3/A4 in the plan's decision log).

```bash
export PATH="$PWD/venv/bin:/usr/bin:$PATH"          # a conda gcc may shadow /usr/bin
pip install git+https://github.com/lestarch-autobot/fprime-testing-harness@855dd4f9901469d1d06400fa0759ea8793614f8e
(cd FprimeRustReference/ReferenceDeployment && fprime-util generate && fprime-util build -j8)
T=$PWD/FprimeRustReference/ReferenceDeployment/test/int
OUT=~/campaign-out                                   # run dirs, manifests and images live OUTSIDE the git trees
mkdir -p $OUT/images $OUT/dev
cp build-artifacts/Linux/FprimeRustReference_ReferenceDeployment/bin/FprimeRustReference_ReferenceDeployment $OUT/images/FprimeRustReference.bin

# plan + stimulus consistency (helpers bind, every planted-fault `fails` predicate is a declared check)
fth verify-plan $T/plan.json --tests $T --bench $T/bench.json

# manifest: pinned UUT / fprime / harness trees, plan, menu, bench, stimulus and the image's Svc.Version strings
cd $OUT/dev && fth manifest --uut $OLDPWD/lib/fprime-rust --harness $OLDPWD --repo fprime=$OLDPWD/lib/fprime \
  --plan $T/plan.json --image FprimeRustReference=$OUT/images/FprimeRustReference.bin \
  --menu $T/faults-menu.json --bench $T/bench.json --tests $T -o manifest.json

# dry run: no deployment, every command logged, every check "not evaluated: dry-run"
fth run --run-dir $OUT/runs/dry --manifest manifest.json --plan $T/plan.json --bench $T/bench.json --tests $T --dry-run --unsealed

# the whole plan on the native bench (fth launches fprime-gds + the deployment from bench.json, records, judges)
fth run --run-dir $OUT/runs/clean --manifest manifest.json --plan $T/plan.json --bench $T/bench.json --tests $T --unsealed
cat $OUT/runs/clean/runner/report.md

# helper known-answer tests (plain pytest; not a plan test, fth never collects it). -B and
# no:cacheprovider keep __pycache__/.pytest_cache out of test/int: fth refuses a stimulus dir
# holding __pycache__ (delete `$T/__pycache__ $T/.pytest_cache` before any fth run/verify-plan/manifest)
python -B -m pytest $T/helpers_kat.py -q -p no:cacheprovider

# one test only (pytest selection after the fth options)
fth run --run-dir $OUT/runs/l0-1 --manifest manifest.json --plan $T/plan.json --bench $T/bench.json --tests $T --unsealed -k test_L0_1

# a planted fault, development style (its `fails` set must FAIL, everything else PASS)
fth run --run-dir $OUT/runs/bump-zero --manifest manifest.json --plan $T/plan.json --bench $T/bench.json --tests $T --unsealed --plant L5-1=bump_zero
```

Before every launch make sure no other deployment/GDS actor is up (`ss -ltnp | grep 5000` empty) and kill the
processes you started by their PID only. The stimulus must stay blind to the draw: no fixture exposes the
sealed faults, and `fth verify-plan --tests` refuses a test that names them.

### Coverage evidence (development only, never the RfR image)

```bash
# twin build (see "Harness tester and coverage twin"); always name the build cache explicitly — without
# --build-cache fprime-util picks the most recent cache and installs the twin into build-artifacts/
# pure image  : build-fprime-automatic-native/bin/Linux/FprimeRustReference_ReferenceDeployment   (nm | grep -c ' T __gcov' == 0)
# twin image  : build-fprime-automatic-native-cov/bin/Linux/FprimeRustReference_ReferenceDeployment
# coverage run: the main plan on the twin; bench-cov.json's flash step deletes every .gcda under the twin cache first
fth manifest ... --image FprimeRustReference=<twin.bin> --bench $T/bench-cov.json -o manifest-cov.json
fth run --run-dir $OUT/runs/cov-run --manifest manifest-cov.json --plan $T/plan.json --bench $T/bench-cov.json --tests $T --unsealed
# dump run + UT coverage + pure run -> dev-evidence/ (dev-campaign makes the dump manifest and runs test/int/gcov-dump
# with bench-dump.json, which leaves the .gcda files alone; the three --filter values are the plan's coverage.system.scope)
fth dev-campaign --out $OUT/dev-evidence --plan $T/plan.json --tests $T --bench $T/bench-dump.json --ut-coverage <gcovr dir> \
  --coverage-run $OUT/runs/cov-run --coverage-manifest manifest-cov.json --dump-run $OUT/runs/dump-run \
  --dump-plan $T/gcov-dump/plan.json --dump-tests $T/gcov-dump --gcno-root build-fprime-automatic-native-cov --project-root . \
  --filter FprimeRustReference/Components/ --filter lib/fprime-rust/FprimeRust/ \
  --filter 'build-fprime-automatic-native-cov/FprimeRustReference/Components/.*/(?!.*Ac\.cpp$)[^/]*\.cpp$' \
  --gcov <gcov of the GCC that built the twin> --pure-run $OUT/runs/clean --pure-manifest manifest.json
```

## Harness tester and coverage twin

`FprimeRustReference.RustTester` (`rustTester`, base id 0x10027000, plain C++, `Components/RustTester/`) is the
harness tester of fprime-system-test-harness §1(4): `MARK(id)` emits `Marker {}` (the fth run-start marker) and
`DUMP_COVERAGE(start, count)` pages the gcov counters of the judged C++ scope out as `CoverageChunk` /
`CoverageObject` / `CoverageDumpDone` events in the `fth coverage-system` text contract (§3a). Native persistence is
libgcov's on-disk merge: every `MARK` flushes the counters into the `.gcda` files beside the `.gcno` in the twin's
build cache (dump + reset), a chunk-0 window flushes and then pages those files, `DUMP_COVERAGE 0 0` zeroes the
counters and deletes the files — so a dump process reads what earlier processes accumulated. The pure
reference image (default) carries no instrumentation and answers `CoverageUnavailable`; the coverage twin is a
second build cache with the `FPRIME_RUST_REFERENCE_COVERAGE` option and a GCC >= 12:

```bash
cd FprimeRustReference/ReferenceDeployment
CC=gcc-15 CXX=g++-15 fprime-util generate -DFPRIME_RUST_REFERENCE_COVERAGE=ON --build-cache "$PWD/../../build-fprime-automatic-native-cov"
fprime-util build --build-cache "$PWD/../../build-fprime-automatic-native-cov" -j8
# before the coverage run only: DUMP_COVERAGE 0 0 (zero + delete the .gcda files); never between runs
# after a run that paced DUMP_COVERAGE windows (8 chunks each, start = 0, 8, 16, ... until CoverageDumpDone):
fth coverage-system --record <events.jsonl dir> --gcno-root "$PWD/../../build-fprime-automatic-native-cov"     --project-root "$PWD/../.." --filter FprimeRustReference/Components/ --filter lib/fprime-rust/FprimeRust/     --filter build-fprime-automatic-native-cov/FprimeRustReference/Components/ --gcov gcov-15 --out <out>
```

`BURST_CMD_U32(count, opcode, arg)` (test deployment only) replays one `FW_PACKET_COMMAND` packet `count` times
(1..12) into `CdhCore.cmdDisp.seqCmdBuff[1]` from inside the deployment, so the plan can fill `RustMonitor`'s
10-deep `SET_LIMIT` queue within one tick and prove the overflow hook (`OpCodeError(..., BUSY)` from the
dispatcher); `BurstSent` reports the burst, `BurstResponsesOk`/`BurstResponsesError` count the dispatcher's
responses (`seqCmdStatus[1]`); burst only hook-protected async commands such as `SET_LIMIT` — an async command
without an overflow hook asserts when a burst exceeds its queue. Exit-time flush: a startup constructor
(`constructor(101)`) registers libgcov and the atexit flush before the topology statics, so destructor
increments are counted.

Both caches install into the same `build-artifacts/` directory, so copy the twin's `bin/` and dictionary aside
before building the other image. The filter-to-scope mapping and the `.gcno` locations are in
`Components/RustTester/docs/sdd.md`.

## License

Apache 2.0 — same as F Prime and `fprime-rust`.
