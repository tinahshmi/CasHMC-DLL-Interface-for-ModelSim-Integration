# CasHMC-DLL-Interface-for-ModelSim-Integration

## 1. Project Overview

CasHMC is a cycle-accurate simulator for the Hybrid Memory Cube (HMC) architecture.

In the original CasHMC implementation, memory requests are generated through the simulator's internal transaction and packet mechanisms. In this project, selected CasHMC components have been modified to provide a simpler application-level interface.

The project is currently developed in two stages:

1. **Modified CasHMC + MemoryAPI** — provides a high-level read/write interface and validates memory transactions through the CasHMC simulation model.
2. **ModelSim DLL Interface** — provides an external interface for integrating the HMC model with ModelSim-based hardware simulations.

## 2. Modified CasHMC Components

The original CasHMC source files are kept in:

```text
packages/CasHMC/sources/
```

Selected components have been modified and placed in:

```text
src/
```

Currently modified components include:

```text
src/
├── CommandQueue.cpp
├── CrossbarSwitch.cpp
├── HMC.cpp
├── MemoryAPI.cpp
├── VaultController.cpp
└── testMemoryAPI.cpp
```

## 3. Building and Running the Modified CasHMC

The project uses GNU Make and a C++ compiler such as `g++`.

From the project root:

```bash
make clean
make
make run
```

The `make` command builds the modified CasHMC and generates the test executable:

```text
build/testMemoryAPI
```

The `make run` command runs the `testMemoryAPI` application and executes the memory transaction test.

## 4. Windows DLL Build and ModelSim Integration

The `winbuild/` directory contains the Windows/MSVC build artifacts used for ModelSim integration, including the CasHMC `.obj` files, `cashmc.dll`, and test files.

### 4.1 Build the Object Files

Open **x64 Native Tools Command Prompt for VS 2019** and navigate to the CasHMC project root.

Compile the required source files:

```bat
cl /nologo /EHsc /std:c++17 /MD /I sources /c sources\CasHMC_file_name.cpp /Fo:winbuild\CasHMC_file_name.obj
```

Replace `CasHMC_file_name` with the corresponding source file name.

### 4.2 Build the DLL

Link the object files using the Microsoft linker:

```bat
link /DLL /OUT:winbuild\cashmc.dll /IMPLIB:winbuild\cashmc.lib ^
winbuild\BankState.obj ^
winbuild\Packet.obj ^
winbuild\CommandQueue.obj ^
winbuild\ConfigReader.obj ^
winbuild\DRAM.obj ^
winbuild\DRAMCommand.obj ^
winbuild\CrossbarSwitch.obj ^
winbuild\HMC.obj ^
winbuild\HMCController.obj ^
winbuild\Link.obj ^
winbuild\LinkMaster.obj ^
winbuild\LinkSlave.obj ^
winbuild\MemoryAPI.obj ^
winbuild\VaultController.obj ^
winbuild\Transaction.obj ^
winbuild\CasHMCDLL.obj
```

This generates:

```text
winbuild/cashmc.dll
winbuild/cashmc.lib
winbuild/cashmc.exp
```

### 4.3 Test the DLL

Run the standalone Windows test:

```bat
winbuild\test_dll.exe
```

### 4.4 Run with ModelSim

The SystemVerilog testbench is:

```text
winbuild/tb.sv
```

Run ModelSim with the CasHMC DLL:

```tcl
vsim -sv_lib winbuild/cashmc tb
```
