#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>
#include <vector>

#include "native_dialogs.h"
#include "shell_app.h"
#include "shell_host.h"

namespace litheview_demo {

LRESULT ShellHost::HandleRenderSurfaceMessage(HWND window,
                                              UINT message,
                                              WPARAM wparam,
                                              LPARAM lparam) {
  if (HandleDevToolsKeyMessage(message, wparam, lparam)) {
    return 0;
  }
  switch (message) {
    case WM_ERASEBKGND:
      return 1;
    case WM_PAINT: {
      PAINTSTRUCT paint;
      BeginPaint(window, &paint);
      EndPaint(window, &paint);
      return 0;
    }
    case WM_SETCURSOR:
      if (LOWORD(lparam) == HTCLIENT && SetPageCursor()) {
        return TRUE;
      }
      break;
    case WM_IME_SETCONTEXT:
      return CallWindowProc(render_window_proc_, window, message, wparam,
                            lparam & ~ISC_SHOWUICOMPOSITIONWINDOW);
    case WM_IME_STARTCOMPOSITION: {
      if (HIMC context = ImmGetContext(window)) {
        UpdateImeWindows(context);
        ImmReleaseContext(window, context);
      }
      return 0;
    }
    case WM_IME_COMPOSITION:
      HandleImeComposition(window, lparam);
      return 0;
    case WM_IME_ENDCOMPOSITION:
      if (view_) {
        ltv_win_view_ime_finish_composition(view_);
      }
      return 0;
    case WM_IME_NOTIFY: {
      if (HIMC context = ImmGetContext(window)) {
        UpdateImeWindows(context);
        ImmReleaseContext(window, context);
      }
      break;
    }
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
      return HandlePageMessage(window, message, wparam, lparam);
  }
  return CallWindowProc(render_window_proc_, window, message, wparam, lparam);
}

bool ShellHost::HandleDevToolsKeyMessage(UINT message,
                                         WPARAM wparam,
                                         LPARAM lparam) {
  if (window_mode_ != ShellWindowMode::kBrowser || wparam != VK_F12 ||
      (message != WM_KEYDOWN && message != WM_KEYUP)) {
    return false;
  }
  if (message == WM_KEYDOWN && (lparam & (static_cast<LPARAM>(1) << 30)) == 0 &&
      view_ && !app_->OpenDevTools(view_)) {
    MessageBoxW(window_,
                L"\x65E0\x6CD5\x6253\x5F00\x5F00\x53D1\x8005\x5DE5\x5177\x3002",
                L"LitheView DevTools", MB_OK | MB_ICONERROR);
  }
  return true;
}

LRESULT ShellHost::HandlePageMessage(HWND source,
                                     UINT message,
                                     WPARAM wparam,
                                     LPARAM lparam) {
  if (!view_ || !render_window_) {
    return 0;
  }

  if (message == WM_KEYDOWN || message == WM_KEYUP || message == WM_CHAR) {
    ltv_win_view_forward_keyboard_message(view_, message, wparam, lparam);
    return 0;
  }

  if (message == WM_CAPTURECHANGED) {
    if (mouse_button_captured_ &&
        reinterpret_cast<HWND>(lparam) != mouse_capture_window_) {
      const UINT button_up_message = captured_button_up_message_;
      mouse_button_captured_ = false;
      mouse_capture_window_ = nullptr;
      captured_button_up_message_ = 0;
      POINT point;
      if (button_up_message && GetCursorPos(&point) &&
          ScreenToClient(render_window_, &point)) {
        ltv_win_view_forward_mouse_message(
            view_, button_up_message, 0, MAKELPARAM(point.x, point.y),
            ToScreenPosition(render_window_, point.x, point.y));
      }
    }
    return 0;
  }

  if (message == WM_MOUSEWHEEL) {
    POINT point = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
    if (!ScreenToClient(render_window_, &point)) {
      return 0;
    }
    RECT client;
    GetClientRect(render_window_, &client);
    if (!PtInRect(&client, point)) {
      return 0;
    }
    ltv_win_view_forward_mouse_message(
        view_, message, wparam, MAKELPARAM(point.x, point.y),
        ToScreenPosition(render_window_, point.x, point.y));
    return 0;
  }

  int x = GET_X_LPARAM(lparam);
  int y = GET_Y_LPARAM(lparam);
  if (source == window_) {
    y -= NavigationBarHeightPixels();
  }
  if (y < 0 && !mouse_button_captured_) {
    return 0;
  }

  if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN) {
    SetCapture(source);
    mouse_button_captured_ = true;
    mouse_capture_window_ = source;
    captured_button_up_message_ =
        message == WM_LBUTTONDOWN ? WM_LBUTTONUP : WM_RBUTTONUP;
    SetFocus(render_window_);
  }

  ltv_win_view_forward_mouse_message(
      view_, message, wparam, MAKELPARAM(x, y),
      ToScreenPosition(source, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)));

  if ((message == WM_LBUTTONUP || message == WM_RBUTTONUP) &&
      mouse_button_captured_) {
    mouse_button_captured_ = false;
    mouse_capture_window_ = nullptr;
    captured_button_up_message_ = 0;
    ReleaseCapture();
  }
  return 0;
}

void ShellHost::HandleImeComposition(HWND window, LPARAM flags) {
  if (!view_) {
    return;
  }
  HIMC context = ImmGetContext(window);
  if (!context) {
    return;
  }

  if (flags & GCS_RESULTSTR) {
    const std::vector<uint16_t> result = ReadImeUtf16(context, GCS_RESULTSTR);
    ltv_win_view_ime_commit_text(view_, result.data(),
                                 static_cast<uint32_t>(result.size()));
  }

  if (flags & GCS_COMPSTR) {
    const std::vector<uint16_t> composition =
        ReadImeUtf16(context, GCS_COMPSTR);
    if (composition.empty()) {
      ltv_win_view_ime_cancel_composition(view_);
    } else {
      const std::vector<uint8_t> attributes =
          ReadImeBytes(context, GCS_COMPATTR);
      std::vector<ltv_win_ime_span_t> spans;
      uint32_t selection_start = static_cast<uint32_t>(std::clamp<LONG>(
          ImmGetCompositionStringW(context, GCS_CURSORPOS, nullptr, 0), 0,
          static_cast<LONG>(composition.size())));
      uint32_t selection_end = selection_start;

      if (attributes.size() == composition.size()) {
        bool target_selection_found = false;
        for (size_t start = 0; start < attributes.size();) {
          size_t end = start + 1;
          while (end < attributes.size() &&
                 attributes[end] == attributes[start]) {
            ++end;
          }
          const bool is_target = IsImeTargetAttribute(attributes[start]);
          spans.push_back({
              .struct_size = sizeof(ltv_win_ime_span_t),
              .version = LTV_STRUCT_VERSION,
              .start_offset = static_cast<uint32_t>(start),
              .end_offset = static_cast<uint32_t>(end),
              .thickness =
                  is_target ? LTV_WIN_IME_SPAN_THICK : LTV_WIN_IME_SPAN_THIN,
          });
          if (is_target && !target_selection_found) {
            selection_start = static_cast<uint32_t>(start);
            selection_end = static_cast<uint32_t>(end);
            target_selection_found = true;
          }
          start = end;
        }
      } else {
        spans.push_back({
            .struct_size = sizeof(ltv_win_ime_span_t),
            .version = LTV_STRUCT_VERSION,
            .start_offset = 0,
            .end_offset = static_cast<uint32_t>(composition.size()),
            .thickness = LTV_WIN_IME_SPAN_THIN,
        });
      }

      ltv_win_view_ime_set_composition(
          view_, composition.data(), static_cast<uint32_t>(composition.size()),
          spans.data(), static_cast<uint32_t>(spans.size()), selection_start,
          selection_end);
    }
  }

  UpdateImeWindows(context);
  ImmReleaseContext(window, context);
}

void ShellHost::UpdateImeWindows(HIMC context) {
  if (!view_ || !context || ime_window_update_in_progress_) {
    return;
  }
  ime_window_update_in_progress_ = true;
  ltv_win_rect_t caret = {
      .struct_size = sizeof(ltv_win_rect_t),
      .version = LTV_STRUCT_VERSION,
  };
  if (ltv_win_view_get_ime_caret_rect(view_, &caret) != LTV_OK) {
    ime_window_update_in_progress_ = false;
    return;
  }

  const POINT candidate_position = {
      static_cast<LONG>(std::lround(caret.x * device_scale_factor_)),
      static_cast<LONG>(std::lround((caret.y + std::max(1, caret.height)) *
                                    device_scale_factor_)),
  };
  CANDIDATEFORM candidate = {};
  candidate.dwIndex = 0;
  candidate.dwStyle = CFS_CANDIDATEPOS;
  candidate.ptCurrentPos = candidate_position;
  ImmSetCandidateWindow(context, &candidate);

  COMPOSITIONFORM composition = {};
  composition.dwStyle = CFS_POINT;
  composition.ptCurrentPos = candidate_position;
  ImmSetCompositionWindow(context, &composition);
  ime_window_update_in_progress_ = false;
}

LPARAM ShellHost::ToScreenPosition(HWND source, int x, int y) const {
  POINT point = {x, y};
  ClientToScreen(source, &point);
  return MAKELPARAM(point.x, point.y);
}

bool ShellHost::SetPageCursor() const {
  POINT point;
  if (!GetCursorPos(&point) || !render_window_ ||
      !ScreenToClient(render_window_, &point) || !view_) {
    return false;
  }

  ltv_cursor_type_t cursor = LTV_CURSOR_ARROW;
  if (ltv_win_view_get_cursor(
          view_, static_cast<int>(std::lround(point.x / device_scale_factor_)),
          static_cast<int>(std::lround(point.y / device_scale_factor_)),
          &cursor) != LTV_OK) {
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
  SetCursor(LoadCursor(nullptr, resource));
  return true;
}

}  // namespace litheview_demo
