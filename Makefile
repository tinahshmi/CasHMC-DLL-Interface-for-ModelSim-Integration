# ============================================================
# CasHMC - Modified Version with MemoryAPI
# ============================================================

CXX := g++

CXXFLAGS := -O3 -g -DDEBUG_LOG

CPPFLAGS := \
	-Iinclude \
	-Ipackages/CasHMC/sources

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj

TARGET := $(BUILD_DIR)/testMemoryAPI


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
	src/VaultController.cpp


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


# ============================================================
# Build
# ============================================================

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^
	@echo ""
	@echo "========================================"
	@echo " CasHMC MemoryAPI test build successful"
	@echo "========================================"
	@echo "Executable: $(TARGET)"
	@echo ""


# ============================================================
# Compile .cpp -> .o
# ============================================================

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@


# ============================================================
# Clean
# ============================================================
# run: $(TARGET)
# 	./$(TARGET)
run: $(TARGET)
	cd packages/CasHMC && ../../$(TARGET)
	
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
