#include "native_browser_surface.h"

#include <litheview/litheview_win.h>
#include <windowsx.h>

#include <QWidget>
#include <algorithm>
#include <cmath>

namespace litheview_qt_demo {
namespace {

constexpr wchar_t kBrowserSurfaceClassName[] = L"LitheViewQtAcceleratedSurface";

}  // namespace

NativeBrowserSurface::~NativeBrowserSurface() {
  Detach();
}

bool NativeBrowserSurface::Attach(ltv_view_t view, QWidget* host) {
  if (!view || !host || view_ || window_) {
    return false;
  }
  view_ = view;
  host_ = host;
  if (!CreateNativeWindow() ||
      ltv_win_view_attach(view_, reinterpret_cast<uintptr_t>(window_)) !=
          LTV_OK) {
    Detach();
    return false;
  }
  attached_ = true;
  if (!ResizeView()) {
    Detach();
    return false;
  }
  return true;
}

void NativeBrowserSurface::Detach() {
  CancelPreparedResize();
  if (attached_) {
    ltv_win_view_detach(view_);
    attached_ = false;
  }
  HWND window = window_;
  window_ = nullptr;
  if (window && IsWindow(window)) {
    DestroyWindow(window);
  }
  view_ = nullptr;
  host_ = nullptr;
  surface_width_pixels_ = 0;
  surface_height_pixels_ = 0;
}

void NativeBrowserSurface::ResizeToHost() {
  if (!window_ || !host_) {
    return;
  }
  const HWND parent = reinterpret_cast<HWND>(host_->winId());
  RECT bounds = {};
  if (!GetClientRect(parent, &bounds)) {
    return;
  }
  SetWindowPos(window_, HWND_TOP, 0, 0, std::max(1L, bounds.right),
               std::max(1L, bounds.bottom), SWP_NOACTIVATE);
}

void NativeBrowserSurface::Refresh() {
  if (attached_) {
    ResizeView();
  }
}

void NativeBrowserSurface::FinishResize() {
  if (!attached_) {
    return;
  }
  ltv_win_view_cancel_native_resize(view_);
  resize_prepared_ = false;
}

void NativeBrowserSurface::Focus() {
  if (window_) {
    SetFocus(window_);
  }
}

bool NativeBrowserSurface::CreateNativeWindow() {
  WNDCLASSEXW window_class = {};
  window_class.cbSize = sizeof(window_class);
  window_class.lpfnWndProc = &NativeBrowserSurface::WindowProc;
  window_class.hInstance = GetModuleHandleW(nullptr);
  window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  window_class.lpszClassName = kBrowserSurfaceClassName;
  if (!RegisterClassExW(&window_class) &&
      GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    return false;
  }

  const HWND parent = reinterpret_cast<HWND>(host_->winId());
  RECT bounds = {};
  if (!GetClientRect(parent, &bounds)) {
    return false;
  }
  window_ = CreateWindowExW(
      WS_EX_NOPARENTNOTIFY | WS_EX_NOREDIRECTIONBITMAP,
      kBrowserSurfaceClassName, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0,
      0, std::max(1L, bounds.right), std::max(1L, bounds.bottom), parent,
      nullptr, GetModuleHandleW(nullptr), this);
  return window_ != nullptr;
}

bool NativeBrowserSurface::HandleMessage(UINT message,
                                         WPARAM wparam,
                                         LPARAM lparam,
                                         LRESULT* result) {
  if (!attached_) {
    return false;
  }
  switch (message) {
    case WM_WINDOWPOSCHANGING: {
      const auto* position = reinterpret_cast<const WINDOWPOS*>(lparam);
      if (position && !(position->flags & SWP_NOSIZE)) {
        PrepareResize(position->cx, position->cy);
      }
      break;
    }
    case WM_WINDOWPOSCHANGED: {
      const auto* position = reinterpret_cast<const WINDOWPOS*>(lparam);
      if (position && !(position->flags & SWP_NOSIZE) && position->cx > 0 &&
          position->cy > 0) {
        const qreal scale = host_->devicePixelRatioF();
        ResizeView(
            std::max(1, static_cast<int>(std::lround(position->cx / scale))),
            std::max(1, static_cast<int>(std::lround(position->cy / scale))),
            position->cx, position->cy);
      } else {
        CancelPreparedResize();
      }
      break;
    }
    case WM_SETCURSOR:
      if (LOWORD(lparam) == HTCLIENT && SetPageCursor()) {
        *result = TRUE;
        return true;
      }
      break;
    case WM_SETFOCUS:
      ltv_view_set_focus(view_, 1);
      break;
    case WM_KILLFOCUS:
      ltv_view_set_focus(view_, 0);
      break;
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_CHAR:
      ltv_win_view_forward_keyboard_message(view_, message, wparam, lparam);
      *result = 0;
      return true;
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
      ForwardMouseMessage(message, wparam, lparam);
      if (message == WM_MOUSEMOVE) {
        SetPageCursor();
      }
      break;
    case WM_ERASEBKGND:
      *result = 1;
      return true;
    default:
      break;
  }
  return false;
}

bool NativeBrowserSurface::ResizeView() {
  RECT bounds = {};
  if (!attached_ || !GetClientRect(window_, &bounds) || host_->width() <= 0 ||
      host_->height() <= 0) {
    CancelPreparedResize();
    return false;
  }
  return ResizeView(host_->width(), host_->height(), std::max(1L, bounds.right),
                    std::max(1L, bounds.bottom));
}

bool NativeBrowserSurface::ResizeView(int width_dip,
                                      int height_dip,
                                      int width_pixels,
                                      int height_pixels) {
  if (!attached_ || width_dip <= 0 || height_dip <= 0 || width_pixels <= 0 ||
      height_pixels <= 0) {
    CancelPreparedResize();
    return false;
  }
  const bool resized =
      ltv_win_view_resize(view_, width_dip, height_dip, width_pixels,
                          height_pixels) == LTV_OK;
  if (resized) {
    surface_width_pixels_ = width_pixels;
    surface_height_pixels_ = height_pixels;
    resize_prepared_ = false;
  } else {
    CancelPreparedResize();
  }
  return resized;
}

void NativeBrowserSurface::PrepareResize(int width_pixels, int height_pixels) {
  if (width_pixels <= 0 || height_pixels <= 0 || resize_prepared_ ||
      (width_pixels == surface_width_pixels_ &&
       height_pixels == surface_height_pixels_)) {
    return;
  }
  resize_prepared_ = ltv_win_view_prepare_native_resize(view_) == LTV_OK;
}

void NativeBrowserSurface::CancelPreparedResize() {
  if (!resize_prepared_ || !view_) {
    return;
  }
  ltv_win_view_cancel_native_resize(view_);
  resize_prepared_ = false;
}

void NativeBrowserSurface::ForwardMouseMessage(uint32_t message,
                                               uintptr_t key_state,
                                               intptr_t position) {
  LPARAM local_position = position;
  LPARAM screen_position = position;
  if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL) {
    POINT point = {GET_X_LPARAM(position), GET_Y_LPARAM(position)};
    ScreenToClient(window_, &point);
    local_position = MAKELPARAM(point.x, point.y);
  } else {
    POINT point = {GET_X_LPARAM(position), GET_Y_LPARAM(position)};
    ClientToScreen(window_, &point);
    screen_position = MAKELPARAM(point.x, point.y);
  }

  if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
      message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN) {
    SetCapture(window_);
    host_->setFocus(Qt::MouseFocusReason);
    SetFocus(window_);
  }

  ltv_win_view_forward_mouse_message(view_, message, key_state, local_position,
                                     screen_position);

  if (message == WM_LBUTTONUP || message == WM_RBUTTONUP ||
      message == WM_MBUTTONUP || message == WM_XBUTTONUP) {
    ReleaseCapture();
  }
}

bool NativeBrowserSurface::SetPageCursor() const {
  POINT point;
  if (!GetCursorPos(&point) || !ScreenToClient(window_, &point)) {
    return false;
  }

  const qreal scale = host_->devicePixelRatioF();
  ltv_cursor_type_t cursor = LTV_CURSOR_ARROW;
  if (ltv_win_view_get_cursor(
          view_, static_cast<int>(std::lround(point.x / scale)),
          static_cast<int>(std::lround(point.y / scale)), &cursor) != LTV_OK) {
    return false;
  }

  LPCWSTR resource = IDC_ARROW;
  switch (cursor) {
    case LTV_CURSOR_CROSS:
      resource = IDC_CROSS;
      break;
    case LTV_CURSOR_HAND:
      resource = IDC_HAND;
      break;
    case LTV_CURSOR_IBEAM:
      resource = IDC_IBEAM;
      break;
    case LTV_CURSOR_WAIT:
      resource = IDC_WAIT;
      break;
    case LTV_CURSOR_PROGRESS:
      resource = IDC_APPSTARTING;
      break;
    case LTV_CURSOR_HELP:
      resource = IDC_HELP;
      break;
    case LTV_CURSOR_SIZE_WE:
      resource = IDC_SIZEWE;
      break;
    case LTV_CURSOR_SIZE_NS:
      resource = IDC_SIZENS;
      break;
    case LTV_CURSOR_SIZE_NESW:
      resource = IDC_SIZENESW;
      break;
    case LTV_CURSOR_SIZE_NWSE:
      resource = IDC_SIZENWSE;
      break;
    case LTV_CURSOR_SIZE_ALL:
      resource = IDC_SIZEALL;
      break;
    case LTV_CURSOR_NOT_ALLOWED:
      resource = IDC_NO;
      break;
    case LTV_CURSOR_HIDDEN:
      SetCursor(nullptr);
      return true;
    default:
      break;
  }
  SetCursor(LoadCursorW(nullptr, resource));
  return true;
}

LRESULT CALLBACK NativeBrowserSurface::WindowProc(HWND window,
                                                  UINT message,
                                                  WPARAM wparam,
                                                  LPARAM lparam) {
  auto* self = reinterpret_cast<NativeBrowserSurface*>(
      GetWindowLongPtrW(window, GWLP_USERDATA));
  if (message == WM_NCCREATE) {
    auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
    self = static_cast<NativeBrowserSurface*>(create->lpCreateParams);
    SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
  }
  LRESULT result = 0;
  if (self && self->HandleMessage(message, wparam, lparam, &result)) {
    return result;
  }
  if (message == WM_NCDESTROY) {
    SetWindowLongPtrW(window, GWLP_USERDATA, 0);
  }
  return DefWindowProcW(window, message, wparam, lparam);
}

}  // namespace litheview_qt_demo
