#pragma once

#include <functional>
#include <memory>

#include "json/value.h"

class EngineI {
    public:
    virtual ~EngineI() {}

    virtual void ExecutePythonFile(
        std::string binary_exec_path,
        std::string pythonFileExecutionPath,
        std::string pythonLibraryPath) = 0;

    virtual void HandlePythonFileExecutionRequest(
        std::shared_ptr<Json::Value> jsonBody,
        std::function<void(Json::Value&&, Json::Value&&)>&& callback) = 0;
};
