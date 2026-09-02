# ============================================================
# CasHMC - Modified Version with MemoryAPI
# ============================================================

CXX := g++

CXXFLAGS := -O3 -g -DDEBUG_LOG

CPPFLAGS := \
	-Iinclude \
	-Ipackages/CasHMC/sources

BUILD_DIR := build

# Separate top-level build trees per platform/toolchain, so Linux
# (native g++) and Windows (MinGW-w64) object files never mix.
LINUX_BUILD_DIR   := $(BUILD_DIR)/linux
WINDOWS_BUILD_DIR := $(BUILD_DIR)/windows

OBJ_DIR := $(LINUX_BUILD_DIR)/obj

TARGET := $(BUILD_DIR)/testMemoryAPI

# ============================================================
# Windows DLL build (MinGW-w64, replaces MSVC cl/link)
# ============================================================

MINGW_CXX := x86_64-w64-mingw32-g++

# -include forces the POSIX-compat shim into every translation
# unit before any vendored CasHMC header is parsed, so the
# pristine packages/CasHMC/sources/ tree (re-cloned from GitHub)
# never needs to be edited to build under MinGW.
MINGW_CXXFLAGS := -O2 -std=c++17 -DBUILDING_DLL -include include/MINGW_POSIX_Compat.h

WIN_OBJ_DIR  := $(WINDOWS_BUILD_DIR)/obj

DLL_TARGET := $(BUILD_DIR)/cashmc.dll

# ============================================================
# Linux shared object build (native g++, for ModelSim on Linux)
# ============================================================

SO_CXXFLAGS := -O2 -std=c++17 -fPIC -DBUILDING_DLL

SO_OBJ_DIR := $(LINUX_BUILD_DIR)/so_obj

SO_TARGET := $(BUILD_DIR)/cashmc.so

# ============================================================
# Original CasHMC source files
# ============================================================

CAS_HMC_SRC := $(wildcard packages/CasHMC/sources/*.cpp)

# Remove:
#   1. CasHMC files that were modified
#   2. RunSim.cpp because testMemoryAPI.cpp provides main()
CAS_HMC_SRC := $(filter-out \
	packages/CasHMC/sources/CommandQueue.cpp \
	packages/CasHMC/sources/CrossbarSwitch.cpp \
	packages/CasHMC/sources/HMC.cpp \
	packages/CasHMC/sources/VaultController.cpp \
	packages/CasHMC/sources/RunSim.cpp, \
	$(CAS_HMC_SRC))

# ============================================================
# Modified CasHMC source files
# ============================================================

MODIFIED_SRC := \
	src/CommandQueue.cpp \
	src/CrossbarSwitch.cpp \
	src/HMC.cpp \
	src/VaultController.cpp \
	src/CasHMCDLL.cpp

# ============================================================
# New Memory API
# ============================================================

MEMORY_API_SRC := \
	src/MemoryAPI.cpp

# ============================================================
# Test program
# ============================================================

TEST_SRC := \
	src/testMemoryAPI.cpp

# All sources that go into the DLL/SO: original + modified CasHMC
# components (which now includes CasHMCDLL.cpp, the extern "C"
# entry point ModelSim calls into) and the MemoryAPI.
# testMemoryAPI.cpp (which provides main()) is intentionally excluded.
DLL_ALL_SRC := \
	$(CAS_HMC_SRC) \
	$(MODIFIED_SRC) \
	$(MEMORY_API_SRC)

# ============================================================
# All source files
# ============================================================

SRC := \
	$(CAS_HMC_SRC) \
	$(MODIFIED_SRC) \
	$(MEMORY_API_SRC) \
	$(TEST_SRC)

# Convert source paths to object paths
OBJ := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

# Convert DLL source paths to object paths (MinGW build)
WIN_OBJ := $(DLL_ALL_SRC:%.cpp=$(WIN_OBJ_DIR)/%.o)

# Convert DLL source paths to object paths (native Linux .so build)
SO_OBJ := $(DLL_ALL_SRC:%.cpp=$(SO_OBJ_DIR)/%.o)

# ============================================================
# Build
# ============================================================

all: submodules $(TARGET) dll so

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo ""
	@echo "========================================"
	@echo " CasHMC MemoryAPI test build successful"
	@echo "========================================"
	@echo "Executable: $(TARGET)"
	@echo ""

submodules:
	@git submodule update --init --recursive

# ============================================================
# Compile .cpp -> .o
# ============================================================

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# ============================================================
# Windows DLL build (MinGW-w64)
# ============================================================
# Usage: make dll
# Produces build/cashmc.dll + build/libcashmc.a (import lib)
#
# The MinGW runtime (libstdc++, libgcc, winpthread) is statically
# linked into the DLL so no extra runtime DLLs need to ship
# alongside cashmc.dll for ModelSim to load it.

dll: $(DLL_TARGET)

$(DLL_TARGET): $(WIN_OBJ)
	@mkdir -p $(dir $@)
	$(MINGW_CXX) -shared -o $@ $^ \
		-Wl,--out-implib,$(BUILD_DIR)/libcashmc.a \
		-static-libgcc -static-libstdc++ -static -lwinpthread
	@echo ""
	@echo "========================================"
	@echo " CasHMC Windows DLL build successful"
	@echo "========================================"
	@echo "DLL:         $(DLL_TARGET)"
	@echo "Import lib:  $(BUILD_DIR)/libcashmc.a"
	@echo ""

$(WIN_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(MINGW_CXX) $(CPPFLAGS) $(MINGW_CXXFLAGS) -c $< -o $@

# ============================================================
# Linux shared object build (native g++)
# ============================================================
# Usage: make so
# Produces build/cashmc.so, for ModelSim running on Linux.
# vsim's -sv_lib option appends the platform-appropriate
# extension automatically, so the same "-sv_lib build/cashmc"
# invocation used on Windows also works here.

so: $(SO_TARGET)

$(SO_TARGET): $(SO_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) -shared -o $@ $^
	@echo ""
	@echo "========================================"
	@echo " CasHMC Linux .so build successful"
	@echo "========================================"
	@echo "Shared object: $(SO_TARGET)"
	@echo ""

$(SO_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(SO_CXXFLAGS) -c $< -o $@

# ============================================================
# Clean
# ============================================================
# run: $(TARGET)
# 	./$(TARGET)
run: $(TARGET)
	cd packages/CasHMC && ../../$(TARGET)
	
clean:
	rm -rf $(BUILD_DIR)

distclean: clean
	@git submodule deinit -f --all

.PHONY: all run clean dll so distclean submodules