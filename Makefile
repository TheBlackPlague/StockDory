ifeq ($(OS),Windows_NT)
    CP = powershell cp
    RM = powershell rm
    EXTENSION = .exe
else
    CP = cp
    RM = rm
    EXTENSION =
endif

all: openbench

openbench:
ifdef EVALFILE
	$(RM) -rf src/Engine/Model/* && \
	$(CP) $(EVALFILE) src/Engine/Model/
endif
	cmake -B RB -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja && \
	cmake --build RB --config Release && \
	$(CP) RB/StockDory$(EXTENSION) StockDory$(EXTENSION)
