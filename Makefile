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
	cmake -B Build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja -DBUILD_NATIVE=ON -DPGO=ON && \
	cmake --build Build --config Release && \
	RB$(SLASH)StockDory$(EXTENSION) bench && \
	$(LLVM_PROFDATA) merge -output=Build/pgo.profdata Build/pgo.profraw && \
	$(RM) Build/StockDory$(EXTENSION) && \
	cmake -B Build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja -DBUILD_NATIVE=ON -DPGO=ON && \
	cmake --build Build --config Release && \
	$(CP) Build/StockDory$(EXTENSION) StockDory$(EXTENSION)
