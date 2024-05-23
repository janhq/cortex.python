# Makefile for Cortex python-runtime engine - Build, Lint, Test, and Clean

CMAKE_EXTRA_FLAGS ?= ""
RUN_TESTS ?= true
PYTHON_FILE_EXECUTION_PATH ?= .github/scripts/python-file-to-test.py

# Default target, does nothing
all:
	@echo "Specify a target to run"

# Build the Cortex python-runtime engine
install-dependencies:
ifeq ($(OS),Windows_NT) # Windows
	cmd /C install_deps.bat
else  # Unix-like systems (Linux and MacOS)
	bash ./install_deps.sh
endif

build-engine:
ifeq ($(OS),Windows_NT)
	@powershell -Command "mkdir -p build; cd build; cmake .. $(CMAKE_EXTRA_FLAGS); cmake --build . --config Release;"
else
	mkdir -p build
	cd build && cmake .. && cmake --build . --config Release -j12
endif

build-example-server:
ifeq ($(OS),Windows_NT)
	@powershell -Command "mkdir -p .\examples\server\build\engines\cortex.python; cd .\examples\server\build; cmake .. $(CMAKE_EXTRA_FLAGS); cmake --build . --config Release; cp -r ..\..\..\build\python .\Release\engines\cortex.python\python"
else 
	@mkdir -p examples/server/build/engines/cortex.python; \
	cp -rf build_deps/_install/lib/engines-3 build/python/; \
	cp -rf build_deps/_install/lib/libcrypto.* build/python/; \
	cp -rf build_deps/_install/lib/libssl.* build/python/; \
	cp -rf build_deps/_install/lib/ossl-modules build/python/; \
	cp -r build/python examples/server/build/engines/cortex.python; \
	ls examples/server/build/engines/cortex.python/python;
	@cd examples/server/build; \
	cmake .. && cmake --build . --config Release -j12
endif

package:
ifeq ($(OS),Windows_NT)
	@powershell -Command "mkdir -p cortex.python; cp build\Release\engine.dll cortex.python\; cp -r build\python cortex.python\; 7z a -ttar temp.tar cortex.python\*; 7z a -tgzip cortex.python.tar.gz temp.tar;"
else
	@mkdir -p cortex.python && \
	cp build/libengine.$(shell uname | tr '[:upper:]' '[:lower:]' | sed 's/darwin/dylib/;s/linux/so/') cortex.python && \
	cp -r build/python cortex.python
	tar -czvf cortex.python.tar.gz cortex.python
endif

run-e2e-test:
ifeq ($(RUN_TESTS),false)
	@echo "Skipping tests"
else
ifeq ($(OS),Windows_NT)
	@powershell -Command "mkdir -p .\examples\server\build\Release\engines\cortex.python; cd examples\server\build\Release; cp ..\..\..\..\build\Release\engine.dll engines\cortex.python; ..\..\..\..\.github\scripts\e2e-test-server-windows.bat server.exe ..\..\..\..\$(PYTHON_FILE_EXECUTION_PATH);"
else
	@mkdir -p examples/server/build/engines/cortex.python && \
	rm -rf build_deps && \
	cd examples/server/build && \
	cp ../../../build/libengine.$(shell uname | tr '[:upper:]' '[:lower:]' | sed 's/darwin/dylib/;s/linux/so/') engines/cortex.python/ && \
	chmod +x ../../../.github/scripts/e2e-test-server-linux-and-mac.sh && ../../../.github/scripts/e2e-test-server-linux-and-mac.sh ./server ../../../$(PYTHON_FILE_EXECUTION_PATH)
endif
endif

clean:
ifeq ($(OS),Windows_NT)
	cmd /C "rmdir /S /Q build examples\\server\\build cortex.python cortex.python.tar.gz cortex.python.zip"
else
	rm -rf build examples/server/build cortex.python cortex.python.tar.gz cortex.python.zip
endif
