#pragma once

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#define PY_DL HMODULE
#define PY_LOAD_LIB(path) \
  LoadLibraryW(python_utils::stringToWString(path).c_str())
#define GET_PY_FUNC GetProcAddress
#define PY_FREE_LIB FreeLibrary
#else
#include <dlfcn.h>
#define PY_DL void*
#define PY_LOAD_LIB(path) dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL)
#define GET_PY_FUNC dlsym
#define PY_FREE_LIB dlclose
#endif

#if __APPLE__
#include <mach-o/dyld.h>
#endif

#include "trantor/utils/Logger.h"

namespace python_utils {

typedef long Py_ssize_t;
typedef struct _object PyObject;
typedef void (*Py_InitializeFunc)();
typedef void (*Py_FinalizeFunc)();
typedef void (*PyErr_PrintFunc)();
typedef int (*PyRun_SimpleStringFunc)(const char*);
typedef int (*PyRun_SimpleFileFunc)(FILE*, const char*);
typedef int (*PyList_InsertFunc)(PyObject*, Py_ssize_t, PyObject*);
typedef int (*PyList_SetSliceFunc)(PyObject*, Py_ssize_t, Py_ssize_t,
                                   PyObject*);
typedef PyObject* (*PySys_GetObjectFunc)(const char*);
typedef PyObject* (*PyUnicode_FromStringFunc)(const char*);
typedef Py_ssize_t (*PyList_SizeFunc)(PyObject*);

inline void SignalHandler(int signum) {
  LOG_WARN << "Interrupt signal (" << signum << ") received.";
  std::abort();
}

#if defined(_WIN32)
inline std::wstring stringToWString(const std::string& str) {
  if (str.empty())
    return std::wstring();
  int size_needed =
      MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0],
                      size_needed);
  return wstrTo;
}

inline std::wstring getCurrentExecutablePath() {
  wchar_t path[MAX_PATH];
  DWORD result = GetModuleFileNameW(NULL, path, MAX_PATH);
  if (result == 0) {
    LOG_ERROR << "Error getting module file name: " << GetLastError();
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
  LOG_ERROR << "Error getting executable path on macOS";
  return "";
#else
  char buf[4096];
  ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len != -1) {
    buf[len] = '\0';
    return std::string(buf);
  }
  LOG_ERROR << "Error getting executable path on Linux: " << strerror(errno);
  return "";
#endif
}
#endif

inline std::string GetDirectoryPathFromFilePath(const std::string& file_path) {
  size_t last_separator = file_path.find_last_of("/\\");
  if (last_separator != std::string::npos) {
    return file_path.substr(0, last_separator + 1);
  }
  return "./";
}
inline std::string FindPythonDynamicLib(const std::string& lib_dir) {
  try {
    LOG_INFO << "Entering FindPythonDynamicLib, lib_dir: " << lib_dir;

    if (lib_dir.empty()) {
      LOG_ERROR << "lib_dir is empty";
      return "";
    }

    std::string pattern;
#if defined(_WIN32) || defined(_WIN64)
    pattern = "python[0-9][0-9]+\\.dll";
#elif defined(__APPLE__) || defined(__MACH__)
    pattern = "libpython[0-9]+\\.[0-9]+\\.dylib";
#else
    pattern = "libpython[0-9]+\\.[0-9]+\\.so.*";
#endif
    LOG_INFO << "Pattern: " << pattern;

    std::regex regex_pattern(pattern);
    LOG_INFO << "Regex pattern compiled successfully";

    if (!std::filesystem::exists(lib_dir)) {
      LOG_ERROR << "Directory does not exist: " << lib_dir;
      return "";
    }

    if (!std::filesystem::is_directory(lib_dir)) {
      LOG_ERROR << "Path is not a directory: " << lib_dir;
      return "";
    }

    LOG_INFO << "Starting directory iteration";
    for (const auto& entry : std::filesystem::directory_iterator(lib_dir)) {
      std::string file_name = entry.path().filename().string();
      LOG_INFO << "File name: " << file_name;
      if (std::regex_match(file_name, regex_pattern)) {
        LOG_INFO << "Match found: " << entry.path().string();
        return entry.path().string();
      }
    }

    LOG_ERROR << "No Python dynamic library found in " << lib_dir;
    return "";
  } catch (const std::exception& e) {
    LOG_ERROR << "Exception in FindPythonDynamicLib: " << e.what();
    return "";
  } catch (...) {
    LOG_ERROR << "Unknown exception in FindPythonDynamicLib";
    return "";
  }
}

inline void ClearAndSetPythonSysPath(const std::string& default_py_lib_path,
                                     PY_DL py_dl) {
  auto python_sys_get_object_func =
      (PySys_GetObjectFunc)GET_PY_FUNC(py_dl, "PySys_GetObject");
  auto python_list_insert_func =
      (PyList_InsertFunc)GET_PY_FUNC(py_dl, "PyList_Insert");
  auto python_unicode_from_string_func =
      (PyUnicode_FromStringFunc)GET_PY_FUNC(py_dl, "PyUnicode_FromString");
  auto python_list_set_slice_func =
      (PyList_SetSliceFunc)GET_PY_FUNC(py_dl, "PyList_SetSlice");
  auto python_list_size_func =
      (PyList_SizeFunc)GET_PY_FUNC(py_dl, "PyList_Size");

  if (!python_sys_get_object_func || !python_list_insert_func ||
      !python_unicode_from_string_func || !python_list_set_slice_func ||
      !python_list_size_func) {
    LOG_ERROR
        << "Failed to get required Python functions for sys.path manipulation";
    return;
  }

  PyObject* sys_path = python_sys_get_object_func("path");
  if (!sys_path) {
    LOG_ERROR << "Failed to get sys.path";
    return;
  }

  python_list_set_slice_func(sys_path, 0, python_list_size_func(sys_path),
                             NULL);

  std::vector<std::string> path_strings;
#if defined(_WIN32)
  path_strings = {default_py_lib_path, default_py_lib_path + "DLLs\\",
                  default_py_lib_path + "Lib\\",
                  default_py_lib_path + "Lib\\site-packages\\"};
#else
  path_strings = {default_py_lib_path + "lib/python/",
                  default_py_lib_path + "lib/python/lib-dynload/",
                  default_py_lib_path + "lib/python/site-packages/"};
#endif

  for (const auto& path : path_strings) {
    PyObject* path_obj = python_unicode_from_string_func(path.c_str());
    if (path_obj) {
      python_list_insert_func(sys_path, 0, path_obj);
      LOG_INFO << "Added to sys.path: " << path;
    } else {
      LOG_ERROR << "Failed to create Unicode object for path: " << path;
    }
  }
}

inline void ExecutePythonFile(const std::string& binary_exec_path,
                              const std::string& py_file_path,
                              const std::string& py_lib_path) {
  signal(SIGINT, SignalHandler);
  std::string binary_dir_path = GetDirectoryPathFromFilePath(binary_exec_path);

  std::string effective_py_lib_path = py_lib_path;
  if (effective_py_lib_path.empty()) {
    effective_py_lib_path =
        (std::filesystem::path(binary_dir_path) /
         std::filesystem::path("engines/cortex.python/python/"))
            .string();
    LOG_WARN << "No specified Python library path, using default: "
             << effective_py_lib_path;
  }

// Set PYTHONHOME and PYTHONPATH environment variables
#if defined(_WIN32)
  _putenv_s("PYTHONHOME", effective_py_lib_path.c_str());
  _putenv_s("PYTHONPATH", effective_py_lib_path.c_str());
#else
  setenv("PYTHONHOME", effective_py_lib_path.c_str(), 1);
  setenv("PYTHONPATH", effective_py_lib_path.c_str(), 1);
#endif

  LOG_INFO << "Set PYTHONHOME to: " << effective_py_lib_path;
  LOG_INFO << "Set PYTHONPATH to: " << effective_py_lib_path;

  std::string py_dl_path = FindPythonDynamicLib(effective_py_lib_path);
  if (py_dl_path.empty()) {
    LOG_ERROR << "Could not find Python dynamic library file in path: "
              << effective_py_lib_path;
    return;
  }
  LOG_INFO << "Found dynamic library file: " << py_dl_path;

  PY_DL py_dl = PY_LOAD_LIB(py_dl_path);
  if (!py_dl) {
    LOG_ERROR << "Failed to load Python dynamic library from file: "
              << py_dl_path;
    return;
  }
  LOG_INFO << "Successfully loaded Python dynamic library from path: "
           << py_dl_path;

  auto python_initialize_func =
      (Py_InitializeFunc)GET_PY_FUNC(py_dl, "Py_Initialize");
  auto python_finalize_func =
      (Py_FinalizeFunc)GET_PY_FUNC(py_dl, "Py_Finalize");
  auto python_err_print = (PyErr_PrintFunc)GET_PY_FUNC(py_dl, "PyErr_Print");
  auto python_run_simple_string_func =
      (PyRun_SimpleStringFunc)GET_PY_FUNC(py_dl, "PyRun_SimpleString");
  auto python_run_simple_file_func =
      (PyRun_SimpleFileFunc)GET_PY_FUNC(py_dl, "PyRun_SimpleFile");

  if (!python_initialize_func || !python_finalize_func || !python_err_print ||
      !python_run_simple_string_func || !python_run_simple_file_func) {
    LOG_ERROR << "Failed to bind necessary Python functions";
    PY_FREE_LIB(py_dl);
    return;
  }

  python_initialize_func();

  // Set sys.path
  ClearAndSetPythonSysPath(effective_py_lib_path, py_dl);

  // Add debug logging
  python_run_simple_string_func(R"(
import sys
import os
print("Python version:", sys.version)
print("Python sys.path:", sys.path)
print("Python executable:", sys.executable)
print("Python prefix:", sys.prefix)
print("Python exec prefix:", sys.exec_prefix)
print("PYTHONHOME:", os.environ.get('PYTHONHOME', 'Not set'))
print("PYTHONPATH:", os.environ.get('PYTHONPATH', 'Not set'))
    )");

  LOG_INFO << "Trying to run Python file: " << py_file_path;
  FILE* file = fopen(py_file_path.c_str(), "r");
  if (file == NULL) {
    LOG_ERROR << "Failed to open file: " << py_file_path
              << ", error: " << strerror(errno);
  } else {
    if (python_run_simple_file_func(file, py_file_path.c_str()) != 0) {
      python_err_print();
      LOG_ERROR << "Failed to execute file: " << py_file_path;
    } else {
      LOG_INFO << "Successfully executed Python file: " << py_file_path;
    }
    fclose(file);
  }

  python_finalize_func();
  PY_FREE_LIB(py_dl);
  LOG_INFO << "Python execution completed";
}

}  // namespace python_utils