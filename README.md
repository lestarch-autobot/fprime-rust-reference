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

# helper known-answer tests (plain pytest; not a plan test, fth never collects it)
python -m pytest $T/helpers_kat.py -q

# one test only (pytest selection after the fth options)
fth run --run-dir $OUT/runs/l0-1 --manifest manifest.json --plan $T/plan.json --bench $T/bench.json --tests $T --unsealed -k test_L0_1

# a planted fault, development style (its `fails` set must FAIL, everything else PASS)
fth run --run-dir $OUT/runs/bump-zero --manifest manifest.json --plan $T/plan.json --bench $T/bench.json --tests $T --unsealed --plant L5-1=bump_zero
```

Before every launch make sure no other deployment/GDS actor is up (`ss -ltnp | grep 5000` empty) and kill the
processes you started by their PID only. The stimulus must stay blind to the draw: no fixture exposes the
sealed faults, and `fth verify-plan --tests` refuses a test that names them.

## License

Apache 2.0 — same as F Prime and `fprime-rust`.
