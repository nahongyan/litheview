#ifndef LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_DEVTOOLS_RESOURCE_SERVER_H_
#define LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_DEVTOOLS_RESOURCE_SERVER_H_

#include <atomic>
#include <cstdint>
#include <thread>

namespace litheview_demo {

class DevToolsResourceServer {
 public:
  DevToolsResourceServer() = default;
  DevToolsResourceServer(const DevToolsResourceServer&) = delete;
  DevToolsResourceServer& operator=(const DevToolsResourceServer&) = delete;
  ~DevToolsResourceServer();

  bool Start(int port);

 private:
  void Serve();
  void Stop();

  std::atomic<bool> stopping_ = false;
  uintptr_t listen_socket_ = static_cast<uintptr_t>(-1);
  std::thread thread_;
  bool winsock_started_ = false;
};

}  // namespace litheview_demo

#endif  // LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_DEVTOOLS_RESOURCE_SERVER_H_
