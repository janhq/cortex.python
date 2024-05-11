#include <memory>
#include <string>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <signal.h>

#include "dylib.h"
#include "httplib.h"
#include "json/reader.h"
#include "jsoncpp/json/forwards.h"
#include "base/cortex-common/enginei.h"
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

  EngineI* GetEngine() const {
    return engine_;
  }

  struct SyncQueue {
    void Push(std::pair<Json::Value, Json::Value>&& p) {
      std::unique_lock<std::mutex> l(mtx);
      q.push(p);
      cond.notify_one();
    }

    std::pair<Json::Value, Json::Value> WaitAndPop() {
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

private:
  std::unique_ptr<dylib> dylib_;
  EngineI* engine_;

};

std::function<void(int)> shutdown_handler;
std::atomic_flag is_terminating = ATOMIC_FLAG_INIT;

inline void SignalHandler(int signal) {
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
  
  Server server;

  // Check if this process is for python execution
  if (argc > 1) {
    if (strcmp(argv[1], "--run_python_file") == 0) {
        std::string py_home_path = (argc > 3) ? argv[3] : "";
        server.GetEngine()->ExecutePythonFile(argv[0], argv[2], py_home_path);
        return 0;
    }
  } 

  // This process is for running the server
  std::string hostname = (argc > 1) ? argv[1] : "127.0.0.1";
  int port             = (argc > 2) ? std::atoi(argv[2]) : 3928;

  Json::Reader r;
  auto svr = std::make_unique<httplib::Server>();
  
  if (!svr->bind_to_port(hostname, port)) {
    fprintf(stderr, "\ncouldn't bind to server socket: hostname=%s port=%d\n\n",
            hostname.c_str(), port);
    return 1;
  }

  const auto handle_file_execution = [&r, &server](const httplib::Request& req, httplib::Response& resp) {
    resp.set_header("Access-Control-Allow-Origin", req.get_header_value("Origin"));
    auto req_body = std::make_shared<Json::Value>();
    r.parse(req.body, *req_body);
    server.GetEngine()->HandlePythonFileExecutionRequest(
        req_body, [&server, &resp](Json::Value status, Json::Value res) {
          resp.set_content(res.toStyledString().c_str(),
                           "application/json; charset=utf-8");
          resp.status = status["status_code"].asInt();
        });
  };

  svr->Post("/execute", handle_file_execution);

  LOG_INFO << "HTTP server listening: " << hostname << ":" << port;
  svr->new_task_queue = [] {
    return new httplib::ThreadPool(5);
  };
  // run the HTTP server in a thread - see comment below
  std::thread t([&]() {
    if (!svr->listen_after_bind()) {
      return 1;
    }

    return 0;
  });
  std::atomic<bool> running = true;

  shutdown_handler = [&](int) {
    running = false;
  };
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
  struct sigaction sigint_action;
  sigint_action.sa_handler = SignalHandler;
  sigemptyset(&sigint_action.sa_mask);
  sigint_action.sa_flags = 0;
  sigaction(SIGINT, &sigint_action, NULL);
  sigaction(SIGTERM, &sigint_action, NULL);
#elif defined(_WIN32)
  auto console_ctrl_handler = +[](DWORD ctrl_type) -> BOOL {
    return (ctrl_type == CTRL_C_EVENT) ? (signal_handler(SIGINT), true) : false;
  };
  SetConsoleCtrlHandler(
      reinterpret_cast<PHANDLER_ROUTINE>(console_ctrl_handler), true);
#endif

  while (running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  svr->stop();
  t.join();
  LOG_DEBUG << "Server shutdown";
  return 0;
}
