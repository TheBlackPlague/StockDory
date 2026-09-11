CC  ?= clang
CXX ?= clang++
EXE ?= StockDory

ifeq ($(OS),Windows_NT)
    CP = powershell -Command "Copy-Item"
    RM = powershell -Command "Remove-Item -Recurse -Force"
    EXTENSION = .exe
    LLVM_PROFDATA = llvm-profdata
    SLASH = \\
else
    CP = cp
    RM = rm
    EXTENSION =
    LLVM_PROFDATA = llvm-profdata-20
    SLASH = /
endif

all: openbench

openbench:
ifdef EVALFILE
	$(RM) src/Engine/Model/* && \
	$(CP) $(EVALFILE) src/Engine/Model/
endif
	cmake -B RB -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -G Ninja -DPGO=True && \
	cmake --build RB --config Release && \
	RB$(SLASH)StockDory$(EXTENSION) bench && \
	$(LLVM_PROFDATA) merge -output=RB/pgo.profdata RB/pgo.profraw && \
	$(RM) RB/StockDory$(EXTENSION) && \
	cmake -B RB -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=$(CC) -DCMAKE_CXX_COMPILER=$(CXX) -G Ninja -DPGO=True && \
	cmake --build RB --config Release && \
	$(CP) RB/StockDory$(EXTENSION) $(EXE)$(EXTENSION)
