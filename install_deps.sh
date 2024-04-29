cmake -S ./pyrunner_deps -B ./build_deps/pyrunner_deps
make -C ./build_deps/pyrunner_deps -j 10
rm -rf ./build_deps/pyrunner_deps
