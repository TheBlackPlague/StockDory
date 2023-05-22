all: openbench

openbench:
	cmake -B ReleaseBuild -DCMAKE_BUILD_TYPE=Release && \
	cmake --build ReleaseBuild --config Release && \
	cp ReleaseBuild/StockDory StockDory