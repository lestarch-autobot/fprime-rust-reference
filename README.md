# fprime-rust-reference

Reference deployment that demonstrates the
[`fprime-rust`](https://github.com/lestarch-autobot/fprime-rust) autocoder by
hosting two Rust components (`RustCounter`, an active counter with RESET/BUMP commands and a
`THRESHOLD` parameter; `RustMonitor`, a queued limit monitor with SET_LIMIT) and a C++ `Relay`
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

## License

Apache 2.0 — same as F Prime and `fprime-rust`.
