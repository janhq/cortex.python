#include <jsoncpp/json/forwards.h>
#include <memory>
#include <string>
#include <signal.h>
#include <condition_variable>
#include <mutex>
#include <queue>

#include "dylib.h"
#include "httplib.h"
#include "json/reader.h"
#include "base/cortex-common/EngineI.h"
#include "trantor/utils/Logger.h"

class Server {
 public:
  Server() {
    dylib_ = std::make_unique<dylib>("./engines/cortex.python-runtime", "engine");
    auto func = dylib_->get_function<EngineI*()>("get_engine");
    engine_ = func();
  }

  ~Server() {
    if (engine_) {
      delete engine_;
    }
  }

 public:
  std::unique_ptr<dylib> dylib_;
  EngineI* engine_;

  struct SyncQueue {
    void push(std::pair<Json::Value, Json::Value>&& p) {
      std::unique_lock<std::mutex> l(mtx);
      q.push(p);
      cond.notify_one();
    }

    std::pair<Json::Value, Json::Value> wait_and_pop() {
      std::unique_lock<std::mutex> l(mtx);
      cond.wait(l, [this] { return !q.empty(); });
      auto res = q.front();
      q.pop();
      return res;
    }

    std::mutex mtx;
    std::condition_variable cond;
    // Status and result
    std::queue<std::pair<Json::Value, Json::Value>> q;
  };
};

std::function<void(int)> shutdown_handler;
std::atomic_flag is_terminating = ATOMIC_FLAG_INIT;

inline void signal_handler(int signal) {
  if (is_terminating.test_and_set()) {
    // in case it hangs, we can force terminate the server by hitting Ctrl+C twice
    // this is for better developer experience, we can remove when the server is stable enough
    fprintf(stderr, "Received second interrupt, terminating immediately.\n");
    exit(1);
  }

  shutdown_handler(signal);
}

using SyncQueue = Server::SyncQueue;

int main(int argc, char** argv) {
  std::string hostname = "127.0.0.1";
  int port = 3928;

  Server server;
  Json::Reader r;
  auto svr = std::make_unique<httplib::Server>();
  
  if (!svr->bind_to_port(hostname, port)) {
    fprintf(stderr, "\ncouldn't bind to server socket: hostname=%s port=%d\n\n",
            hostname.c_str(), port);
    return 1;
  }

  auto process_res = [&server](httplib::Response& resp, SyncQueue& q) {
    auto [status, res] = q.wait_and_pop();
    resp.set_content(res.toStyledString().c_str(), "application/json; charset=utf-8");
    resp.status = status["status_code"].asInt();
  };

  const auto handle_file_execution = [&](const httplib::Request& req, httplib::Response& resp) {
    resp.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin"));
    auto req_body = std::make_shared<Json::Value>();
    r.parse(req.body, *req_body);
    server.engine_->HandlePythonFileExecutionRequest(
        req_body, [&server, &resp](Json::Value status, Json::Value res) {
          resp.set_content(res.toStyledString().c_str(),
                           "application/json; charset=utf-8");
          resp.status = status["status_code"].asInt();
        });
  };

  svr->Post("/execute", handle_file_execution);




}