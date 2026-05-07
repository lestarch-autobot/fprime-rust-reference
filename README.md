# fprime-rust-reference

Reference deployment that demonstrates the
[`fprime-rust`](https://github.com/JPL-Devin/fprime-rust) autocoder by
hosting a Rust-backed component (`RustExample`) inside a standard F Prime
topology.  The structure mirrors
[`fprime-python-reference`](https://github.com/fprime-community/fprime-python-reference):

```
fprime-rust-reference/
├── CMakeLists.txt              # Loads F Prime and registers fprime-rust target
├── settings.ini                # framework_path + library_locations
├── lib/
│   ├── fprime/                 # submodule  -> github.com/nasa/fprime
│   └── fprime-rust/            # submodule  -> github.com/JPL-Devin/fprime-rust
├── FprimeRustReference/
│   ├── Components/             # project-specific components
│   │   ├── RustExample/        # the demo Rust component
│   │   └── ...                 # additional reference components
│   └── ReferenceDeployment/    # topology + Main.cpp
└── requirements.txt
```

## Quick start

```bash
git clone --recurse-submodules https://github.com/JPL-Devin/fprime-rust-reference.git
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

# 4. Talk to the Rust component
fprime-cli command-send FprimeRustReference.rustExample.Reset
fprime-cli command-send FprimeRustReference.rustExample.Bump --arguments 7
fprime-cli channels    # observe FprimeRustReference.rustExample.Counter
```

## What's in `RustExample`?

```
FprimeRustReference/Components/RustExample/
├── RustExample.fpp        # FPP model — annotated `@ fprime-rust`
├── RustExample.rs         # User-authored Rust handlers
└── CMakeLists.txt         # register_fprime_module(...)
```

The autocoder regenerates the C++ shim and per-component Cargo workspace
on every build.  See the
[`fprime-rust` README](https://github.com/JPL-Devin/fprime-rust) for the
full design.

## License

Apache 2.0 — same as F Prime and `fprime-rust`.
