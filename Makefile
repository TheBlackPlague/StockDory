# === Configuration ===

# User-overridable variables
ifeq ($(origin CXX), default)
    CXX = clang++
endif
EXE ?= StockDory

# Detect OS and set environment-specific variables
ifeq ($(OS),Windows_NT)
    CP             = powershell -Command "Copy-Item -Force"
    RM             = powershell -Command "Remove-Item -Recurse -Force"
    EXT            = .exe
    LLVM_PROFDATA ?= llvm-profdata
    SLASH          = \\
else
    CP    = cp
    RM    = rm -rf
    EXT   =
    SLASH = /

    LLVM_PROFDATA ?= $(shell \
        CXX_PATH="$(command -v $(CXX))"; \
        CXX_DIR="$(dirname "$CXX_PATH")"; \
        CXX_VERSION="$(basename "$CXX_PATH" | sed -n 's/.*-\([0-9][0-9]*\)$/\1/p')"; \
        if [ -n "$CXX_VERSION" ] && [ -x "$CXX_DIR/llvm-profdata-$CXX_VERSION" ]; then \
            echo "$CXX_DIR/llvm-profdata-$CXX_VERSION"; \
        elif [ -x "$CXX_DIR/llvm-profdata" ]; then \
            echo "$CXX_DIR/llvm-profdata"; \
        elif [ -n "$CXX_VERSION" ] && command -v "llvm-profdata-$CXX_VERSION" >/dev/null 2>&1; then \
            command -v "llvm-profdata-$CXX_VERSION"; \
        elif command -v llvm-profdata >/dev/null 2>&1; then \
            command -v llvm-profdata; \
        else \
            echo llvm-profdata; \
        fi)
endif

# === Targets ===

all: openbench

openbench:
ifdef EVALFILE
	$(RM) src/Engine/Model/* && \
	$(CP) $(EVALFILE) src/Engine/Model/Network.nnue
endif
	@echo "[*] Performing initial build for profiling..."
	cmake -B Build -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_CXX_COMPILER=$(CXX) \
		-DBUILD_PGO=ON
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
    	-DCMAKE_CXX_COMPILER=$(CXX) \
    	-DBUILD_PGO=ON
	cmake --build Build --config Release

	@echo "[*] Copying final binary to root directory..."
	$(CP) Build$(SLASH)StockDory$(EXT) $(EXE)$(EXT)

# === Utility Targets ===

clean:
	$(RM) Build
	$(RM) $(EXE)$(EXT)

.PHONY: all openbench clean
