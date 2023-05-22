all: openbench

openbench:
	cmake -B ReleaseBuild -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ && \
	cmake --build ReleaseBuild --config Release && \
	cp ReleaseBuild/StockDory StockDory