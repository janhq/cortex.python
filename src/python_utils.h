#pragma once

#include <csignal>
#include <filesystem>
#include <regex>
#include <string>
#include <iostream>
#include <dlfcn.h>

#include "trantor/utils/Logger.h"

#if defined(_WIN32)
  #define PY_DL HMODULE
  #define PY_LOAD_LIB(path) LoadLibraryW(python_utils::stringToWString(path).c_str());
  #define GET_PY_FUNC GetProcAddress
  #define PY_FREE_LIB FreeLibrary
#else
  #define PY_DL void*
  #define PY_LOAD_LIB(path) dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
  #define GET_PY_FUNC dlsym
  #define PY_FREE_LIB dlclose
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

namespace python_utils {

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

inline void SignalHandler(int signum) {
  LOG_WARN << "Interrupt signal (" << signum << ") received.";
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

inline std::string GetDirectoryPathFromFilePath(std::string file_path) {
  size_t last_forw_slash_pos = file_path.find_last_of('/');
  size_t last_back_slash_pos = file_path.find_last_of('\\');

  size_t pos = std::string::npos;
  if (last_forw_slash_pos != std::string::npos && last_back_slash_pos != std::string::npos) {
      pos = (std::max)(last_forw_slash_pos, last_back_slash_pos);
  } else if (last_forw_slash_pos != std::string::npos) {
      pos = last_forw_slash_pos;
  } else if (last_back_slash_pos != std::string::npos) {
      pos = last_back_slash_pos;
  }
  if (pos != std::string::npos) {
      return file_path.substr(0, pos + 1);
  } else {
      return "./";
  }
}

inline std::string FindPythonDynamicLib(const std::string& lib_dir) {
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
  std::regex regex_pattern(pattern);
  for (const auto& entry : std::filesystem::directory_iterator(lib_dir)) {
    std::string file_name = entry.path().filename().string();
    if (std::regex_match(file_name, regex_pattern)) {
      return entry.path().string();
    }
  }
  return ""; // Return an empty string if no matching library is found
}

inline void ClearAndSetPythonSysPath(std::string default_py_lib_path, PY_DL py_dl) {  
  auto python_sys_get_object_func = (PySys_GetObjectFunc)GET_PY_FUNC(py_dl, "PySys_GetObject");
  auto python_list_insert_func = (PyList_InsertFunc)GET_PY_FUNC(py_dl, "PyList_Insert");
  auto python_unicode_from_string_func = (PyUnicode_FromStringFunc)GET_PY_FUNC(py_dl, "PyUnicode_FromString");
  auto python_list_set_slice_func = (PyList_SetSliceFunc)GET_PY_FUNC(py_dl, "PyList_SetSlice");
  auto python_list_size_func = (PyList_SizeFunc)GET_PY_FUNC(py_dl, "PyList_Size");
  PyObject* sys_path = python_sys_get_object_func("path");
  python_list_set_slice_func(sys_path, 0, python_list_size_func(sys_path), NULL);
#if defined(_WIN32)
  std::vector<PyObject*> pathStrings = {
      python_unicode_from_string_func((default_py_lib_path).c_str()),
      python_unicode_from_string_func((default_py_lib_path + "DLLs/").c_str()),
      python_unicode_from_string_func((default_py_lib_path + "Lib/").c_str()),
      python_unicode_from_string_func((default_py_lib_path + "Lib/site-packages/").c_str())};
#else
  std::vector<PyObject*> pathStrings = {
      python_unicode_from_string_func((default_py_lib_path + "lib/python/").c_str()),
      python_unicode_from_string_func((default_py_lib_path + "lib/python/lib-dynload/").c_str()),
      python_unicode_from_string_func((default_py_lib_path + "lib/python/site-packages/").c_str())};
#endif

  if (sys_path) {
    for(PyObject* pathString : pathStrings) {
      python_list_insert_func(sys_path, 0, pathString);
    }
  }
}

inline void ExecutePythonFile(std::string binary_exec_path, std::string py_file_path ,std::string py_lib_path) {

  signal(SIGINT, SignalHandler);
  std::string binary_dir_path = python_utils::GetDirectoryPathFromFilePath(binary_exec_path);

  bool is_default_python_lib = false;
  if (py_lib_path == "") {
    is_default_python_lib = true;
    py_lib_path = binary_dir_path + "python/";
    LOG_WARN << "No specified Python library path, using default Python library in " << py_lib_path;
  }

  std::string py_dl_path = FindPythonDynamicLib(py_lib_path);
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

  auto python_initialize_func = (Py_InitializeFunc)GET_PY_FUNC(py_dl, "Py_Initialize");
  auto python_finalize_func = (Py_FinalizeFunc)GET_PY_FUNC(py_dl, "Py_Finalize");
  auto python_err_print = (PyErr_PrintFunc)GET_PY_FUNC(py_dl, "PyErr_Print");
  auto python_run_simple_string_func = (PyRun_SimpleStringFunc)GET_PY_FUNC(py_dl, "PyRun_SimpleString");
  auto python_run_simple_pile_func = (PyRun_SimpleFileFunc)GET_PY_FUNC(py_dl, "PyRun_SimpleFile");

  if (!python_initialize_func || !python_finalize_func || !python_err_print 
      || !python_run_simple_string_func || !python_run_simple_pile_func) {
    LOG_ERROR << "Failed to bind necessary Python functions";
    PY_FREE_LIB(py_dl);
    return;
  }

  // Start Python runtime
  python_initialize_func();

  if (is_default_python_lib) {
    // Re-route the sys paths to Python default library
    ClearAndSetPythonSysPath(py_lib_path, py_dl);
  }

  LOG_INFO << "Trying to run Python file in path " << py_file_path;
  FILE* file = fopen(py_file_path.c_str(), "r");
  if (file == NULL) {
    LOG_ERROR << "Failed to open file " << py_file_path;
  } else {
    if (python_run_simple_pile_func(file, py_file_path.c_str() ) != 0) {
      python_err_print();
      LOG_ERROR << "Failed to execute file " << py_file_path;
    }
    fclose(file);
  }

  python_finalize_func();
  PY_FREE_LIB(py_dl);
}


} // namespace python_utils
