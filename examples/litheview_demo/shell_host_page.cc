#include "shell_host.h"

namespace litheview_demo {

ltv_view_client_t ShellHost::MakeViewClient() {
  ltv_view_client_t client = {};
  client.struct_size = sizeof(client);
  client.version = LTV_STRUCT_VERSION;
  client.user_data = this;
  client.on_url_changed = &ShellHost::OnUrlChanged;
  client.on_title_changed = &ShellHost::OnTitleChanged;
  client.on_load_error = &ShellHost::OnLoadError;
  client.on_closed = &ShellHost::OnClosed;
  client.on_renderer_terminated = &ShellHost::OnRendererTerminated;
  client.on_close_requested = &ShellHost::OnCloseRequested;
  client.on_before_popup = &ShellHost::OnBeforePopup;
  client.on_popup_created = &ShellHost::OnPopupCreated;
  client.on_console_message = &ShellHost::OnConsoleMessage;
  client.on_javascript_dialog = &ShellHost::OnJavaScriptDialog;
  client.on_file_dialog = &ShellHost::OnFileDialog;
  client.on_before_download = &ShellHost::OnBeforeDownload;
  client.on_download_updated = &ShellHost::OnDownloadUpdated;
  client.on_javascript_message = &ShellHost::OnJavaScriptMessage;
  client.on_navigation_state_changed = &ShellHost::OnNavigationStateChanged;
  client.on_gpu_frame = &ShellHost::OnGpuFrame;
  return client;
}

bool ShellHost::InitializeView() {
  if (!view_) {
    view_ = ltv_view_create();
  }
  const ltv_view_client_t client = MakeViewClient();
  if (!view_ || ltv_view_set_client(view_, &client) != LTV_OK) {
    DestroyView();
    return false;
  }
  current_url_ = initial_url_;
  return AttachView();
}

bool ShellHost::LoadDemoPage(DemoPage page) {
  if (!view_ || page == DemoPage::kExternal) {
    return false;
  }
  const DemoPageContent& content = GetDemoPageContent(page);
  const DemoPage previous_page = current_page_;
  gpu_output_pending_ = false;
  if (!ConfigureOutputForPage(DemoPage::kExternal)) {
    return false;
  }

  current_page_ = page;
  current_url_ = content.url;
  gpu_output_pending_ = content.uses_gpu_output;
  const ltv_error_t result = ltv_view_load_html(view_, content.html, content.url);
  if (result != LTV_OK) {
    gpu_output_pending_ = false;
    current_page_ = previous_page;
    ConfigureOutputForPage(previous_page);
    AppendLog("[page error] built-in page did not load (" +
              std::to_string(result) + ")");
    return false;
  }
  if (address_) {
    SetWindowTextW(address_, Utf8ToWide(current_url_).c_str());
  }
  UpdateDemoPageMenu();
  ltv_view_set_focus(view_, 1);
  InvalidateRect(render_window_, nullptr, TRUE);
  return true;
}

bool ShellHost::ConfigureOutputForPage(DemoPage page) {
  const bool use_gpu_output = GetDemoPageContent(page).uses_gpu_output;
  if (use_gpu_output) {
    if (gpu_output_started_) {
      return true;
    }
    if (view_attached_) {
      if (ltv_win_view_detach(view_) != LTV_OK) {
        return false;
      }
      view_attached_ = false;
    }
    ResizeView();
    if (StartGpuOutput()) {
      return true;
    }
    AttachView();
    AppendLog("[gpu output error] shared texture stream did not start");
    return false;
  }

  StopGpuOutput();
  return AttachView();
}

bool ShellHost::AttachView() {
  if (view_attached_) {
    return true;
  }
  if (!view_ || !render_window_ ||
      ltv_win_view_attach(view_, reinterpret_cast<uintptr_t>(render_window_)) !=
          LTV_OK) {
    return false;
  }
  view_attached_ = true;
  view_size_dips_ = {};
  surface_size_pixels_ = {};
  ResizeView();
  return true;
}

bool ShellHost::StartGpuOutput() {
  if (gpu_output_started_) {
    return true;
  }
  const ltv_gpu_output_settings_t settings = {
      sizeof(settings), LTV_STRUCT_VERSION, LTV_GPU_DELIVERY_GPU_COPY, 60};
  gpu_output_started_ =
      ltv_view_start_gpu_output(view_, &settings) == LTV_OK;
  if (gpu_output_started_ && gpu_output_window_) {
    ShowWindow(gpu_output_window_, SW_SHOW);
    BringWindowToTop(gpu_output_window_);
  }
  return gpu_output_started_;
}

void ShellHost::SuspendGpuOutputForResize() {
  if (!gpu_output_started_ || !view_) {
    return;
  }
  if (ltv_view_stop_gpu_output(view_) == LTV_OK) {
    gpu_output_started_ = false;
    gpu_output_suspended_for_resize_ = true;
  }
}

void ShellHost::ResumeGpuOutputAfterResize() {
  if (!gpu_output_suspended_for_resize_) {
    return;
  }
  gpu_output_suspended_for_resize_ = false;
  if (GetDemoPageContent(current_page_).uses_gpu_output) {
    ResizeView();
    if (!StartGpuOutput()) {
      SetWindowTextW(window_, kNavigationFailedTitle);
    }
  }
}

void ShellHost::StopGpuOutput() {
  gpu_output_suspended_for_resize_ = false;
  if (gpu_output_started_ && view_) {
    ltv_view_stop_gpu_output(view_);
    gpu_output_started_ = false;
  }
  if (presenter_) {
    presenter_->Reset();
  }
  if (gpu_output_window_) {
    ShowWindow(gpu_output_window_, SW_HIDE);
  }
  if (render_window_) {
    InvalidateRect(render_window_, nullptr, FALSE);
  }
}

void ShellHost::DestroyView() {
  gpu_output_pending_ = false;

  RevokeFileDropTargets();
  StopGpuOutput();
  if (!view_) {
    return;
  }
  if (view_attached_) {
    ltv_win_view_detach(view_);
    view_attached_ = false;
  }
  ltv_view_set_client(view_, nullptr);
  ltv_view_destroy(view_);
  view_ = nullptr;
}

}  // namespace litheview_demo
