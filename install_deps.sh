cmake -S ./third-party -B ./build_deps/third-party
export LD_LIBRARY_PATH=$(pwd)/build_deps/_install/lib64/
make -C ./build_deps/third-party -j 10
rm -rf ./build_deps/third-party
