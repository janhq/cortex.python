#pragma once

#include <dlfcn.h>
#include <csignal>
#include <filesystem>
#include <regex>
#include <string>
#include <iostream>

#include "trantor/utils/Logger.h"

#if defined(_WIN32)
  #define PY_DL HMODULE
  #define PY_LOAD_LIB(path) LoadLibraryW(PythonRuntimeUtils::stringToWString(path).c_str());
  #define GET_PY_FUNC GetProcAddress
  #define PY_FREE_LIB FreeLibrary
#else
  #define PY_DL void*
  #define PY_LOAD_LIB(path) dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
  #define GET_PY_FUNC dlsym
  #define PY_FREE_LIB dlclose
  extern char **environ; // Environment variable for posix_spawn
#endif

#ifdef _WIN32
  #include <winsock2.h>
  #include <windows.h>
#else
  #include <dirent.h>
  #include <unistd.h>
#endif

#if __APPLE__
#include <limits.h>
#include <mach-o/dyld.h>
#endif

namespace PythonRuntimeUtils {

typedef long Py_ssize_t;
typedef struct _object PyObject;
typedef void (*Py_InitializeFunc)();
typedef void (*Py_FinalizeFunc)();
typedef void (*PyErr_PrintFunc)();
typedef int (*PyRun_SimpleStringFunc)(const char*);
typedef int (*PyRun_SimpleFileFunc)(FILE*, const char*);
typedef int (*PyList_InsertFunc)(PyObject*, Py_ssize_t, PyObject*);
typedef int (*PyList_SetSliceFunc)(PyObject*, Py_ssize_t, Py_ssize_t, PyObject*);
typedef PyObject* (*PySys_GetObjectFunc)(const char*);
typedef PyObject* (*PyUnicode_FromStringFunc)(const char*);
typedef Py_ssize_t (*PyList_SizeFunc)(PyObject*);

inline void signalHandler(int signum) {
  std::cout << "Interrupt signal (" << signum << ") received.\n";
  abort();
}

#if defined(_WIN32)
inline std::wstring stringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
inline std::wstring getCurrentExecutablePath() {
    wchar_t path[MAX_PATH];
    DWORD result = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (result == 0) {
        std::wcerr << L"Error getting module file name." << std::endl;
        return L"";
    }
    return std::wstring(path);
}
#else
inline std::string getCurrentExecutablePath() {
#ifdef __APPLE__
  char buf[PATH_MAX];
  uint32_t bufsize = PATH_MAX;

  if (_NSGetExecutablePath(buf, &bufsize) == 0) {
    return std::string(buf);
  }
  return "";
#else
  std::vector<char> buf(PATH_MAX);
  ssize_t len = readlink("/proc/self/exe", &buf[0], buf.size());
  if (len == -1 || len == buf.size()) {
    std::cerr << "Error reading symlink /proc/self/exe." << std::endl;
    return "";
  }
  return std::string(&buf[0], len);
#endif
}
#endif

inline std::string getDirectoryPathFromFilePath(std::string file_path) {
  size_t lastSlashPos = file_path.find_last_of('/');
  size_t lastBackslashPos = file_path.find_last_of('\\');

  size_t pos = std::string::npos;
  if (lastSlashPos != std::string::npos && lastBackslashPos != std::string::npos) {
      pos = (std::max)(lastSlashPos, lastBackslashPos);
  } else if (lastSlashPos != std::string::npos) {
      pos = lastSlashPos;
  } else if (lastBackslashPos != std::string::npos) {
      pos = lastBackslashPos;
  }
  if (pos != std::string::npos) {
      return file_path.substr(0, pos + 1);
  } else {
      return "./";
  }
}

inline std::string findPythonLib(const std::string& libDir) {
  std::string pattern;
#if defined(_WIN32) || defined(_WIN64)
  // Windows
  // python310.dll
  pattern = "python[0-9][0-9]+\\.dll";
#elif defined(__APPLE__) || defined(__MACH__)
  // macOS
  pattern = "libpython[0-9]+\\.[0-9]+\\.dylib";
#else
  // Linux or other Unix-like systems
  pattern = "libpython[0-9]+\\.[0-9]+\\.so.*";
#endif
  std::regex regexPattern(pattern);
  for (const auto& entry : std::filesystem::directory_iterator(libDir)) {
    std::string fileName = entry.path().filename().string();
    if (std::regex_match(fileName, regexPattern)) {
      return entry.path().string();
    }
  }
  return ""; // Return an empty string if no matching library is found
}

inline void clearAndSetPythonSysPath(std::string default_py_lib_path, PY_DL py_dl) {  
  auto PySys_GetObject = (PySys_GetObjectFunc)GET_PY_FUNC(py_dl, "PySys_GetObject");
  auto PyList_Insert = (PyList_InsertFunc)GET_PY_FUNC(py_dl, "PyList_Insert");
  auto PyUnicode_FromString = (PyUnicode_FromStringFunc)GET_PY_FUNC(py_dl, "PyUnicode_FromString");
  auto PyList_SetSlice = (PyList_SetSliceFunc)GET_PY_FUNC(py_dl, "PyList_SetSlice");
  auto PyList_Size = (PyList_SizeFunc)GET_PY_FUNC(py_dl, "PyList_Size");
  PyObject* sys_path = PySys_GetObject("path");
  PyList_SetSlice(sys_path, 0, PyList_Size(sys_path), NULL);
#if defined(_WIN32)
  std::vector<PyObject*> pathStrings = {PyUnicode_FromString((default_py_lib_path).c_str()),
                                        PyUnicode_FromString((default_py_lib_path + "DLLs/").c_str()),
                                        PyUnicode_FromString((default_py_lib_path + "Lib/").c_str()),
                                        PyUnicode_FromString((default_py_lib_path + "Lib/site-packages/").c_str())};
#else
  std::vector<PyObject*> pathStrings = {PyUnicode_FromString((default_py_lib_path + "lib/python/").c_str()),
                                        PyUnicode_FromString((default_py_lib_path + "lib/python/lib-dynload/").c_str()),
                                        PyUnicode_FromString((default_py_lib_path + "lib/python/site-packages/").c_str())};
#endif

  if (sys_path) {
    for(PyObject* pathString : pathStrings) {
      PyList_Insert(sys_path, 0, pathString);
    }
  }
}

inline void executePythonFile(std::string binary_exec_path, std::string py_file_path ,std::string py_lib_path) {

  signal(SIGINT, signalHandler);

  std::string binary_dir_path = PythonRuntimeUtils::getDirectoryPathFromFilePath(binary_exec_path);

  bool isPyDefaultLib = false;
  if (py_lib_path == "") {
    isPyDefaultLib = true;
    py_lib_path = binary_dir_path + "python/";
    LOG_WARN << "No specified Python library path, using default Python library in " << py_lib_path;
  }

  std::string py_dl_path = findPythonLib(py_lib_path);
  if (py_dl_path == "") {
    LOG_ERROR << "Could not find Python dynamic library file in path: " << py_lib_path;
    return;
  } else {
    LOG_DEBUG << "Found dynamic library file " << py_dl_path;;
  }

  PY_DL py_dl = PY_LOAD_LIB(py_dl_path);
  if (!py_dl) {
    LOG_ERROR << "Failed to load Python dynamic library from file: " << py_dl_path;
    return;
  } else {
    LOG_INFO << "Successully loaded Python dynamic library from path: " << py_dl_path;
  }

  auto Py_Initialize = (Py_InitializeFunc)GET_PY_FUNC(py_dl, "Py_Initialize");
  auto Py_Finalize = (Py_FinalizeFunc)GET_PY_FUNC(py_dl, "Py_Finalize");
  auto PyErr_Print = (PyErr_PrintFunc)GET_PY_FUNC(py_dl, "PyErr_Print");
  auto PyRun_SimpleString = (PyRun_SimpleStringFunc)GET_PY_FUNC(py_dl, "PyRun_SimpleString");
  auto PyRun_SimpleFile = (PyRun_SimpleFileFunc)GET_PY_FUNC(py_dl, "PyRun_SimpleFile");

  if (!Py_Initialize || !Py_Finalize || !PyErr_Print || !PyRun_SimpleString || !PyRun_SimpleFile) {
    LOG_ERROR << "Failed to bind necessary Python functions";
    PY_FREE_LIB(py_dl);
    return;
  }

  // Start Python runtime
  Py_Initialize();

  if (isPyDefaultLib) {
    // Re-route the sys paths to Python default library
    clearAndSetPythonSysPath(py_lib_path, py_dl);
  }

  LOG_INFO << "Trying to run Python file in path " << py_file_path;
  FILE* file = fopen(py_file_path.c_str(), "r");
  if (file == NULL) {
    LOG_ERROR << "Failed to open file " << py_file_path;
  } else {
    if (PyRun_SimpleFile(file, py_file_path.c_str() ) != 0) {
      PyErr_Print();
      LOG_ERROR << "Failed to execute file " << py_file_path;
    }
    fclose(file);
  }

  Py_Finalize();
  PY_FREE_LIB(py_dl);
}


} // namespace PythonRuntimeUtils
