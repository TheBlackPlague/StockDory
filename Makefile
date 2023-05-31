ifeq ($(OS),Windows_NT)
    CP = powershell cp
	EXTENSION = .exe
else
    CP = cp
	EXTENSION = 
endif

all: openbench

openbench:
	cmake -B RB -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -G Ninja && \
	cmake --build RB --config Release && \
	$(CP) RB/StockDory$(EXTENSION) StockDory$(EXTENSION)