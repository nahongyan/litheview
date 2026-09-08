#include "browser_widget.h"

#include <windows.h>

#include <litheview/litheview_win.h>

#include <QEvent>
#include <QFocusEvent>
#include <QResizeEvent>
#include <QUrl>
#include <memory>

#include "browser_window.h"
#include "demo_config.h"
#include "native_browser_surface.h"

namespace litheview_qt_demo {

BrowserWidget::BrowserWidget(BrowserWindow* owner)
    : QWidget(owner), owner_(owner) {
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_InputMethodEnabled);
  setFocusPolicy(Qt::StrongFocus);
}

BrowserWidget::~BrowserWidget() {
  Shutdown();
}

bool BrowserWidget::Initialize() {
  ltv_view_client_t client = {};
  client.struct_size = sizeof(client);
  client.version = LTV_STRUCT_VERSION;
  client.user_data = this;
  client.on_url_changed = &OnUrlChanged;
  client.on_title_changed = &OnTitleChanged;
  client.on_load_error = &OnLoadError;
  client.on_closed = &OnClosed;
  client.on_renderer_terminated = &OnRendererTerminated;
  client.on_close_requested = &OnCloseRequested;
  client.on_navigation_state_changed = &OnNavigationStateChanged;

  view_ = ltv_view_create_with_client(&client);
  if (!view_) {
    return false;
  }

  surface_ = std::make_unique<NativeBrowserSurface>();
  if (!surface_->Attach(view_, this)) {
    Shutdown();
    return false;
  }

  const QByteArray override_url = qgetenv("LITHEVIEW_QT_DEMO_URL");
  const char* initial_url =
      override_url.isEmpty() ? kInitialUrl : override_url.constData();
  if (ltv_view_load_url(view_, initial_url) != LTV_OK) {
    Shutdown();
    return false;
  }
  ltv_view_set_focus(view_, 1);
  return true;
}

void BrowserWidget::Shutdown() {
  if (!view_) {
    return;
  }
  surface_.reset();
  ltv_view_set_client(view_, nullptr);
  ltv_view_destroy(view_);
  view_ = nullptr;
}

void BrowserWidget::Navigate(const QString& input) {
  if (!view_) {
    return;
  }
  const QUrl url = QUrl::fromUserInput(input);
  if (url.isValid()) {
    const QByteArray encoded = url.toEncoded();
    ltv_view_navigate(view_, encoded.constData());
  }
}

void BrowserWidget::GoBack() {
  if (view_) {
    ltv_view_go_back(view_);
  }
}

void BrowserWidget::GoForward() {
  if (view_) {
    ltv_view_go_forward(view_);
  }
}

void BrowserWidget::Reload() {
  if (view_) {
    ltv_view_reload(view_);
  }
}

void BrowserWidget::Stop() {
  if (view_) {
    ltv_view_stop(view_);
  }
}

void BrowserWidget::RefreshNativeSurface() {
  if (surface_) {
    surface_->Refresh();
  }
}

void BrowserWidget::FinishNativeResize() {
  if (surface_) {
    surface_->FinishResize();
  }
}

uintptr_t BrowserWidget::NativeSurfaceWindow() const {
  return surface_ ? surface_->window() : 0;
}

void BrowserWidget::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  if (surface_) {
    surface_->ResizeToHost();
  }
}

void BrowserWidget::focusInEvent(QFocusEvent* event) {
  QWidget::focusInEvent(event);
  if (surface_) {
    surface_->Focus();
  }
}

bool BrowserWidget::event(QEvent* event) {
  if (event->type() == QEvent::DevicePixelRatioChange && view_) {
    ltv_win_set_device_scale_factor(static_cast<float>(devicePixelRatioF()));
    if (surface_) {
      surface_->ResizeToHost();
      surface_->Refresh();
    }
  }
  return QWidget::event(event);
}

void LTV_CALLBACK BrowserWidget::OnUrlChanged(ltv_view_t,
                                              const char* url,
                                              void* user_data) {
  auto* self = static_cast<BrowserWidget*>(user_data);
  self->owner_->SetPageUrl(QString::fromUtf8(url ? url : ""));
}

void LTV_CALLBACK BrowserWidget::OnTitleChanged(ltv_view_t,
                                                const char* title,
                                                void* user_data) {
  auto* self = static_cast<BrowserWidget*>(user_data);
  self->owner_->SetPageTitle(QString::fromUtf8(title ? title : ""));
}

void LTV_CALLBACK BrowserWidget::OnLoadError(ltv_view_t,
                                             const char* url,
                                             int32_t error_code,
                                             const char* error_text,
                                             void* user_data) {
  auto* self = static_cast<BrowserWidget*>(user_data);
  self->owner_->ShowStatus(
      QStringLiteral("Load failed (%1): %2 - %3")
          .arg(error_code)
          .arg(QString::fromUtf8(url ? url : ""))
          .arg(QString::fromUtf8(error_text ? error_text : "")));
}

void LTV_CALLBACK BrowserWidget::OnClosed(ltv_view_t, void* user_data) {
  auto* self = static_cast<BrowserWidget*>(user_data);
  PostMessageW(reinterpret_cast<HWND>(self->owner_->winId()), WM_CLOSE, 0, 0);
}

void LTV_CALLBACK
BrowserWidget::OnRendererTerminated(ltv_view_t,
                                    ltv_renderer_termination_status_t status,
                                    void* user_data) {
  auto* self = static_cast<BrowserWidget*>(user_data);
  self->owner_->ShowStatus(QStringLiteral("Renderer terminated with status %1")
                               .arg(static_cast<int>(status)));
}

ltv_close_decision_t LTV_CALLBACK BrowserWidget::OnCloseRequested(ltv_view_t,
                                                                  void*) {
  return LTV_CLOSE_ALLOW;
}

void LTV_CALLBACK
BrowserWidget::OnNavigationStateChanged(ltv_view_t,
                                        const ltv_navigation_state_t* state,
                                        void* user_data) {
  if (!state) {
    return;
  }
  auto* self = static_cast<BrowserWidget*>(user_data);
  self->owner_->SetNavigationState(state->can_go_back != 0,
                                   state->can_go_forward != 0,
                                   state->is_loading != 0);
}

}  // namespace litheview_qt_demo
