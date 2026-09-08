#include <algorithm>
#include <iterator>
#include <utility>

#include "shell_app.h"
#include "shell_host.h"

namespace litheview_demo {

ShellHost::ShellHost(ShellApp* app,
                     ltv_view_t view,
                     std::string initial_url,
                     bool navigate_on_open,
                     ShellWindowMode window_mode)
    : app_(app),
      view_(view),
      initial_url_(std::move(initial_url)),
      navigate_on_open_(navigate_on_open),
      window_mode_(window_mode) {}

ShellHost::~ShellHost() {
  DestroyView();
}

bool ShellHost::Open() {
  native_scale_factor_ =
      static_cast<float>(GetDpiForSystem()) / USER_DEFAULT_SCREEN_DPI;
  if (ltv_win_get_device_scale_factor(&device_scale_factor_) != LTV_OK) {
    return false;
  }

  WNDCLASS window_class = {};
  window_class.lpfnWndProc = &ShellHost::WindowProc;
  window_class.hInstance = GetModuleHandle(nullptr);
  window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
  window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  window_class.lpszClassName = kWindowClass;
  RegisterClass(&window_class);

  window_ = CreateWindowEx(0, kWindowClass, kWindowTitle,
                           WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT,
                           CW_USEDEFAULT, ScaleNativeDip(kDefaultWindowWidth),
                           ScaleNativeDip(kDefaultWindowHeight), nullptr,
                           nullptr, window_class.hInstance, this);
  if (!window_ || (HasNavigationControls() && !CreateApplicationMenu())) {
    if (window_) {
      DestroyWindow(window_);
      window_ = nullptr;
    }
    return false;
  }

  if (HasNavigationControls()) {
    back_button_ = CreateWindow(
        L"BUTTON", L"\x2190", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0,
        window_, ControlId(kBackId), window_class.hInstance, nullptr);
    forward_button_ = CreateWindow(
        L"BUTTON", L"\x2192", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0,
        window_, ControlId(kForwardId), window_class.hInstance, nullptr);
    reload_button_ = CreateWindow(
        L"BUTTON", L"\x21BB", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0, 0,
        window_, ControlId(kReloadId), window_class.hInstance, nullptr);
    address_ = CreateWindowEx(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0,
        window_, ControlId(kAddressId), window_class.hInstance, nullptr);
    menu_button_ = CreateWindow(
        L"BUTTON", L"\x66F4\x591A", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 0,
        0, window_, ControlId(kMenuId), window_class.hInstance, nullptr);
  }
  render_window_ = CreateWindowEx(
      0, L"STATIC", L"",
      WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0,
      NavigationBarHeightPixels(), 0, 0, window_, ControlId(kRenderSurfaceId),
      window_class.hInstance, nullptr);
  gpu_output_window_ = CreateWindowEx(
      0, L"STATIC", L"", WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 0,
      0, window_, nullptr, window_class.hInstance, nullptr);
  if (!render_window_ || !gpu_output_window_ ||
      (HasNavigationControls() &&
       (!back_button_ || !forward_button_ || !reload_button_ || !address_ ||
        !menu_button_))) {
    DestroyWindow(window_);
    window_ = nullptr;
    return false;
  }

  if (HasNavigationControls()) {
    const HFONT ui_font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    for (HWND control : {back_button_, forward_button_, reload_button_,
                         address_, menu_button_}) {
      SendMessage(control, WM_SETFONT, reinterpret_cast<WPARAM>(ui_font), TRUE);
    }
    EnableWindow(back_button_, FALSE);
    EnableWindow(forward_button_, FALSE);
    for (HWND button :
         {back_button_, forward_button_, reload_button_, menu_button_}) {
      SetWindowLongPtr(button, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
      WNDPROC previous = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
          button, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&ButtonProc)));
      if (!button_window_proc_) {
        button_window_proc_ = previous;
      }
    }
  }
  SetWindowLongPtr(render_window_, GWLP_USERDATA,
                   reinterpret_cast<LONG_PTR>(this));
  render_window_proc_ = reinterpret_cast<WNDPROC>(
      SetWindowLongPtr(render_window_, GWLP_WNDPROC,
                       reinterpret_cast<LONG_PTR>(&RenderSurfaceProc)));
  SetWindowLongPtr(gpu_output_window_, GWLP_USERDATA,
                   reinterpret_cast<LONG_PTR>(this));
  SetWindowLongPtr(gpu_output_window_, GWLP_WNDPROC,
                   reinterpret_cast<LONG_PTR>(&RenderSurfaceProc));
  ImmAssociateContextEx(render_window_, nullptr, IACE_DEFAULT);
  ImmAssociateContextEx(gpu_output_window_, nullptr, IACE_DEFAULT);

  if (!RegisterFileDropTargets()) {
    DestroyWindow(window_);
    window_ = nullptr;
    return false;
  }
  Layout();

  if (initial_url_.empty()) {
    initial_url_ = kDefaultInitialUrl;
    int argument_count = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
    if (arguments && argument_count > 1) {
      initial_url_ = WideToUtf8(arguments[1]);
    }
    if (arguments) {
      LocalFree(arguments);
    }
  }
  presenter_ = std::make_unique<SharedTexturePresenter>(gpu_output_window_);
  if (!InitializeView()) {
    DestroyWindow(window_);
    window_ = nullptr;
    return false;
  }
  if (address_) {
    SetWindowTextW(address_, Utf8ToWide(initial_url_).c_str());
    SetWindowLongPtr(address_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    address_window_proc_ = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
        address_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&AddressProc)));
  }
  window_ready_ = true;
  ShowWindow(window_, SW_SHOW);
  UpdateWindow(window_);
  PostMessage(window_, kInitializePageMessage, 0, 0);
  return true;
}

LRESULT CALLBACK ShellHost::AddressProc(HWND window,
                                        UINT message,
                                        WPARAM wparam,
                                        LPARAM lparam) {
  auto* self =
      reinterpret_cast<ShellHost*>(GetWindowLongPtr(window, GWLP_USERDATA));
  if (self && self->HandleDevToolsKeyMessage(message, wparam, lparam)) {
    return 0;
  }
  const bool is_select_all_key = message == WM_KEYDOWN && wparam == 'A' &&
                                 (GetKeyState(VK_CONTROL) & 0x8000) != 0 &&
                                 (GetKeyState(VK_MENU) & 0x8000) == 0;
  const bool is_select_all_character = message == WM_CHAR && wparam == 1;
  if (self && (is_select_all_key || is_select_all_character)) {
    SendMessage(window, EM_SETSEL, 0, GetWindowTextLength(window));
    return 0;
  }
  if (self && wparam == VK_RETURN &&
      (message == WM_KEYDOWN || message == WM_CHAR || message == WM_KEYUP)) {
    if (message == WM_KEYDOWN) {
      self->Navigate();
    }
    return 0;
  }
  return self ? CallWindowProc(self->address_window_proc_, window, message,
                               wparam, lparam)
              : DefWindowProc(window, message, wparam, lparam);
}

LRESULT CALLBACK ShellHost::ButtonProc(HWND window,
                                       UINT message,
                                       WPARAM wparam,
                                       LPARAM lparam) {
  auto* self =
      reinterpret_cast<ShellHost*>(GetWindowLongPtr(window, GWLP_USERDATA));
  if (self && self->HandleDevToolsKeyMessage(message, wparam, lparam)) {
    return 0;
  }
  return self ? CallWindowProc(self->button_window_proc_, window, message,
                               wparam, lparam)
              : DefWindowProc(window, message, wparam, lparam);
}

LRESULT CALLBACK ShellHost::WindowProc(HWND window,
                                       UINT message,
                                       WPARAM wparam,
                                       LPARAM lparam) {
  ShellHost* self =
      reinterpret_cast<ShellHost*>(GetWindowLongPtr(window, GWLP_USERDATA));
  if (message == WM_NCCREATE) {
    auto* create = reinterpret_cast<CREATESTRUCT*>(lparam);
    self = static_cast<ShellHost*>(create->lpCreateParams);
    self->window_ = window;
    SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
  }
  return self ? self->HandleMessage(message, wparam, lparam)
              : DefWindowProc(window, message, wparam, lparam);
}

LRESULT CALLBACK ShellHost::RenderSurfaceProc(HWND window,
                                              UINT message,
                                              WPARAM wparam,
                                              LPARAM lparam) {
  auto* self =
      reinterpret_cast<ShellHost*>(GetWindowLongPtr(window, GWLP_USERDATA));
  return self
             ? self->HandleRenderSurfaceMessage(window, message, wparam, lparam)
             : DefWindowProc(window, message, wparam, lparam);
}

LRESULT ShellHost::HandleMessage(UINT message, WPARAM wparam, LPARAM lparam) {
  if (HandleDevToolsKeyMessage(message, wparam, lparam)) {
    return 0;
  }
  switch (message) {
    case WM_SIZE:
      Layout();
      return 0;
    case WM_GETMINMAXINFO: {
      auto* bounds = reinterpret_cast<MINMAXINFO*>(lparam);
      bounds->ptMinTrackSize.x = ScaleNativeDip(kMinimumWindowWidth);
      bounds->ptMinTrackSize.y = ScaleNativeDip(kMinimumWindowHeight);
      return 0;
    }
    case kInitializePageMessage:
      ResizeView();
      if (navigate_on_open_) {
        Navigate();
      }
      return 0;
    case kViewClosedMessage:
      SendMessageW(window_, WM_CLOSE, 0, 0);
      return 0;
    case WM_ENTERSIZEMOVE:
      SuspendGpuOutputForResize();
      return 0;
    case WM_SIZING:
      return TRUE;
    case WM_EXITSIZEMOVE:
      Layout();
      ResumeGpuOutputAfterResize();
      return 0;
    case WM_DPICHANGED: {
      const auto* suggested = reinterpret_cast<const RECT*>(lparam);
      native_scale_factor_ =
          static_cast<float>(HIWORD(wparam)) / USER_DEFAULT_SCREEN_DPI;
      if (ltv_win_set_device_scale_factor(native_scale_factor_) == LTV_OK) {
        ltv_win_get_device_scale_factor(&device_scale_factor_);
      }
      SetWindowPos(window_, nullptr, suggested->left, suggested->top,
                   suggested->right - suggested->left,
                   suggested->bottom - suggested->top,
                   SWP_NOACTIVATE | SWP_NOZORDER);
      Layout();
      return 0;
    }
    case WM_COMMAND: {
      const int command = LOWORD(wparam);
      if (command >= kDemoPageFirstId &&
          command <
              kDemoPageFirstId + static_cast<int>(std::size(kDemoPages))) {
        LoadDemoPage(kDemoPages[command - kDemoPageFirstId]);
        return 0;
      }
      switch (command) {
        case kBackId:
          gpu_output_pending_ = false;
          if (view_ && ConfigureOutputForPage(DemoPage::kExternal)) {
            current_page_ = DemoPage::kExternal;
            UpdateDemoPageMenu();
            ltv_view_go_back(view_);
          }
          return 0;
        case kForwardId:
          gpu_output_pending_ = false;
          if (view_ && ConfigureOutputForPage(DemoPage::kExternal)) {
            current_page_ = DemoPage::kExternal;
            UpdateDemoPageMenu();
            ltv_view_go_forward(view_);
          }
          return 0;
        case kReloadId:
          if (view_) {
            if (is_loading_) {
              ltv_view_stop(view_);
            } else if (GetDemoPageContent(current_page_).uses_gpu_output) {
              gpu_output_pending_ = false;
              if (ConfigureOutputForPage(DemoPage::kExternal)) {
                gpu_output_pending_ = true;
                if (ltv_view_reload(view_) != LTV_OK) {
                  gpu_output_pending_ = false;
                  ConfigureOutputForPage(current_page_);
                }
              }
            } else {
              ltv_view_reload(view_);
            }
          }
          return 0;
        case kMenuId:
          ShowApplicationMenu();
          return 0;
        case kDevToolsId:
          if (view_ && !app_->OpenDevTools(view_)) {
            MessageBoxW(
                window_,
                L"\x65E0\x6CD5\x6253\x5F00\x5F00\x53D1\x8005\x5DE5\x5177\x3002",
                L"LitheView DevTools", MB_OK | MB_ICONERROR);
          }
          return 0;
        case kExitId:
          SendMessageW(window_, WM_CLOSE, 0, 0);
          return 0;
      }
      break;
    }
    case WM_SYSCOMMAND: {
      ltv_win_set_os_modal_loop(1);
      const LRESULT result = DefWindowProc(window_, message, wparam, lparam);
      ltv_win_set_os_modal_loop(0);
      return result;
    }
    case WM_PAINT:
      Paint();
      return 0;
    case WM_SETCURSOR:
      if (LOWORD(lparam) == HTCLIENT && SetPageCursor()) {
        return TRUE;
      }
      break;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_CAPTURECHANGED:
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
      return HandlePageMessage(window_, message, wparam, lparam);
    case WM_CLOSE:

      DestroyView();
      DestroyWindow(window_);
      return 0;
    case WM_DESTROY:

      DestroyView();
      if (application_menu_) {
        DestroyMenu(application_menu_);
        application_menu_ = nullptr;
      }
      window_ = nullptr;
      if (window_ready_) {
        window_ready_ = false;
        app_->OnWindowDestroyed();
      }
      return 0;
  }
  return DefWindowProc(window_, message, wparam, lparam);
}

void ShellHost::Paint() {
  PAINTSTRUCT paint;
  BeginPaint(window_, &paint);
  EndPaint(window_, &paint);
}

}  // namespace litheview_demo
