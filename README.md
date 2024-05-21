# cortex.python
**Python embedding on C++**

Each python execution request will create a new child process for python runtime by using `<process.h>` on Windows and `<spawn.h>` on UNIX (Linux and MacOS).

## I. Installation:

### Linux and MacOS
1. Install dependencies:
```bash
# In cortex.python/ root dir
./install_deps.sh
```

2. Build the project
```bash
# In cortex.python/ root dir
mkdir build && cd build
cmake ..
make -j32
```

### Windows (testing)
1. Install dependencies
```powershell
# In cortex.python\ root dir
cmake -S ./third-party -B ./build_deps/third-party
cmake --build ./build_deps/third-party --config Release
```

2. Build the project
```powershell
# In cortex.python\ root dir
mkdir -p build
cd build
cmake ..
cmake --build . --config Release -j {NUMBER_OF_PROCESSORS}
# Replace NUMBER_OF_PROCESSORS with an actual number
```

3. Copy the `.../build/python/` folder to `.../build/Release/python/` (`python-runtime.exe` and `python/` must be in the same folder)

## II. Run the program:

### Linux and MacOS
Go to the `.../build/` directory.

While running the python-runtime with no argument, it will use the default `.../build/python/` library
```
~/cortex.python/build$ ./python-runtime
```
Or, you can use your desired Python3 version by adding a path to the directory that contain the `libpython***.so` or `libpython***.dylib` file.
```
~/cortex.python/build$ ./python-runtime {PATH_TO_DYNAMIC_LIB_DIR}
```

Then input the path to your Python file that need to be executed. You can input **multiple times**, each input will create a new child process for Python runtime.

### Windows
Go to the `...\build\Release\` folder.

While running the python-runtime with no argument, it will use the default `.../build/Release/python/` library 
```
> .\python-runtime.exe
```

Or, you can use your desired Python3 version by adding a path to the folder that contain the `python***.dll` file.
```
> .\python-runtime.exe {PATH_TO_DYNAMIC_LIB_DIR}
```

Then input the path to your Python file that need to be executed. You can input **multiple times**, each input will create a new child process for Python runtime.

## III. Install a new package in default library `/site-packages/`
Updating... 🙏 
### Linux
```bash
# In cortex.python/ root dir
cd ./build/python/
export PYTHONHOME=$(pwd)
./bin/python3 -m ensurepip
./bin/python3 -m pip install --upgrade pip
./bin/python3 -m pip install numpy --target={PATH_TO_python-runtime_ROOT_DIR}/build/python/lib/python/site-packages/
```
### Windows
```powershell
cd .\build\python\
.\python.exe -m ensurepip
.\python.exe -m pip install --upgrade pip
.\python.exe -m pip install numpy --target={PATH_TO_python-runtime_ROOT_DIR}\build\python\lib\python\site-packages\
```

## IV. Example run:

Let's use python-runtime to execute 2 this python file in `/home/jan/cortex/main.py`.
```python
import sys
for path in sys.path:
    print(path)
print("Hello from Cortex!")
```

### Linux

```
# Default library
~/cortex.python/build$ ./python-runtime
python-runtime started

/home/jan/cortex/main.py
Created child process for Python embedding
No specified Python library path, using default Python library in /home/jan/cortex.python/build/python/
Found dynamic library file /home/jan/cortex.python/build/python/libpython3.10.so.1.0
Successully loaded Python dynamic library from file: /home/jan/cortex.python/build/python/libpython3.10.so.1.0
Trying to run Python file in path /home/jan/cortex/main.py

/home/jan/cortex.python/build/python/lib/python/site-packages/
/home/jan/cortex.python/build/python/lib/python/lib-dynload/
/home/jan/cortex.python/build/python/lib/python/
Hello from Cortex!

/home/jan/cortex/main.py
Created child process for Python embedding
No specified Python library path, using default Python library in /home/jan/cortex.python/build/python/
Found dynamic library file /home/jan/cortex.python/build/python/libpython3.10.so.1.0
Successully loaded Python dynamic library from file: /home/jan/cortex.python/build/python/libpython3.10.so.1.0
Trying to run Python file in path /home/jan/cortex/main.py

/home/jan/cortex.python/build/python/lib/python/site-packages/
/home/jan/cortex.python/build/python/lib/python/lib-dynload/
/home/jan/cortex.python/build/python/lib/python/
Hello from Cortex!
```

```
# With specific python version
~/cortex.python/build$ ./python-runtime /usr/lib/python3.10/config-3.10-x86_64-linux-gnu/
python-runtime started

/home/jan/cortex/main.py
Created child process for Python embedding
Found dynamic library file /usr/lib/python3.10/config-3.10-x86_64-linux-gnu/libpython3.10.so
Successully loaded Python dynamic library from file: /usr/lib/python3.10/config-3.10-x86_64-linux-gnu/libpython3.10.so
Trying to run Python file in path /home/jan/cortex/main.py

/usr/lib/python310.zip
/usr/lib/python3.10
/usr/lib/python3.10/lib-dynload
/usr/local/lib/python3.10/dist-packages
/usr/lib/python3/dist-packages
Hello from Cortex!

/home/jan/cortex/main.py
Created child process for Python embedding
Found dynamic library file /usr/lib/python3.10/config-3.10-x86_64-linux-gnu/libpython3.10.so
Successully loaded Python dynamic library from file: /usr/lib/python3.10/config-3.10-x86_64-linux-gnu/libpython3.10.so
Trying to run Python file in path /home/jan/cortex/main.py

/usr/lib/python310.zip
/usr/lib/python3.10
/usr/lib/python3.10/lib-dynload
/usr/local/lib/python3.10/dist-packages
/usr/lib/python3/dist-packages
Hello from Cortex!
```

## V. Troubleshooting

### Linux
1. Missing `_ctypes` files:
   
Your `/build_deps/_install/` directory is broken, please reinstall `libffi` using `install_deps.sh` and then build the project again.

### MacOS
1. SSL certificate error:
   
Your `/build_deps/_install/` directory is broken, please reinstall `ssl` using `install_deps.sh` and then build the project again.

### Windows
1. Could not find some missing Python files
   
Your `/build_deps/_install/python` directory is broken, this may caused by manually deleting python files in `/build_deps/_install/python`. Since Python library on Windows is intalled using binary installer, your system may cache some of Python data.

Please use the Python Installer in `/build_deps/_install/python/python-installer.exe` to **repair** then **uninstall** Python. After that re-install the dependencies and re-build the project again.
