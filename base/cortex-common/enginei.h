#pragma once

#include <functional>
#include <memory>

#include "json/value.h"

class EngineI {
 public:
  virtual ~EngineI() {}

  virtual void HandleChatCompletion(
      std::shared_ptr<Json::Value> json_body,
      std::function<void(Json::Value&&, Json::Value&&)>&& callback) {}
  virtual void HandleEmbedding(
      std::shared_ptr<Json::Value> json_body,
      std::function<void(Json::Value&&, Json::Value&&)>&& callback) {}
  virtual void LoadModel(
      std::shared_ptr<Json::Value> json_body,
      std::function<void(Json::Value&&, Json::Value&&)>&& callback)  {}
  virtual void UnloadModel(
      std::shared_ptr<Json::Value> json_body,
      std::function<void(Json::Value&&, Json::Value&&)>&& callback)  {}
  virtual void GetModelStatus(
      std::shared_ptr<Json::Value> json_body,
      std::function<void(Json::Value&&, Json::Value&&)>&& callback)  {}

  virtual void ExecutePythonFile(std::string binary_execute_path,
                                 std::string file_execution_path,
                                 std::string python_library_path) = 0;

  virtual void HandlePythonFileExecutionRequest(
      std::shared_ptr<Json::Value> json_body,
      std::function<void(Json::Value&&, Json::Value&&)>&& callback) = 0;

  virtual bool IsSupported(const std::string& f) {
    if (f == "ExecutePythonFile" || f == "HandlePythonFileExecutionRequest") {
      return true;
    }
    return false;
  }
};
