#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

#include "shell_host.h"

namespace litheview_demo {

bool ShellHost::CreateApplicationMenu() {
  application_menu_ = CreatePopupMenu();
  if (!application_menu_) {
    return false;
  }
  if (window_mode_ == ShellWindowMode::kBrowser) {
    for (size_t index = 0; index < std::size(kDemoPages); ++index) {
      const DemoPageContent& content = GetDemoPageContent(kDemoPages[index]);
      AppendMenuW(application_menu_, MF_STRING,
                  kDemoPageFirstId + static_cast<UINT>(index),
                  content.menu_title);
    }
    AppendMenuW(application_menu_, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(application_menu_, MF_STRING, kDevToolsId,
                L"\x5F00\x53D1\x8005\x5DE5\x5177");
    AppendMenuW(application_menu_, MF_SEPARATOR, 0, nullptr);
  }
  AppendMenuW(application_menu_, MF_STRING, kExitId, L"\x9000\x51FA");
  return true;
}

void ShellHost::ShowApplicationMenu() {
  if (!window_ || !menu_button_ || !application_menu_) {
    return;
  }
  RECT button_bounds = {};
  if (!GetWindowRect(menu_button_, &button_bounds)) {
    return;
  }
  SetForegroundWindow(window_);
  TrackPopupMenuEx(application_menu_,
                   TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON,
                   button_bounds.right, button_bounds.bottom, window_, nullptr);
  PostMessage(window_, WM_NULL, 0, 0);
}

void ShellHost::UpdateDemoPageMenu() {
  if (!application_menu_) {
    return;
  }
  for (size_t index = 0; index < std::size(kDemoPages); ++index) {
    CheckMenuItem(application_menu_,
                  kDemoPageFirstId + static_cast<UINT>(index),
                  MF_BYCOMMAND | MF_UNCHECKED);
  }
  if (current_page_ != DemoPage::kExternal) {
    CheckMenuRadioItem(
        application_menu_, kDemoPageFirstId,
        kDemoPageFirstId + static_cast<UINT>(std::size(kDemoPages)) - 1,
        kDemoPageFirstId +
            static_cast<UINT>(static_cast<size_t>(current_page_) -
                              static_cast<size_t>(DemoPage::kOverview)),
        MF_BYCOMMAND);
  }
}

void ShellHost::AppendLog(std::string message) {
  message.append("\r\n");
  OutputDebugStringA(message.c_str());
}

void ShellHost::Layout() {
  if (!window_ || !render_window_) {
    return;
  }
  RECT client = {};
  GetClientRect(window_, &client);
  const int client_width = static_cast<int>(std::max(0L, client.right));
  const int client_height = static_cast<int>(std::max(0L, client.bottom));
  if (HasNavigationControls()) {
    const int margin = ScaleNativeDip(3);
    const int gap = ScaleNativeDip(3);
    const int button_width = ScaleNativeDip(34);
    const int menu_width = ScaleNativeDip(58);
    const int row_height = ScaleNativeDip(24);
    const int navigation_y = ScaleNativeDip(2);

    int x = margin;
    MoveWindow(back_button_, x, navigation_y, button_width, row_height, TRUE);
    x += button_width + gap;
    MoveWindow(forward_button_, x, navigation_y, button_width, row_height,
               TRUE);
    x += button_width + gap;
    MoveWindow(reload_button_, x, navigation_y, button_width, row_height, TRUE);
    x += button_width + gap;
    const int menu_x =
        std::max(x + ScaleNativeDip(80), client_width - margin - menu_width);
    MoveWindow(address_, x, navigation_y, std::max(40, menu_x - gap - x),
               row_height, TRUE);
    MoveWindow(menu_button_, menu_x, navigation_y, menu_width, row_height,
               TRUE);
  }

  const PixelSize target_surface_size = {
      client_width, std::max(0, client_height - NavigationBarHeightPixels())};
  const bool surface_size_changed = target_surface_size != PageSizePixels();
  const bool resize_prepared =
      surface_size_changed && view_ && view_attached_ &&
      ltv_win_view_prepare_native_resize(view_) == LTV_OK;
  constexpr UINT kSurfaceResizeFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER |
                                       SWP_NOREDRAW | SWP_NOSENDCHANGING |
                                       SWP_NOZORDER;
  if (!SetWindowPos(render_window_, nullptr, 0, NavigationBarHeightPixels(),
                    target_surface_size.width, target_surface_size.height,
                    kSurfaceResizeFlags)) {
    if (resize_prepared) {
      ltv_win_view_cancel_native_resize(view_);
    }
    return;
  }
  SetWindowPos(gpu_output_window_, HWND_TOP, 0, NavigationBarHeightPixels(),
               target_surface_size.width, target_surface_size.height,
               SWP_NOACTIVATE | SWP_NOOWNERZORDER);
  if (window_ready_) {
    ResizeView();
  }
}

int ShellHost::ScaleNativeDip(int value) const {
  return static_cast<int>(std::lround(value * native_scale_factor_));
}

int ShellHost::NavigationBarHeightPixels() const {
  return HasNavigationControls() ? ScaleNativeDip(kNavigationBarHeight) : 0;
}

bool ShellHost::HasNavigationControls() const {
  return window_mode_ == ShellWindowMode::kBrowser;
}

PixelSize ShellHost::PageSizePixels() const {
  if (!render_window_) {
    return {};
  }
  RECT client = {};
  GetClientRect(render_window_, &client);
  return {static_cast<int>(std::max(0L, client.right)),
          static_cast<int>(std::max(0L, client.bottom))};
}

void ShellHost::ResizeView() {
  if (!view_) {
    return;
  }
  const PixelSize page_size_pixels = PageSizePixels();
  const PixelSize view_size_dips = {
      static_cast<int>(
          std::ceil(page_size_pixels.width / device_scale_factor_)),
      static_cast<int>(
          std::ceil(page_size_pixels.height / device_scale_factor_))};
  if (view_size_dips.width <= 0 || view_size_dips.height <= 0 ||
      (view_size_dips == view_size_dips_ &&
       page_size_pixels == surface_size_pixels_)) {
    return;
  }
  if (ltv_win_view_resize(view_, view_size_dips.width, view_size_dips.height,
                          page_size_pixels.width,
                          page_size_pixels.height) != LTV_OK) {
    return;
  }
  view_size_dips_ = view_size_dips;
  surface_size_pixels_ = page_size_pixels;
}

void ShellHost::Navigate() {
  if (!view_) {
    return;
  }
  std::string requested_url = initial_url_;
  if (address_) {
    const int length = GetWindowTextLengthW(address_);
    std::wstring url(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(address_, url.data(), length + 1);
    url.resize(length);
    requested_url = WideToUtf8(url);
  }
  const std::string parsed_url = FixupUrl(std::move(requested_url));
  if (parsed_url.empty()) {
    return;
  }

  gpu_output_pending_ = false;
  if (!ConfigureOutputForPage(DemoPage::kExternal)) {
    SetWindowTextW(window_, kNavigationFailedTitle);
    return;
  }
  const bool navigation_started =
      ltv_view_navigate(view_, parsed_url.c_str()) == LTV_OK;
  if (navigation_started) {
    current_page_ = DemoPage::kExternal;
    current_url_ = parsed_url;
    UpdateDemoPageMenu();
  }
  SetWindowTextW(window_,
                 navigation_started ? kWindowTitle : kNavigationFailedTitle);
}

}  // namespace litheview_demo
