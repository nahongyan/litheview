#include "browser_window.h"

#include <windows.h>

#include <commctrl.h>
#include <dwmapi.h>
#include <litheview/litheview.h>
#include <litheview/litheview_win.h>

#include <QAction>
#include <QCloseEvent>
#include <QLayout>
#include <QLineEdit>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <cstdint>
#include <memory>

#include "browser_widget.h"
#include "demo_config.h"

namespace litheview_qt_demo {
namespace {

constexpr UINT_PTR kSystemModalLoopSubclassId = 1;
constexpr wchar_t kInteractionVisualClassName[] =
    L"LitheViewQtInteractionVisual";
constexpr int kInteractionVisualHandoffMilliseconds = 160;
constexpr int kInteractionVisualFinalFrameMilliseconds = 64;

class NativeInteractionVisual {
 public:
  static std::unique_ptr<NativeInteractionVisual> Create(HWND owner,
                                                         HWND browser) {
    auto visual = std::unique_ptr<NativeInteractionVisual>(
        new NativeInteractionVisual(owner, browser));
    if (!visual->Capture() || !visual->CreateNativeWindow()) {
      return nullptr;
    }
    return visual;
  }

  NativeInteractionVisual(const NativeInteractionVisual&) = delete;
  NativeInteractionVisual& operator=(const NativeInteractionVisual&) = delete;

  ~NativeInteractionVisual() {
    if (interaction_window_) {
      DestroyWindow(interaction_window_);
    }
    if (handoff_window_) {
      DestroyWindow(handoff_window_);
    }
    if (bitmap_) {
      DeleteObject(bitmap_);
    }
  }

  void SyncToBrowser() {
    if (!interaction_window_ && !handoff_window_) {
      return;
    }

    RECT bounds;
    if (!GetWindowRect(browser_, &bounds)) {
      return;
    }
    const int width = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    if (width <= 0 || height <= 0) {
      return;
    }

    if (interaction_window_) {
      POINT origin = {bounds.left, bounds.top};
      if (ScreenToClient(owner_, &origin)) {
        SyncWindow(interaction_window_, origin, width, height);
      }
    }
    SyncWindow(handoff_window_, {bounds.left, bounds.top}, width, height);
  }

  bool BeginHandoff() {
    if (handoff_window_) {
      SyncToBrowser();
      RedrawWindow(handoff_window_, nullptr, nullptr,
                   RDW_INVALIDATE | RDW_UPDATENOW);
      DwmFlush();
      return true;
    }
    handoff_window_ = CreateVisualWindow(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        true);
    if (!handoff_window_) {
      return false;
    }
    SetLayeredWindowAttributes(handoff_window_, 0, 254, LWA_ALPHA);
    SyncToBrowser();
    RedrawWindow(handoff_window_, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_UPDATENOW);
    DwmFlush();
    if (interaction_window_) {
      DestroyWindow(interaction_window_);
      interaction_window_ = nullptr;
    }
    return true;
  }

 private:
  NativeInteractionVisual(HWND owner, HWND browser)
      : owner_(owner), browser_(browser) {}

  bool Capture() {
    RECT bounds;
    if (!owner_ || !browser_ || !GetWindowRect(browser_, &bounds)) {
      return false;
    }
    const int width = bounds.right - bounds.left;
    const int height = bounds.bottom - bounds.top;
    if (width <= 0 || height <= 0) {
      return false;
    }

    HDC screen = GetDC(nullptr);
    HDC memory = screen ? CreateCompatibleDC(screen) : nullptr;
    HBITMAP bitmap =
        screen ? CreateCompatibleBitmap(screen, width, height) : nullptr;
    if (!screen || !memory || !bitmap) {
      if (memory) {
        DeleteDC(memory);
      }
      if (screen) {
        ReleaseDC(nullptr, screen);
      }
      return false;
    }
    HGDIOBJ previous = SelectObject(memory, bitmap);
    const bool copied = BitBlt(memory, 0, 0, width, height, screen, bounds.left,
                               bounds.top, SRCCOPY) != FALSE;
    SelectObject(memory, previous);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);
    if (!copied) {
      DeleteObject(bitmap);
      return false;
    }
    if (bitmap_) {
      DeleteObject(bitmap_);
    }
    bitmap_ = bitmap;
    bitmap_width_ = width;
    bitmap_height_ = height;
    return true;
  }

  static ATOM RegisterWindowClass() {
    WNDCLASSEXW window_class = {};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = &WindowProc;
    window_class.hInstance = GetModuleHandleW(nullptr);
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.lpszClassName = kInteractionVisualClassName;
    ATOM atom = RegisterClassExW(&window_class);
    if (!atom && GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
      return 1;
    }
    return atom;
  }

  bool CreateNativeWindow() {
    if (!RegisterWindowClass()) {
      return false;
    }
    interaction_window_ = CreateVisualWindow(0, false);
    if (!interaction_window_) {
      return false;
    }
    SyncToBrowser();
    RedrawWindow(interaction_window_, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    return true;
  }

  HWND CreateVisualWindow(DWORD extended_style, bool popup) {
    return CreateWindowExW(extended_style, kInteractionVisualClassName, L"",
                           popup ? WS_POPUP : WS_CHILD, 0, 0, 1, 1, owner_,
                           nullptr, GetModuleHandleW(nullptr), this);
  }

  void SyncWindow(HWND window, const POINT& origin, int width, int height) {
    if (!window) {
      return;
    }
    SetWindowPos(window, HWND_TOP, origin.x, origin.y, width, height,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(window, nullptr, FALSE);
  }

  void Paint(HWND window) {
    PAINTSTRUCT paint = {};
    HDC target = BeginPaint(window, &paint);
    RECT bounds;
    GetClientRect(window, &bounds);
    HDC source = CreateCompatibleDC(target);
    HGDIOBJ previous = SelectObject(source, bitmap_);
    SetStretchBltMode(target, COLORONCOLOR);
    StretchBlt(target, 0, 0, bounds.right, bounds.bottom, source, 0, 0,
               bitmap_width_, bitmap_height_, SRCCOPY);
    SelectObject(source, previous);
    DeleteDC(source);
    EndPaint(window, &paint);
  }

  static LRESULT CALLBACK WindowProc(HWND window,
                                     UINT message,
                                     WPARAM wparam,
                                     LPARAM lparam) {
    auto* self = reinterpret_cast<NativeInteractionVisual*>(
        GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
      auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
      self = static_cast<NativeInteractionVisual*>(create->lpCreateParams);
      SetWindowLongPtrW(window, GWLP_USERDATA,
                        reinterpret_cast<LONG_PTR>(self));
    }
    if (self && message == WM_PAINT) {
      self->Paint(window);
      return 0;
    }
    if (message == WM_ERASEBKGND) {
      return 1;
    }
    if (message == WM_NCHITTEST) {
      return HTTRANSPARENT;
    }
    if (message == WM_NCDESTROY && self) {
      SetWindowLongPtrW(window, GWLP_USERDATA, 0);
    }
    return DefWindowProcW(window, message, wparam, lparam);
  }

  HWND owner_ = nullptr;
  HWND browser_ = nullptr;
  HWND interaction_window_ = nullptr;
  HWND handoff_window_ = nullptr;
  HBITMAP bitmap_ = nullptr;
  int bitmap_width_ = 0;
  int bitmap_height_ = 0;
};

}  // namespace

class SystemModalLoopBridge {
 public:
  SystemModalLoopBridge(BrowserWindow* owner, BrowserWidget* browser)
      : owner_(owner), browser_(browser) {}

  ~SystemModalLoopBridge() { Remove(); }

  bool Install() {
    if (installed_) {
      return true;
    }
    window_ = reinterpret_cast<HWND>(owner_->winId());
    installed_ =
        SetWindowSubclass(window_, &SubclassProc, kSystemModalLoopSubclassId,
                          reinterpret_cast<DWORD_PTR>(this)) != FALSE;
    return installed_;
  }

  void Remove() {
    ++visual_generation_;
    visual_.reset();
    if (installed_) {
      RemoveWindowSubclass(window_, &SubclassProc, kSystemModalLoopSubclassId);
      installed_ = false;
    }
    window_ = nullptr;
  }

 private:
  void BeginInteractiveSystemLoop(UINT command) {
    ++visual_generation_;
    resizing_ = command == SC_SIZE;
    visual_ = resizing_ ? nullptr
                        : NativeInteractionVisual::Create(
                              window_, reinterpret_cast<HWND>(
                                           browser_->NativeSurfaceWindow()));
  }

  void SyncVisual() {
    if (visual_) {
      visual_->SyncToBrowser();
    }
  }

  void EndInteractiveSystemLoop() {
    if (resizing_) {
      resizing_ = false;
      browser_->RefreshNativeSurface();
      DwmFlush();
      return;
    }
    if (!visual_) {
      return;
    }
    SyncVisual();
    visual_->BeginHandoff();
    browser_->RefreshNativeSurface();
    DwmFlush();
    const uint64_t generation = visual_generation_;
    QTimer::singleShot(
        kInteractionVisualHandoffMilliseconds, owner_, [this, generation] {
          if (generation != visual_generation_) {
            return;
          }
          browser_->RefreshNativeSurface();
          DwmFlush();
          QTimer::singleShot(kInteractionVisualFinalFrameMilliseconds, owner_,
                             [this, generation] {
                               if (generation != visual_generation_) {
                                 return;
                               }
                               DwmFlush();
                               visual_.reset();
                               browser_->RefreshNativeSurface();
                               DwmFlush();
                             });
        });
  }

  static LRESULT CALLBACK SubclassProc(HWND window,
                                       UINT message,
                                       WPARAM wparam,
                                       LPARAM lparam,
                                       UINT_PTR,
                                       DWORD_PTR ref_data) {
    auto* self = reinterpret_cast<SystemModalLoopBridge*>(ref_data);
    if (message == WM_SYSCOMMAND) {
      const UINT command = static_cast<UINT>(wparam & 0xfff0);
      const bool interactive = command == SC_MOVE || command == SC_SIZE;
      if (interactive) {
        self->BeginInteractiveSystemLoop(command);
      }
      const bool entered = ltv_win_set_os_modal_loop(1) == LTV_OK;
      const LRESULT result = DefSubclassProc(window, message, wparam, lparam);
      if (entered) {
        ltv_win_set_os_modal_loop(0);
      }
      if (interactive) {
        self->EndInteractiveSystemLoop();
      }
      return result;
    }

    if (message == WM_EXITSIZEMOVE && self->resizing_) {
      self->browser_->FinishNativeResize();
    }
    const LRESULT result = DefSubclassProc(window, message, wparam, lparam);
    if (message == WM_SIZE || message == WM_WINDOWPOSCHANGED) {
      if (message == WM_SIZE && self->resizing_ && self->owner_->layout()) {
        self->owner_->layout()->activate();
      }
      self->SyncVisual();
    }
    return result;
  }

  BrowserWindow* owner_ = nullptr;
  BrowserWidget* browser_ = nullptr;
  HWND window_ = nullptr;
  std::unique_ptr<NativeInteractionVisual> visual_;
  uint64_t visual_generation_ = 0;
  bool installed_ = false;
  bool resizing_ = false;
};

BrowserWindow::BrowserWindow() {
  setWindowTitle(QStringLiteral("LitheView Qt SDK Demo"));
  resize(kInitialWidth, kInitialHeight);

  auto* toolbar = addToolBar(QStringLiteral("Navigation"));
  toolbar->setMovable(false);
  back_ = toolbar->addAction(QStringLiteral("Back"));
  forward_ = toolbar->addAction(QStringLiteral("Forward"));
  reload_ = toolbar->addAction(QStringLiteral("Reload"));
  address_ = new QLineEdit(QString::fromLatin1(kInitialUrl), toolbar);
  address_->setClearButtonEnabled(true);
  toolbar->addWidget(address_);

  browser_ = new BrowserWidget(this);
  setCentralWidget(browser_);
  statusBar()->showMessage(QStringLiteral("Initializing LitheView..."));

  connect(back_, &QAction::triggered, browser_, &BrowserWidget::GoBack);
  connect(forward_, &QAction::triggered, browser_, &BrowserWidget::GoForward);
  connect(reload_, &QAction::triggered, this, [this] {
    if (is_loading_) {
      browser_->Stop();
    } else {
      browser_->Reload();
    }
  });
  connect(address_, &QLineEdit::returnPressed, this,
          [this] { browser_->Navigate(address_->text()); });
  SetNavigationState(false, false, false);
}

BrowserWindow::~BrowserWindow() {
  RemoveSystemModalLoopBridge();
}

bool BrowserWindow::Initialize() {
  const bool initialized = browser_->Initialize();
  if (!initialized || !InstallSystemModalLoopBridge()) {
    if (initialized) {
      browser_->Shutdown();
    }
    statusBar()->showMessage(
        initialized
            ? QStringLiteral("Could not install the native modal-loop bridge")
            : QStringLiteral("Could not initialize LitheView"));
    return false;
  }
  statusBar()->showMessage(
      initialized ? QStringLiteral("Ready")
                  : QStringLiteral("Could not initialize LitheView"));
  return initialized;
}

bool BrowserWindow::InstallSystemModalLoopBridge() {
  if (!system_modal_loop_bridge_) {
    system_modal_loop_bridge_ =
        std::make_unique<SystemModalLoopBridge>(this, browser_);
  }
  return system_modal_loop_bridge_->Install();
}

void BrowserWindow::RemoveSystemModalLoopBridge() {
  if (system_modal_loop_bridge_) {
    system_modal_loop_bridge_->Remove();
    system_modal_loop_bridge_.reset();
  }
}

void BrowserWindow::SetPageUrl(const QString& url) {
  address_->setText(url);
}

void BrowserWindow::SetPageTitle(const QString& title) {
  setWindowTitle(title.isEmpty()
                     ? QStringLiteral("LitheView Qt SDK Demo")
                     : QStringLiteral("%1 - LitheView Qt SDK Demo").arg(title));
}

void BrowserWindow::SetNavigationState(bool can_go_back,
                                       bool can_go_forward,
                                       bool is_loading) {
  is_loading_ = is_loading;
  back_->setEnabled(can_go_back);
  forward_->setEnabled(can_go_forward);
  reload_->setText(is_loading ? QStringLiteral("Stop")
                              : QStringLiteral("Reload"));
}

void BrowserWindow::ShowStatus(const QString& text) {
  statusBar()->showMessage(text);
}

void BrowserWindow::closeEvent(QCloseEvent* event) {
  if (!shutdown_requested_) {
    shutdown_requested_ = true;
    browser_->Shutdown();
  }
  QMainWindow::closeEvent(event);
}

}  // namespace litheview_qt_demo
