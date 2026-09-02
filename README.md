# CasHMC-DLL-Interface-for-ModelSim-Integration

## 1. Project Overview

CasHMC is a cycle-accurate simulator for the Hybrid Memory Cube (HMC) architecture.

In the original CasHMC implementation, memory requests are generated through the simulator's internal transaction and packet mechanisms. In this project, selected CasHMC components have been modified to provide a simpler application-level interface.

The project is currently developed in two stages:

1. **Modified CasHMC + MemoryAPI** — provides a high-level read/write interface and validates memory transactions through the CasHMC simulation model.
2. **ModelSim DLL Interface** — provides an external interface for integrating the HMC model with ModelSim-based hardware simulations.

## 2. Project Layout

`packages/CasHMC/` is a **git submodule** pointing at the upstream [CasHMC](https://github.com/estwings57/CasHMC) repository and is not committed directly — it's fetched via `make` (see Section 3).

```text
.
├── include/
│   ├── CasHMCDLL.h            # extern "C" declarations for the DLL/SO entry points
│   ├── MemoryAPI.h
│   └── mingw_posix_compat.h   # POSIX-compat shims for building under MinGW-w64
├── src/
│   ├── CommandQueue.cpp       # modified CasHMC components
│   ├── CrossbarSwitch.cpp
│   ├── HMC.cpp
│   ├── VaultController.cpp
│   ├── MemoryAPI.cpp          # high-level read/write interface
│   ├── CasHMCDLL.cpp          # extern "C" entry points ModelSim calls into
│   └── testMemoryAPI.cpp      # Linux test executable (provides main())
├── test/
│   ├── tb.sv                  # SystemVerilog testbench
│   └── test_dll.cpp           # standalone DLL/SO test
└── packages/CasHMC/           # upstream CasHMC (git submodule, unmodified)
    └── sources/
```

`CasHMCDLL.cpp`/`CasHMCDLL.h` provide the `extern "C"` exported entry points ModelSim calls into via DPI-C, and are compiled into every build target (the Linux test executable, the Windows DLL, and the Linux shared object).

`mingw_posix_compat.h` is force-included (`-include`) into every file compiled for the Windows DLL build. It shims POSIX calls (e.g. `mkdir(path, mode)`) that don't exist under MinGW-w64, so the vendored `packages/CasHMC/sources/` submodule never needs to be edited to build under Windows — the fix lives entirely in this project's own tree and survives every fresh submodule checkout.

## 3. Building

The project uses GNU Make, a C++ compiler (`g++`), and — for the Windows DLL — the MinGW-w64 cross-compiler.

```bash
make clean
make
```

**`make` with no target builds everything unconditionally:**

1. Initializes/updates the `packages/CasHMC` git submodule (equivalent to `git submodule update --init --recursive`).
2. Builds the Linux test executable: `build/testMemoryAPI`.
3. Cross-compiles the Windows DLL: `build/cashmc.dll` (+ `build/libcashmc.a` import lib).
4. Builds the Linux shared object: `build/cashmc.so`.

If you only need one artifact, the individual targets still work standalone:

```bash
make submodules   # just fetch/update the CasHMC submodule
make dll           # build/cashmc.dll only
make so            # build/cashmc.so only
```

> Note: since `all` (the default target) now depends on `dll` and `so` unconditionally, a plain `make` requires MinGW-w64 to be installed (see 3.2) even if you only care about the Linux executable. Run `make so` if you want just the Linux shared object without the Windows toolchain, or `make dll` for just the DLL — neither pulls in `testMemoryAPI` or the other artifact.

Run the test executable:

```bash
make run
```

Clean up:

```bash
make clean       # remove build/ entirely
make distclean   # clean + deinit the CasHMC submodule
```

### 3.1 Native Linux build requirements

Just a standard C++17-capable `g++`. Nothing else to install for `testMemoryAPI` or `build/cashmc.so`.

### 3.2 Windows DLL build requirements (MinGW-w64)

No Windows machine or Visual Studio is required — `build/cashmc.dll` is cross-compiled from Linux with MinGW-w64.

```bash
sudo apt update
sudo apt install mingw-w64
```

Verify it's available:

```bash
x86_64-w64-mingw32-g++ --version
```

## 4. Build Artifacts

| Artifact | Target | Toolchain | Purpose |
|---|---|---|---|
| `build/testMemoryAPI` | `make` (default) | native `g++` | Linux CLI test of the MemoryAPI |
| `build/cashmc.dll` + `build/libcashmc.a` | `make dll` | `x86_64-w64-mingw32-g++` | ModelSim on **Windows** |
| `build/cashmc.so` | `make so` | native `g++ -fPIC -shared` | ModelSim on **Linux** |

The DLL and SO are built from the same source list (`DLL_ALL_SRC`: original CasHMC sources + modified components, including `CasHMCDLL.cpp`, + `MemoryAPI.cpp`) — just with a different compiler and linker flags. `vsim -sv_lib build/cashmc` works unchanged either way; ModelSim appends the correct platform extension (`.dll` or `.so`) automatically.

The Windows DLL statically links the MinGW runtime (`libstdc++`, `libgcc`, `winpthread`), so `cashmc.dll` has no external runtime DLL dependencies to ship alongside it.

### 4.1 Verify a build

**Windows DLL:**

```bash
file build/cashmc.dll
# expected: PE32+ executable (DLL), for MS Windows, x86-64

x86_64-w64-mingw32-objdump -p build/cashmc.dll | grep -A 20 "Export Table"
```

**Linux shared object:**

```bash
file build/cashmc.so
# expected: ELF 64-bit LSB shared object, x86-64

objdump -T build/cashmc.so | grep -i cashmc
```

The exported function names (for both) should match the `extern "C"` declarations in `include/CasHMCDLL.h`.

**Standalone DLL/SO test:**

```text
test/test_dll.cpp
```

## 5. Run with ModelSim

The SystemVerilog testbench is:

```text
test/tb.sv
```

Run ModelSim with the CasHMC library:

```tcl
vsim -sv_lib build/cashmc tb
```

ModelSim loads the library via DPI-C and resolves symbols by name, so the compiler used to build it does not matter as long as the exported functions are `extern "C"` and match the expected DPI signatures. On Windows this loads `cashmc.dll`; on Linux it loads `cashmc.so` — same `vsim` invocation either way.