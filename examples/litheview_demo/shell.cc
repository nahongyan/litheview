#include "shell.h"

#include <winsock2.h>

#include <shellapi.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "demo_support.h"
#include "shell_app.h"
#include "shell_host.h"

#if defined(_WIN32)

namespace litheview_demo {

namespace {

int FindAvailableLoopbackPort() {
  WSADATA winsock_data = {};
  if (WSAStartup(MAKEWORD(2, 2), &winsock_data) != 0) {
    return 0;
  }
  const SOCKET socket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_handle == INVALID_SOCKET) {
    WSACleanup();
    return 0;
  }
  sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = 0;
  int port = 0;
  if (bind(socket_handle, reinterpret_cast<const sockaddr*>(&address),
           sizeof(address)) == 0) {
    int address_size = sizeof(address);
    if (getsockname(socket_handle, reinterpret_cast<sockaddr*>(&address),
                    &address_size) == 0) {
      port = ntohs(address.sin_port);
    }
  }
  closesocket(socket_handle);
  WSACleanup();
  return port;
}

int ReadRemoteDebuggingPortFromCommandLine() {
  int argument_count = 0;
  LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
  int port = 0;
  for (int index = 1; arguments && index < argument_count; ++index) {
    constexpr std::wstring_view kPortPrefix = L"--remote-debugging-port=";
    const std::wstring_view argument(arguments[index]);
    if (argument.starts_with(kPortPrefix)) {
      port = _wtoi(arguments[index] + kPortPrefix.size());
      break;
    }
  }
  if (arguments) {
    LocalFree(arguments);
  }
  return port > 0 && port <= 65535 ? port : 0;
}

}  // namespace

ShellApp::ShellApp(int devtools_frontend_port)
    : devtools_frontend_port_(devtools_frontend_port) {}

ShellApp::~ShellApp() = default;

bool ShellApp::OpenInitialWindow() {
  auto host = std::make_unique<ShellHost>(this, nullptr, std::string(), true,
                                          ShellWindowMode::kBrowser);
  if (!host->Open()) {
    return false;
  }
  ++open_window_count_;
  windows_.push_back(std::move(host));
  return true;
}

bool ShellApp::OpenInspectorWindow(std::string initial_url) {
  auto host = std::make_unique<ShellHost>(this, nullptr, std::move(initial_url),
                                          true, ShellWindowMode::kInspector);
  if (!host->Open()) {
    return false;
  }
  ++open_window_count_;
  windows_.push_back(std::move(host));
  return true;
}

void ShellApp::OpenPopup(ltv_view_t popup, const ltv_popup_request_t* request) {
  if (!popup) {
    return;
  }
  const std::string target_url =
      request && request->target_url ? request->target_url : "about:blank";
  auto host = std::make_unique<ShellHost>(this, popup, target_url, false,
                                          ShellWindowMode::kBrowser);
  if (!host->Open()) {
    return;
  }
  ++open_window_count_;
  windows_.push_back(std::move(host));
}

bool ShellApp::OpenDevTools(ltv_view_t inspected_view) {
  char* target_url = nullptr;
  if (devtools_frontend_port_ <= 0 ||
      ltv_view_get_devtools_target_url(inspected_view, &target_url) != LTV_OK ||
      !target_url) {
    ltv_free(target_url);
    return false;
  }
  std::wstring executable_path(MAX_PATH, L'\0');
  const DWORD executable_length =
      GetModuleFileNameW(nullptr, executable_path.data(),
                         static_cast<DWORD>(executable_path.size()));
  if (executable_length == 0 || executable_length >= executable_path.size()) {
    ltv_free(target_url);
    return false;
  }
  executable_path.resize(executable_length);
  const size_t separator = executable_path.find_last_of(L"\\/");
  executable_path.resize(separator == std::wstring::npos ? 0 : separator + 1);
  executable_path.append(L"litheview_inspector.exe");

  std::wstring command_line = L"\"" + executable_path +
                              L"\" --devtools-target=\"" +
                              Utf8ToWide(target_url) + L"\" --frontend-port=" +
                              std::to_wstring(devtools_frontend_port_);
  ltv_free(target_url);
  STARTUPINFOW startup_info = {};
  startup_info.cb = sizeof(startup_info);
  PROCESS_INFORMATION process_info = {};
  const BOOL created = CreateProcessW(
      executable_path.c_str(), command_line.data(), nullptr, nullptr, FALSE, 0,
      nullptr, nullptr, &startup_info, &process_info);
  if (!created) {
    return false;
  }
  CloseHandle(process_info.hThread);
  CloseHandle(process_info.hProcess);
  return true;
}

void ShellApp::OnWindowDestroyed() {
  if (open_window_count_ == 0) {
    return;
  }
  --open_window_count_;
  if (open_window_count_ == 0) {
    ltv_shutdown();
  }
}

namespace {

struct ShellState {
  int exit_code = 1;
  int devtools_frontend_port = 0;
  std::unique_ptr<ShellApp> app;
};

void LTV_CALLBACK OnReady(void* user_data) {
  auto* state = static_cast<ShellState*>(user_data);
  state->app = std::make_unique<ShellApp>(state->devtools_frontend_port);
  if (!state->app->OpenInitialWindow()) {
    ltv_shutdown();
    return;
  }
  state->exit_code = 0;
}

}  // namespace

int RunDemo(int argc, const char* const* argv) {
  int remote_debugging_port = ReadRemoteDebuggingPortFromCommandLine();
  if (remote_debugging_port == 0) {
    remote_debugging_port = FindAvailableLoopbackPort();
  }
  int devtools_frontend_port = FindAvailableLoopbackPort();
  for (int attempt = 0;
       devtools_frontend_port == remote_debugging_port && attempt < 4;
       ++attempt) {
    devtools_frontend_port = FindAvailableLoopbackPort();
  }
  if (remote_debugging_port == 0 || devtools_frontend_port == 0 ||
      remote_debugging_port == devtools_frontend_port) {
    return 1;
  }
  const ltv_settings_t settings = {
      .struct_size = sizeof(ltv_settings_t),
      .version = LTV_STRUCT_VERSION,
      .width_dip = kViewWidth,
      .height_dip = kViewHeight,
      .device_scale_factor =
          static_cast<float>(GetDpiForSystem()) / USER_DEFAULT_SCREEN_DPI,
      .remote_debugging_port = remote_debugging_port,
      .devtools_frontend_port = devtools_frontend_port,
      .remote_debugging_enabled = 1,
  };
  if (ltv_initialize(&settings) != LTV_OK) {
    return 1;
  }
  ShellState state = {.devtools_frontend_port = devtools_frontend_port};
  const int content_exit_code = ltv_run(argc, argv, &OnReady, &state);
  return content_exit_code == 0 ? state.exit_code : content_exit_code;
}

}  // namespace litheview_demo

#else

namespace litheview_demo {

int RunDemo(int, const char* const*) {
  return static_cast<int>(LTV_ERR_UNSUPPORTED);
}

}  // namespace litheview_demo

#endif
