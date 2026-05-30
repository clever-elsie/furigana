.PHONY: all build clean

all: build

build:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDEFAULT_SCAN_PATH=${DEFAULT_SCAN_PATH}
	cmake --build build -j

clean:
	cd build && make clean 2>/dev/null || true