cmake -S ./python-runtime_deps -B ./build_deps/python-runtime_deps
make -C ./build_deps/python-runtime_deps -j 10
rm -rf ./build_deps/python-runtime_deps
