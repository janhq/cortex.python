#pragma once
#include <jsoncpp/json/forwards.h>
#include <functional>
#include <memory>

#include "base/cortex-common/EngineI.h"
#include "src/python_file_execution_request.h"

class PythonRuntimeEngine : public EngineI {
    public: 
    ~PythonRuntimeEngine();
    
    void HandlePythonFileExecutionRequest(
        std::shared_ptr<Json::Value> jsonBody,
        std::function<void(Json::Value&&, Json::Value&&)>&& callback) final;
    
    private:
        void HandlePythonFileExecutionRequestImpl(
            PythonRuntime::pythonFileExecution::PythonFileExecutionRequest&& request,
            std::function<void(Json::Value&&, Json::Value&&)> && callback);

};
