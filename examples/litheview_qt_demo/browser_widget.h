#ifndef LITHEVIEW_EXAMPLES_LITHEVIEW_QT_DEMO_BROWSER_WIDGET_H_
#define LITHEVIEW_EXAMPLES_LITHEVIEW_QT_DEMO_BROWSER_WIDGET_H_

#include <litheview/litheview.h>

#include <QWidget>
#include <cstdint>
#include <memory>

class QEvent;
class QFocusEvent;
class QResizeEvent;

namespace litheview_qt_demo {

class BrowserWindow;
class NativeBrowserSurface;

class BrowserWidget final : public QWidget {
 public:
  explicit BrowserWidget(BrowserWindow* owner);
  ~BrowserWidget() override;

  bool Initialize();
  void Shutdown();
  void Navigate(const QString& input);
  void GoBack();
  void GoForward();
  void Reload();
  void Stop();
  void RefreshNativeSurface();
  void FinishNativeResize();
  uintptr_t NativeSurfaceWindow() const;

 protected:
  void resizeEvent(QResizeEvent* event) override;
  void focusInEvent(QFocusEvent* event) override;
  bool event(QEvent* event) override;

 private:
  static void LTV_CALLBACK OnUrlChanged(ltv_view_t view,
                                        const char* url,
                                        void* user_data);
  static void LTV_CALLBACK OnTitleChanged(ltv_view_t view,
                                          const char* title,
                                          void* user_data);
  static void LTV_CALLBACK OnLoadError(ltv_view_t view,
                                       const char* url,
                                       int32_t error_code,
                                       const char* error_text,
                                       void* user_data);
  static void LTV_CALLBACK OnClosed(ltv_view_t view, void* user_data);
  static void LTV_CALLBACK
  OnRendererTerminated(ltv_view_t view,
                       ltv_renderer_termination_status_t status,
                       void* user_data);
  static ltv_close_decision_t LTV_CALLBACK OnCloseRequested(ltv_view_t view,
                                                            void* user_data);
  static void LTV_CALLBACK
  OnNavigationStateChanged(ltv_view_t view,
                           const ltv_navigation_state_t* state,
                           void* user_data);

  BrowserWindow* const owner_;
  ltv_view_t view_ = nullptr;
  std::unique_ptr<NativeBrowserSurface> surface_;
};

}  // namespace litheview_qt_demo

#endif  // LITHEVIEW_EXAMPLES_LITHEVIEW_QT_DEMO_BROWSER_WIDGET_H_
