#include "PythonRuntimeEngine.h"
#include "PythonRuntimeUtils.hpp"

#if defined(_WIN32)
    #include <process.h>
#else
    #include <spawn.h>
    #include <sys/wait.h>
    extern char **environ;
#endif

constexpr const int k200OK = 200;
constexpr const int k400BadRequest = 400;
constexpr const int k500InternalServerError = 500;

PythonRuntimeEngine::~PythonRuntimeEngine() {}

void PythonRuntimeEngine::ExecutePythonFile(
        std::string binaryExecPath,
        std::string pythonFileExecutionPath,
        std::string pythonLibraryPath) {

    std::string currentDirPath = PythonRuntimeUtils::getDirectoryPathFromFilePath(binaryExecPath);
    PythonRuntimeUtils::executePythonFile(currentDirPath, pythonFileExecutionPath, pythonLibraryPath);
}

void PythonRuntimeEngine::HandlePythonFileExecutionRequest(
    std::shared_ptr<Json::Value> jsonBody,
    std::function<void(Json::Value&&, Json::Value&&)>&& callback) {

    HandlePythonFileExecutionRequestImpl(
        PythonRuntime::pythonFileExecution::fromJson(jsonBody),
        std::move(callback));
}


void PythonRuntimeEngine::HandlePythonFileExecutionRequestImpl(
    PythonRuntime::pythonFileExecution::PythonFileExecutionRequest&& request,
    std::function<void(Json::Value&&, Json::Value&&)> && callback) {

    std::string file_execution_path = request.file_execution_path;
    std::string python_library_path = request.python_library_path;

    Json::Value jsonResp;
    Json::Value statusResp;

    if (file_execution_path == "") {
        jsonResp["message"] = "No specified Python file path";
        statusResp["status_code"] = k400BadRequest;
        callback(std::move(statusResp), std::move(jsonResp));
        return;
    }

    jsonResp["message"] = "Executing the Python file";
    statusResp["status_code"] = k200OK;

#if defined(_WIN32)
    std::wstring exePath = PythonRuntimeUtils::getCurrentExecutablePath();
    std::string pyArgsString = " --run_python_file " + file_execution_path;
    if (py_lib_path != "")
        pyArgsString += " " + py_lib_path;
    std::wstring pyArgs = exePath + PythonRuntimeUtils::stringToWString(pyArgsString);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(const_cast<wchar_t*>(exePath.data()), // the path to the executable file
                        const_cast<wchar_t*>(pyArgs.data()), // command line arguments passed to the child
                        NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        LOG_ERROR << "Failed to create child process: " << GetLastError();
        jsonResp["message"] = "Failed to execute the Python file";
    } else {
        LOG_INFO << "Created child process for Python embedding";
    }
#else
    std::string child_process_exe_path = PythonRuntimeUtils::getCurrentExecutablePath();
    std::vector<char*> child_process_args;
    child_process_args.push_back(const_cast<char*>(child_process_exe_path.c_str()));
    child_process_args.push_back(const_cast<char*>("--run_python_file"));
    child_process_args.push_back(const_cast<char*>(file_execution_path.c_str()));
    if (python_library_path != "")
        child_process_args.push_back(const_cast<char*>(python_library_path.c_str()));

    pid_t pid;

    int status = posix_spawn(&pid, child_process_exe_path.c_str(), nullptr, 
                           nullptr, child_process_args.data(), environ);
                  
    if (status) {
        LOG_ERROR << "Failed to spawn process: " << strerror(status);
        jsonResp["message"] = "Failed to execute the Python file";
        statusResp["status_code"] = k500InternalServerError;
    } else {
      LOG_INFO << "Created child process for Python embedding";
      int stat_loc;
      if (waitpid(pid, &stat_loc, 0) == -1) {
          LOG_ERROR << "Error waiting for child process";
          jsonResp["message"] = "Failed to execute the Python file";
          statusResp["status_code"] = k500InternalServerError;
      }
    }
#endif

    callback(std::move(statusResp), std::move(jsonResp));
    return;
};

extern "C" {
EngineI* get_engine() {
    return new PythonRuntimeEngine();
}
} // extern C
