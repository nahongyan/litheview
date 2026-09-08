#include <windows.h>

#include <litheview/litheview.h>
#include <shellapi.h>

#include <memory>
#include <string>
#include <string_view>

#include "demo_support.h"
#include "devtools_resource_server.h"
#include "shell_app.h"

namespace litheview_demo {

namespace {

struct InspectorState {
  std::string frontend_url;
  int exit_code = 1;
  std::unique_ptr<ShellApp> app;
};

void LTV_CALLBACK OnInspectorReady(void* user_data) {
  auto* state = static_cast<InspectorState*>(user_data);
  state->app = std::make_unique<ShellApp>(0);
  if (!state->app->OpenInspectorWindow(state->frontend_url)) {
    ltv_shutdown();
    return;
  }
  state->exit_code = 0;
}

int RunInspector() {
  int argument_count = 0;
  LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
  std::wstring target;
  int frontend_port = 0;
  for (int index = 1; arguments && index < argument_count; ++index) {
    constexpr std::wstring_view kTargetPrefix = L"--devtools-target=";
    constexpr std::wstring_view kPortPrefix = L"--frontend-port=";
    const std::wstring_view argument(arguments[index]);
    if (argument.starts_with(kTargetPrefix)) {
      target = argument.substr(kTargetPrefix.size());
    } else if (argument.starts_with(kPortPrefix)) {
      frontend_port = _wtoi(arguments[index] + kPortPrefix.size());
    }
  }
  if (arguments) {
    LocalFree(arguments);
  }
  constexpr std::wstring_view kWebSocketPrefix = L"ws://";
  if (target.starts_with(kWebSocketPrefix)) {
    target.erase(0, kWebSocketPrefix.size());
  }
  if (target.empty() || frontend_port <= 0 || frontend_port > 65535) {
    return 1;
  }

  DevToolsResourceServer server;
  if (!server.Start(frontend_port)) {
    return 1;
  }
  InspectorState state = {
      .frontend_url = "http://127.0.0.1:" + std::to_string(frontend_port) +
                      "/inspector.html?ws=" + WideToUtf8(target),
  };
  const ltv_settings_t settings = {
      .struct_size = sizeof(ltv_settings_t),
      .version = LTV_STRUCT_VERSION,
      .width_dip = kViewWidth,
      .height_dip = kViewHeight,
      .device_scale_factor =
          static_cast<float>(GetDpiForSystem()) / USER_DEFAULT_SCREEN_DPI,
      .remote_debugging_port = 0,
      .devtools_frontend_port = 0,
  };
  if (ltv_initialize(&settings) != LTV_OK) {
    return 1;
  }
  const int content_exit_code = ltv_run(0, nullptr, &OnInspectorReady, &state);
  return content_exit_code == 0 ? state.exit_code : content_exit_code;
}

}  // namespace

}  // namespace litheview_demo

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int) {
  return litheview_demo::RunInspector();
}
