#pragma once
#include <functional>
#include <memory>

#include "base/cortex-common/cortexpythoni.h"
#include "json/forwards.h"
#include "src/python_file_execution_request.h"

class PythonEngine : public CortexPythonEngineI {
 public: 
  ~PythonEngine() final;

  void ExecutePythonFile(
      std::string binary_exec_path,
      std::string pythonFileExecutionPath,
      std::string pythonLibraryPath) final;
  
  void HandlePythonFileExecutionRequest(
      std::shared_ptr<Json::Value> jsonBody,
      std::function<void(Json::Value&&, Json::Value&&)>&& callback) final;
  
 private:
  void HandlePythonFileExecutionRequestImpl(
      PythonRuntime::PythonFileExecution::PythonFileExecutionRequest&& request,
      std::function<void(Json::Value&&, Json::Value&&)> && callback);

};
