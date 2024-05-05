cmake -S ./python-runtime_deps -B ./build_deps/python-runtime_deps
cmake --build ./build_deps/python-runtime_deps --config Release -j %NUMBER_OF_PROCESSORS%
