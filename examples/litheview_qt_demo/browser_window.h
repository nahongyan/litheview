#ifndef LITHEVIEW_EXAMPLES_LITHEVIEW_QT_DEMO_BROWSER_WINDOW_H_
#define LITHEVIEW_EXAMPLES_LITHEVIEW_QT_DEMO_BROWSER_WINDOW_H_

#include <QMainWindow>
#include <memory>

class QAction;
class QCloseEvent;
class QLineEdit;

namespace litheview_qt_demo {

class BrowserWidget;
class SystemModalLoopBridge;

class BrowserWindow final : public QMainWindow {
 public:
  BrowserWindow();
  ~BrowserWindow() override;

  bool Initialize();
  void SetPageUrl(const QString& url);
  void SetPageTitle(const QString& title);
  void SetNavigationState(bool can_go_back,
                          bool can_go_forward,
                          bool is_loading);
  void ShowStatus(const QString& text);

 protected:
  void closeEvent(QCloseEvent* event) override;

 private:
  bool InstallSystemModalLoopBridge();
  void RemoveSystemModalLoopBridge();

  BrowserWidget* browser_ = nullptr;
  QLineEdit* address_ = nullptr;
  QAction* back_ = nullptr;
  QAction* forward_ = nullptr;
  QAction* reload_ = nullptr;
  std::unique_ptr<SystemModalLoopBridge> system_modal_loop_bridge_;
  bool is_loading_ = false;
  bool shutdown_requested_ = false;
};

}  // namespace litheview_qt_demo

#endif  // LITHEVIEW_EXAMPLES_LITHEVIEW_QT_DEMO_BROWSER_WINDOW_H_
