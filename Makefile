.PHONY: all build clean

all: build

build:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
	cmake --build build -j

clean:
	cd build && make clean 2>/dev/null || true