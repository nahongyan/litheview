#ifndef LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_SHELL_APP_H_
#define LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_SHELL_APP_H_

#include <litheview/litheview.h>

#include <memory>
#include <string>
#include <vector>

namespace litheview_demo {

class ShellHost;

class ShellApp {
 public:
  explicit ShellApp(int devtools_frontend_port);
  ~ShellApp();

  bool OpenInitialWindow();
  bool OpenInspectorWindow(std::string initial_url);
  void OpenPopup(ltv_view_t popup, const ltv_popup_request_t* request);
  bool OpenDevTools(ltv_view_t inspected_view);
  void OnWindowDestroyed();

 private:
  std::vector<std::unique_ptr<ShellHost>> windows_;
  size_t open_window_count_ = 0;
  int devtools_frontend_port_ = 0;
};

}  // namespace litheview_demo

#endif  // LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_SHELL_APP_H_
