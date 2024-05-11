#pragma once

#include <memory>
#include <string>

#include "json/value.h"

namespace PythonRuntime::PythonFileExecution {

struct PythonFileExecutionRequest {
  std::string file_execution_path = "";
  std::string python_library_path = "";
  bool isDefaultLib = true;
};

inline PythonFileExecutionRequest FromJson(std::shared_ptr<Json::Value> json_body) {
  PythonFileExecutionRequest request;

  if (json_body) {
    request.file_execution_path = json_body->get("file_execution_path", "").asString();
    request.python_library_path = json_body->get("python_library_path", "").asString();
  }

  return request;
}

} // namespace PythonRuntime::PythonFileExecution
