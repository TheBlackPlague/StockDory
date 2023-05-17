all: clean compile move

clean:
	rm -rf ReleaseBuild
	rm -rf StockDory

compile:
	cmake -B ReleaseBuild -DCMAKE_BUILD_TYPE=Release
	cmake --build ReleaseBuild --config Release

move:
	cp ReleaseBuild/StockDory StockDory