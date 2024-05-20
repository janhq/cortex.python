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
	@powershell -Command "mkdir -p .\examples\server\build; cd .\examples\server\build; cmake .. $(CMAKE_EXTRA_FLAGS); cmake --build . --config Release; cp -r ..\..\..\build\python .\Release\python"
else
	mkdir -p examples/server/build
	cd examples/server/build && cmake .. && cmake --build . --config Release -j12
	cp -r build/python examples/server/build
endif

package:
ifeq ($(OS),Windows_NT)
	@powershell -Command "mkdir -p cortex.python-runtime; cp build\Release\engine.dll cortex.python-runtime\; cp -r build\python cortex.python-runtime\; 7z a -ttar temp.tar cortex.python-runtime\*; 7z a -tgzip cortex.python-runtime.tar.gz temp.tar;"
else
	@mkdir -p cortex.python-runtime && \
	cp build/libengine.$(shell uname | tr '[:upper:]' '[:lower:]' | sed 's/darwin/dylib/;s/linux/so/') cortex.python-runtime && \
	cp -r build/python cortex.python-runtime
	tar -czvf cortex.python-runtime.tar.gz cortex.python-runtime
endif

run-e2e-test:
ifeq ($(RUN_TESTS),false)
	@echo "Skipping tests"
else
ifeq ($(OS),Windows_NT)
	@mkdir examples\\server\\build\\Release\\engines\\cortex.python-runtime && \
	cmd /C "cd examples\\server\\build\\Release && copy ..\\..\\..\\..\\build\\Release\\engine.dll engines\\cortex.python-runtime && ..\\..\\..\\..\\.github\\scripts\\e2e-test-server-windows.bat server.exe ..\\..\\..\\..\\$(PYTHON_FILE_EXECUTION_PATH)"
else
	@mkdir -p examples/server/build/engines/cortex.python-runtime && \
	cd examples/server/build && \
	cp ../../../build/libengine.$(shell uname | tr '[:upper:]' '[:lower:]' | sed 's/darwin/dylib/;s/linux/so/') engines/cortex.python-runtime/ && \
	chmod +x ../../../.github/scripts/e2e-test-server-linux-and-mac.sh && ../../../.github/scripts/e2e-test-server-linux-and-mac.sh ./server ../../../$(PYTHON_FILE_EXECUTION_PATH)
endif
endif

clean:
ifeq ($(OS),Windows_NT)
	cmd /C "rmdir /S /Q build examples\\server\\build cortex.python-runtime cortex.python-runtime.tar.gz cortex.python-runtime.zip"
else
	rm -rf build examples/server/build cortex.python-runtime cortex.python-runtime.tar.gz cortex.python-runtime.zip
endif
