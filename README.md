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
