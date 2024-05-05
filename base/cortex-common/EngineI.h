#pragma once

#include <functional>
#include <memory>

#include "json/value.h"

class EngineI {
    public:
    virtual ~EngineI() {}

    virtual void HandlePythonFileExecutionRequest(
        std::shared_ptr<Json::Value> jsonBody,
        std::function<void(Json::Value&&, Json::Value&&)>&& callback) = 0;
};
