# === Configuration ===

# User-overridable variables
CC  ?= clang
CXX ?= clang++
EXE ?= StockDory

# Detect OS and set environment-specific variables
ifeq ($(OS),Windows_NT)
    CP            = powershell -Command "Copy-Item -Force"
    RM            = powershell -Command "Remove-Item -Recurse -Force"
    EXT           = .exe
    LLVM_PROFDATA = llvm-profdata
    SLASH         = \\
else
    CP            = cp
    RM            = rm -rf
    EXT           =
    LLVM_PROFDATA = llvm-profdata-20
    SLASH         = /
endif

# === Targets ===

all: openbench

openbench:
ifdef EVALFILE
	$(RM) src/Engine/Model/* && \
	$(CP) $(EVALFILE) src/Engine/Model/
endif
	@echo "[*] Performing initial build for profiling..."
	cmake -B Build -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DBUILD_NATIVE=ON \
		-DPGO=ON
	cmake --build Build --config Release

	@echo "[*] Running benchmark to generate profiling data..."
	Build$(SLASH)StockDory$(EXT) bench

	@echo "[*] Merging profiling data..."
	$(LLVM_PROFDATA) merge -output=Build/pgo.profdata Build/pgo.profraw
	$(RM) Build$(SLASH)StockDory$(EXT)
	$(RM) Build$(SLASH)pgo.profraw

	@echo "[*] Performing optimized build with profiling data..."
	cmake -B Build -G Ninja \
    	-DCMAKE_BUILD_TYPE=Release \
    	-DCMAKE_C_COMPILER=$(CC) \
    	-DCMAKE_CXX_COMPILER=$(CXX) \
    	-DBUILD_NATIVE=ON \
    	-DPGO=ON
	cmake --build Build --config Release

	@echo "[*] Copying final binary to root directory..."
	$(CP) Build$(SLASH)StockDory$(EXT) $(EXE)$(EXT)

# === Utility Targets ===

clean:
	$(RM) Build
	$(RM) $(EXE)$(EXT)

.PHONY: all openbench clean