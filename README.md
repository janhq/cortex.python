### cortex.pyrunner
Python embedding on C++

## Installation:

# Linux and MacOS
1. Install dependencies:
```
# In cortex.pyrunner root dir
./install_deps.sh
```

1. Build the project
```
# In cortex.pyrunner root dir
mkdir build && cd build
cmake ..
make -j32
```

# Windows (testing)
1. Install dependencies
```
# In cortex.pyrunner root dir
cmake -S ./pyrunner_deps -B ./build_deps/pyrunner_deps
cmake --build ./build_deps/pyrunner_deps --config Release
```

1. Build the project
```
# In cortex.pyrunner root dir
mkdir -p build
cd build
cmake ..
cmake --build . --config Release -j {NUMBER_OF_PROCESSORS}
# Replace NUMBER_OF_PROCESSORS with an actual number
```

1. Copy the `.../build/python/` folder to `.../build/Release/python/`.
`pyrunner.exe` and `python/` must be in the same folder.

## Run the program:

# Linux and MacOS
Go to the `.../build/` directory.
While running the pyrunner with no argument, it will use the default `.../build/python/` library
`./pyrunner`
Or, you can use your desired Python3 version by adding a path to the directory that contain the `libpython***.so` or `libpython***.dylib` file.
`./pyrunner {PATH_TO_DYNAMIC_LIB_DIR}`
Then input the path to your Python file that need to be executed.

# Windows
Go to the `.../build/Release/` folder.
While running the pyrunner with no argument, it will use the default `.../build/Release/python/` library 
`./pyrunner.exe`
Or, you can use your desired Python3 version by adding a path to the folder that contain the `python***.dll` file.
`./pyrunner.exe {PATH_TO_DYNAMIC_LIB_DIR}`
Then input the path to your Python file that need to be executed.

## Run examples

Let's use pyrunner to use this python file in `/home/jan/cortex/main.py`.
```python
import sys
for path in sys.path:
    print(path)
print("Hello from Cortex!")
```

# Linux
| Default library | With specifict .so file |
|------------------|------------------|
|
~/cortex.pyrunner/build$ ./pyrunner
Pyrunner started

/home/jan/cortex/main.py
Created child process for Python embedding
**No specified Python library path, using default Python library in /home/jan/cortex.pyrunner/build/python/**
Found dynamic library file /home/jan/cortex.pyrunner/build/python/libpython3.10.so.1.0
Successully loaded Python dynamic library from file: /home/jan/cortex.pyrunner/build/python/libpython3.10.so.1.0
Trying to run Python file in path /home/jan/cortex/main.py

/home/jan/cortex.pyrunner/build/python/lib/python/site-packages/
/home/jan/cortex.pyrunner/build/python/lib/python/lib-dynload/
/home/jan/cortex.pyrunner/build/python/lib/python/
Hello from Cortex!
|
```
~/cortex.pyrunner/build$ ./pyrunner /usr/lib/python3.10/config-3.10-x86_64-linux-gnu/
Pyrunner started

/home/jan/cortex/main.py
Created child process for Python embedding
**Found dynamic library file /usr/lib/python3.10/config-3.10-x86_64-linux-gnu/libpython3.10.so**
Successully loaded Python dynamic library from file: /usr/lib/python3.10/config-3.10-x86_64-linux-gnu/libpython3.10.so
Trying to run Python file in path /home/jan/cortex/main.py

/usr/lib/python310.zip
/usr/lib/python3.10
/usr/lib/python3.10/lib-dynload
/usr/local/lib/python3.10/dist-packages
/usr/lib/python3/dist-packages
Hello from Cortex!
```
|

