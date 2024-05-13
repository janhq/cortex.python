# Makefile for Cortex python-runtime engine - Build, Lint, Test, and Clean

CMAKE_EXTRA_FLAGS ?= ""
RUN_TESTS ?= false
PYTHON_FILE_EXECUTION_PATH ?= ".github/scripts/python-file-to-test.py"

# Default target, does nothing
all:
	@echo "Specify a target to run"

# Build the Cortex python-runtime engine
build-lib:
ifeq ($(OS),Windows_NT) # Windows
	@powershell -Command "cmake -S ./third-party -B ./build_deps/third-party;"
	@powershell -Command "cmake --build ./build_deps/third-party --config Release -j8;"
	@powershell -Command "mkdir -p build && cd build && cmake .. $(CMAKE_EXTRA_FLAGS) && cmake --build . --config Release;"
else  # Unix-like systems (Linux and MacOS)
	@cmake -S ./third-party -B ./build_deps/third-party
	@make -C ./build_deps/third-party -j8
	# The following mkdir command will now work properly by changing to the directory only once.
	@mkdir -p build && cd build && \
	cmake .. $(CMAKE_EXTRA_FLAGS) && \
	make -j8
endif

build-example-server: build-lib
ifeq ($(OS),Windows_NT)
	@powershell -Command "mkdir -p .\examples\server\build && cd .\examples\server\build && cmake .. $(CMAKE_EXTRA_FLAGS) && cmake --build . --config Release -j8;"
else
	@mkdir -p examples/server/build && cd examples/server/build && \
	cmake .. $(CMAKE_EXTRA_FLAGS) && \
	cmake --build . --config Release -j8
endif

package:
ifeq ($(OS),Windows_NT)
	@powershell -Command "mkdir -p cortex.python-runtime && cp build\Release\engine.dll cortex.python-runtime\ && 7z a -ttar temp.tar cortex.python-runtime\* && 7z a -tgzip cortex.python-runtime.tar.gz temp.tar;"
else
	@mkdir -p cortex.python-runtime && \
	cp build/libengine.$(shell uname | tr '[:upper:]' '[:lower:]' | sed 's/darwin/dylib/;s/linux/so/') cortex.python-runtime/ && \
	tar -czvf cortex.python-runtime.tar.gz cortex.python-runtime
endif

run-e2e-test:
ifeq ($(RUN_TESTS),false)
	@echo "Skipping tests"
else
ifeq ($(OS),Windows_NT)
	@powershell -Command "mkdir -p examples\server\build\Release\engines\cortex.python-runtime && cd examples\server\build\Release && cp ..\..\..\..\build\Release\engine.dll engines\cortex.python-runtime && ..\..\..\..\.github\scripts\e2e-test-server-windows.bat server.exe ..\..\..\$(PYTHON_FILE_EXECUTION_PATH);"
else
	@mkdir -p examples/server/build/engines/cortex.python-runtime && \
	cd examples/server/build && \
	cp ../../../build/libengine.$(shell uname | tr '[:upper:]' '[:lower:]' | sed 's/darwin/dylib/;s/linux/so/') engines/cortex.python-runtime/ && \
	chmod +x ../../../.github/scripts/e2e-test-server-linux-and-mac.sh && ../../../.github/scripts/e2e-test-server-linux-and-mac.sh ./server ../../../$(PYTHON_FILE_EXECUTION_PATH)
endif
endif

