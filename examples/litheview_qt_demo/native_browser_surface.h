#ifndef LITHEVIEW_EXAMPLES_LITHEVIEW_QT_DEMO_NATIVE_BROWSER_SURFACE_H_
#define LITHEVIEW_EXAMPLES_LITHEVIEW_QT_DEMO_NATIVE_BROWSER_SURFACE_H_

#include <windows.h>

#include <litheview/litheview.h>

#include <cstdint>

class QWidget;

namespace litheview_qt_demo {

class NativeBrowserSurface final {
 public:
  NativeBrowserSurface() = default;
  NativeBrowserSurface(const NativeBrowserSurface&) = delete;
  NativeBrowserSurface& operator=(const NativeBrowserSurface&) = delete;
  ~NativeBrowserSurface();

  bool Attach(ltv_view_t view, QWidget* host);
  void Detach();
  void ResizeToHost();
  void Refresh();
  void FinishResize();
  void Focus();

  uintptr_t window() const { return reinterpret_cast<uintptr_t>(window_); }

 private:
  bool CreateNativeWindow();
  bool HandleMessage(UINT message,
                     WPARAM wparam,
                     LPARAM lparam,
                     LRESULT* result);
  bool ResizeView();
  bool ResizeView(int width_dip,
                  int height_dip,
                  int width_pixels,
                  int height_pixels);
  void PrepareResize(int width_pixels, int height_pixels);
  void CancelPreparedResize();
  void ForwardMouseMessage(uint32_t message,
                           uintptr_t key_state,
                           intptr_t position);
  bool SetPageCursor() const;

  static LRESULT CALLBACK WindowProc(HWND window,
                                     UINT message,
                                     WPARAM wparam,
                                     LPARAM lparam);

  ltv_view_t view_ = nullptr;
  QWidget* host_ = nullptr;
  HWND window_ = nullptr;
  bool attached_ = false;
  bool resize_prepared_ = false;
  int surface_width_pixels_ = 0;
  int surface_height_pixels_ = 0;
};

}  // namespace litheview_qt_demo

#endif  // LITHEVIEW_EXAMPLES_LITHEVIEW_QT_DEMO_NATIVE_BROWSER_SURFACE_H_
