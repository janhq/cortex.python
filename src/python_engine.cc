#include "python_engine.h"
#include "python_utils.h"
#include "trantor/utils/Logger.h"

#if defined(_WIN32)
#include <process.h>
#else
#include <spawn.h>
#include <sys/wait.h>
extern char** environ;
#endif

constexpr const int k200OK = 200;
constexpr const int k400BadRequest = 400;
constexpr const int k500InternalServerError = 500;

PythonEngine::~PythonEngine() {}

void PythonEngine::ExecutePythonFile(std::string binary_execute_path,
                                     std::string file_execution_path,
                                     std::string python_library_path) {

  std::string current_dir_path =
      python_utils::GetDirectoryPathFromFilePath(binary_execute_path);
  python_utils::ExecutePythonFile(current_dir_path, file_execution_path,
                                  python_library_path);
}

void PythonEngine::HandlePythonFileExecutionRequest(
    std::shared_ptr<Json::Value> json_body,
    std::function<void(Json::Value&&, Json::Value&&)>&& callback) {

  HandlePythonFileExecutionRequestImpl(
      PythonRuntime::PythonFileExecution::FromJson(json_body),
      std::move(callback));
}

void PythonEngine::HandlePythonFileExecutionRequestImpl(
    PythonRuntime::PythonFileExecution::PythonFileExecutionRequest&& request,
    std::function<void(Json::Value&&, Json::Value&&)>&& callback) {

  std::string file_execution_path = request.file_execution_path;
  std::string python_library_path = request.python_library_path;

  Json::Value json_resp;
  Json::Value status_resp;

  if (file_execution_path.empty()) {
    LOG_ERROR << "No specified Python file path";
    json_resp["message"] = "No specified Python file path";
    status_resp["status_code"] = k400BadRequest;
    callback(std::move(status_resp), std::move(json_resp));
    return;
  }

  json_resp["message"] = "Executing the Python file";
  status_resp["status_code"] = k200OK;

#if defined(_WIN32)
  std::wstring exe_path = python_utils::getCurrentExecutablePath();
  std::string exe_args_string = " --run_python_file " + file_execution_path;
  if (!python_library_path.empty())
    exe_args_string += " --python_library_path " + python_library_path;
  std::wstring pyArgs =
      exe_path + python_utils::stringToWString(exe_args_string);

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  if (!CreateProcessW(
          const_cast<wchar_t*>(exe_path.c_str()),
          const_cast<wchar_t*>(pyArgs.c_str()),
          NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    LOG_ERROR << "Failed to create child process: " << GetLastError();
    json_resp["message"] = "Failed to execute the Python file";
    status_resp["status_code"] = k500InternalServerError;
  } else {
    LOG_INFO << "Created child process for Python embedding";
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
  }
#else
  std::string child_process_exe_path = python_utils::getCurrentExecutablePath();
  std::vector<char*> child_process_args;
  child_process_args.push_back(const_cast<char*>(child_process_exe_path.c_str()));
  child_process_args.push_back(const_cast<char*>("--run_python_file"));
  child_process_args.push_back(const_cast<char*>(file_execution_path.c_str()));

  if (!python_library_path.empty()) {
    child_process_args.push_back(const_cast<char*>(python_library_path.c_str()));
  }
  child_process_args.push_back(NULL);

  pid_t pid;
  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);

  // Prepare environment variables
  std::vector<std::string> env_strings;
  std::vector<char*> env_ptrs;
  for (char** env = environ; *env != nullptr; env++) {
    env_strings.push_back(*env);
  }
  
  // Add or modify PYTHONPATH
  std::string pythonpath = "PYTHONPATH=" + python_library_path;
  env_strings.push_back(pythonpath);

  // Convert environment strings to char* pointers
  for (const auto& s : env_strings) {
    env_ptrs.push_back(const_cast<char*>(s.c_str()));
  }
  env_ptrs.push_back(nullptr);

  int status = posix_spawn(&pid, child_process_exe_path.c_str(), &actions,
                           nullptr, child_process_args.data(), env_ptrs.data());

  if (status != 0) {
    LOG_ERROR << "Failed to spawn process: " << strerror(status);
    json_resp["message"] = "Failed to execute the Python file";
    status_resp["status_code"] = k500InternalServerError;
  } else {
    LOG_INFO << "Created child process for Python embedding";
    int stat_loc;
    if (waitpid(pid, &stat_loc, 0) == -1) {
      LOG_ERROR << "Error waiting for child process";
      json_resp["message"] = "Failed to execute the Python file";
      status_resp["status_code"] = k500InternalServerError;
    } else if (WIFEXITED(stat_loc)) {
      int exit_status = WEXITSTATUS(stat_loc);
      if (exit_status != 0) {
        LOG_ERROR << "Child process exited with status: " << exit_status;
        json_resp["message"] = "Python script execution failed";
        status_resp["status_code"] = k500InternalServerError;
      }
    }
  }

  posix_spawn_file_actions_destroy(&actions);
#endif

  callback(std::move(status_resp), std::move(json_resp));
}

extern "C" {
CortexPythonEngineI* get_engine() {
  return new PythonEngine();
}
}  // extern C