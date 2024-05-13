hello-cortex:
ifeq ($(OS),Windows_NT)
	@echo "Hello Windows";
else ifeq ($(shell uname -s),Linux)
	@echo "Hello Linux";
else
	@echo "Hello MacOS";
endif
