#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>
#include <vector>

#include "native_dialogs.h"
#include "shell_app.h"
#include "shell_host.h"

namespace litheview_demo {

void LTV_CALLBACK ShellHost::OnUrlChanged(ltv_view_t view,
                                          const char* url,
                                          void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (!self || view != self->view_) {
    return;
  }
  const std::string_view changed_url = url ? url : "";
  if (self->current_page_ != DemoPage::kExternal &&
      (changed_url == GetDemoPageContent(self->current_page_).url ||
       changed_url.starts_with("data:text/html"))) {
    self->current_url_ = GetDemoPageContent(self->current_page_).url;
  } else {
    self->current_page_ = DemoPage::kExternal;
    self->current_url_ = changed_url;
    self->UpdateDemoPageMenu();
  }
  if (self->address_) {
    SetWindowTextW(self->address_, Utf8ToWide(self->current_url_).c_str());
  }
}

ltv_console_decision_t LTV_CALLBACK
ShellHost::OnConsoleMessage(ltv_view_t,
                            const ltv_console_message_t* message,
                            void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (self && message) {
    self->AppendLog("[console] " +
                    std::string(message->message ? message->message : ""));
  }
  return LTV_CONSOLE_HANDLED;
}

void LTV_CALLBACK
ShellHost::OnJavaScriptMessage(ltv_view_t view,
                               const ltv_javascript_message_t* message,
                               void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (!self || !message || !message->json_value) {
    return;
  }
  self->AppendLog("[page -> host] " + std::string(message->json_value));
  constexpr std::string_view kExecuteCommand =
      R"JSON({"type":"execute-custom-javascript"})JSON";
  if (message->json_value == kExecuteCommand) {
    constexpr char kExecuteScript[] =
        "Promise.resolve().then(() => "
        "(0, eval)(window.__litheviewCustomScript)).then("
        "value => ({ok:true,value}), "
        "error => ({ok:false,error:String(error),stack:error?.stack||''}))";
    if (ltv_view_execute_javascript(view, kExecuteScript,
                                    &OnCustomJavaScriptResult,
                                    self) != LTV_OK) {
      ltv_view_post_javascript_message(
          view,
          R"JSON({"type":"javascript-result","result":{"ok":false,"error":"execution did not start"}})JSON");
    }
    return;
  }
  const std::string reply = "{\"type\":\"host-ack\",\"received\":" +
                            std::string(message->json_value) + "}";
  if (ltv_view_post_javascript_message(view, reply.c_str()) == LTV_OK) {
    self->AppendLog("[host -> page] " + reply);
  }
}

void LTV_CALLBACK ShellHost::OnCustomJavaScriptResult(
    ltv_view_t view,
    const ltv_javascript_result_t* result,
    void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (!self || view != self->view_ || !result) {
    return;
  }
  std::string reply = R"JSON({"type":"javascript-result","result":)JSON";
  if (result->code == LTV_OK && result->json_result) {
    reply.append(result->json_result);
  } else {
    reply.append(
        R"JSON({"ok":false,"error":"C++ could not execute the script"})JSON");
  }
  reply.push_back('}');
  ltv_view_post_javascript_message(view, reply.c_str());
}

void LTV_CALLBACK
ShellHost::OnNavigationStateChanged(ltv_view_t view,
                                    const ltv_navigation_state_t* state,
                                    void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (!self || !state || view != self->view_) {
    return;
  }
  self->is_loading_ = state->is_loading != 0;
  if (self->back_button_) {
    EnableWindow(self->back_button_, state->can_go_back ? TRUE : FALSE);
  }
  if (self->forward_button_) {
    EnableWindow(self->forward_button_, state->can_go_forward ? TRUE : FALSE);
  }
  if (self->reload_button_) {
    SetWindowTextW(self->reload_button_,
                   state->is_loading ? L"\x00D7" : L"\x21BB");
  }
  if (!state->is_loading && self->gpu_output_pending_) {
    self->gpu_output_pending_ = false;
    if (!self->ConfigureOutputForPage(self->current_page_)) {
      SetWindowTextW(self->window_, kNavigationFailedTitle);
    }
  }
}

void LTV_CALLBACK ShellHost::OnTitleChanged(ltv_view_t view,
                                            const char* title,
                                            void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (!self || !self->window_ || view != self->view_) {
    return;
  }
  std::wstring window_title = kWindowTitle;
  if (title && *title) {
    window_title.append(L" - ");
    window_title.append(Utf8ToWide(title));
  }
  SetWindowTextW(self->window_, window_title.c_str());
}

void LTV_CALLBACK ShellHost::OnLoadError(ltv_view_t view,
                                         const char*,
                                         int32_t,
                                         const char*,
                                         void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (self && self->window_ && view == self->view_) {
    SetWindowText(self->window_, kNavigationFailedTitle);
  }
}

void LTV_CALLBACK ShellHost::OnClosed(ltv_view_t view, void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (self && self->window_ && view == self->view_) {
    PostMessage(self->window_, kViewClosedMessage, 0, 0);
  }
}

void LTV_CALLBACK
ShellHost::OnRendererTerminated(ltv_view_t view,
                                ltv_renderer_termination_status_t,
                                void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (self && self->window_ && view == self->view_) {
    SetWindowText(self->window_, kRendererTerminatedTitle);
  }
}

ltv_close_decision_t LTV_CALLBACK ShellHost::OnCloseRequested(ltv_view_t,
                                                              void*) {
  return LTV_CLOSE_ALLOW;
}

ltv_popup_decision_t LTV_CALLBACK
ShellHost::OnBeforePopup(ltv_view_t, const ltv_popup_request_t*, void*) {
  return LTV_POPUP_ALLOW;
}

void LTV_CALLBACK ShellHost::OnPopupCreated(ltv_view_t,
                                            ltv_view_t popup,
                                            const ltv_popup_request_t* request,
                                            void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (self) {
    self->app_->OpenPopup(popup, request);
  }
}

ltv_dialog_handling_t LTV_CALLBACK
ShellHost::OnJavaScriptDialog(ltv_view_t view,
                              const ltv_javascript_dialog_request_t* request,
                              void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (!self || !self->window_ || !request || request->dialog_id == 0) {
    return LTV_DIALOG_UNHANDLED;
  }

  const std::wstring page_message =
      Utf8ToWide(request->message ? request->message : "");
  bool accept = false;
  std::string prompt_value;
  const char* prompt_override = nullptr;
  switch (request->type) {
    case LTV_JAVASCRIPT_DIALOG_ALERT:
      RunNativeMessageBox(self->window_, page_message,
                          MB_OK | MB_ICONINFORMATION);
      accept = true;
      break;
    case LTV_JAVASCRIPT_DIALOG_CONFIRM:
      accept = RunNativeMessageBox(self->window_, page_message,
                                   MB_OKCANCEL | MB_ICONQUESTION) == IDOK;
      break;
    case LTV_JAVASCRIPT_DIALOG_PROMPT: {
      std::wstring value;
      accept = RunNativePrompt(
          self->window_, page_message,
          Utf8ToWide(request->default_prompt ? request->default_prompt : ""),
          &value);
      if (accept) {
        prompt_value = WideToUtf8(value);
        prompt_override = prompt_value.c_str();
      }
      break;
    }
    case LTV_JAVASCRIPT_DIALOG_BEFORE_UNLOAD: {
      const std::wstring warning =
          page_message.empty()
              ? (request->is_reload
                     ? L"Reload this page? Changes you made may not be saved."
                     : L"Leave this page? Changes you made may not be saved.")
              : page_message;
      accept = RunNativeMessageBox(self->window_, warning,
                                   MB_OKCANCEL | MB_ICONWARNING) == IDOK;
      break;
    }
    default:
      return LTV_DIALOG_UNHANDLED;
  }

  return ltv_view_reply_to_javascript_dialog(view, request->dialog_id,
                                             accept ? 1 : 0,
                                             prompt_override) == LTV_OK
             ? LTV_DIALOG_HANDLED
             : LTV_DIALOG_UNHANDLED;
}

ltv_file_dialog_handling_t LTV_CALLBACK
ShellHost::OnFileDialog(ltv_view_t view,
                        const ltv_file_dialog_request_t* request,
                        void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (!self || !self->window_ || !request || request->dialog_id == 0) {
    return LTV_FILE_DIALOG_UNHANDLED;
  }

  std::vector<std::wstring> selected_paths;
  switch (request->mode) {
    case LTV_FILE_DIALOG_OPEN:
    case LTV_FILE_DIALOG_OPEN_MULTIPLE:
    case LTV_FILE_DIALOG_SAVE:
      selected_paths = RunNativeFilePicker(self->window_, *request);
      break;
    case LTV_FILE_DIALOG_UPLOAD_FOLDER:
    case LTV_FILE_DIALOG_OPEN_DIRECTORY:
      selected_paths = RunNativeFolderPicker(self->window_, *request);
      break;
    default:
      return LTV_FILE_DIALOG_UNHANDLED;
  }

  std::vector<std::string> selected_paths_utf8;
  selected_paths_utf8.reserve(selected_paths.size());
  for (const std::wstring& selected_path : selected_paths) {
    selected_paths_utf8.push_back(WideToUtf8(selected_path));
  }
  std::vector<const char*> selected_path_pointers;
  selected_path_pointers.reserve(selected_paths_utf8.size());
  for (const std::string& selected_path : selected_paths_utf8) {
    selected_path_pointers.push_back(selected_path.c_str());
  }
  return ltv_view_reply_to_file_dialog(
             view, request->dialog_id, selected_path_pointers.data(),
             static_cast<uint32_t>(selected_path_pointers.size())) == LTV_OK
             ? LTV_FILE_DIALOG_HANDLED
             : LTV_FILE_DIALOG_UNHANDLED;
}

ltv_download_handling_t LTV_CALLBACK
ShellHost::OnBeforeDownload(ltv_view_t view,
                            const ltv_download_request_t* request,
                            void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (!self || !self->window_ || !request || request->download_id == 0) {
    return LTV_DOWNLOAD_UNHANDLED;
  }
  const std::wstring selected_path =
      RunNativeDownloadPicker(self->window_, *request);
  const std::string selected_path_utf8 = WideToUtf8(selected_path);
  return ltv_view_reply_to_download(view, request->download_id,
                                    selected_path_utf8.empty()
                                        ? nullptr
                                        : selected_path_utf8.c_str()) == LTV_OK
             ? LTV_DOWNLOAD_HANDLED
             : LTV_DOWNLOAD_UNHANDLED;
}

void LTV_CALLBACK
ShellHost::OnDownloadUpdated(ltv_view_t,
                             const ltv_download_update_t* update,
                             void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (!self || !self->window_ || !update) {
    return;
  }
  if (update->state == LTV_DOWNLOAD_STATE_COMPLETE) {
    std::wstring title = L"LitheView Demo - downloaded";
    if (update->target_path && *update->target_path) {
      title.append(L": ");
      title.append(Utf8ToWide(update->target_path));
    }
    SetWindowTextW(self->window_, title.c_str());
  } else if (update->state == LTV_DOWNLOAD_STATE_INTERRUPTED) {
    SetWindowTextW(self->window_, L"LitheView Demo - download failed");
  }
}

void LTV_CALLBACK ShellHost::OnGpuFrame(ltv_view_t view,
                                        const ltv_gpu_frame_t* frame,
                                        void* user_data) {
  auto* self = static_cast<ShellHost*>(user_data);
  if (!frame) {
    return;
  }
  if (self && view == self->view_ && self->gpu_output_started_ &&
      self->presenter_) {
    self->presenter_->Present(*frame);
  }
  ltv_view_release_gpu_frame(view, frame->frame_token);
}

}  // namespace litheview_demo
