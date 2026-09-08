#ifndef LITHEVIEW_PUBLIC_LITHEVIEW_HPP_
#define LITHEVIEW_PUBLIC_LITHEVIEW_HPP_

#include <functional>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "litheview.h"

namespace litheview {

class EvalResult {
 public:
  EvalResult(const EvalResult&) = delete;
  EvalResult& operator=(const EvalResult&) = delete;

  EvalResult(EvalResult&& other) noexcept : result_(other.result_) {
    other.result_.json_result = nullptr;
    other.result_.error = nullptr;
  }

  EvalResult& operator=(EvalResult&& other) noexcept {
    if (this != &other) {
      Release();
      result_ = other.result_;
      other.result_.json_result = nullptr;
      other.result_.error = nullptr;
    }
    return *this;
  }

  ~EvalResult() { Release(); }

  ltv_error_t code() const noexcept { return result_.code; }
  bool ok() const noexcept { return result_.code == LTV_OK; }
  std::string_view json() const noexcept {
    return result_.json_result ? result_.json_result : "";
  }
  std::string_view error() const noexcept {
    return result_.error ? result_.error : "";
  }

 private:
  friend class View;
  explicit EvalResult(ltv_eval_result_t result) : result_(result) {}

  void Release() noexcept {
    ltv_free(result_.json_result);
    ltv_free(result_.error);
    result_.json_result = nullptr;
    result_.error = nullptr;
  }

  ltv_eval_result_t result_{};
};

class View {
 private:
  struct PendingJavaScriptCallback {
    View* owner;
    std::function<void(const ltv_javascript_result_t&)> callback;
  };

 public:
  using JavaScriptCallback =
      std::function<void(const ltv_javascript_result_t&)>;

  static View Create(const ltv_view_client_t* client = nullptr) noexcept {
    return View(client ? ltv_view_create_with_client(client)
                       : ltv_view_create());
  }

  View(const View&) = delete;
  View& operator=(const View&) = delete;

  View(View&& other) noexcept
      : handle_(std::exchange(other.handle_, nullptr)),
        pending_javascript_callbacks_(
            std::move(other.pending_javascript_callbacks_)) {
    RebindPendingJavaScriptCallbacks();
  }
  View& operator=(View&& other) noexcept {
    if (this != &other) {
      reset();
      handle_ = std::exchange(other.handle_, nullptr);
      pending_javascript_callbacks_ =
          std::move(other.pending_javascript_callbacks_);
      RebindPendingJavaScriptCallbacks();
    }
    return *this;
  }

  ~View() { reset(); }

  explicit operator bool() const noexcept { return handle_ != nullptr; }
  ltv_view_t get() const noexcept { return handle_; }

  ltv_view_t release() noexcept {
    if (!pending_javascript_callbacks_.empty()) {
      return nullptr;
    }
    return std::exchange(handle_, nullptr);
  }

  ltv_error_t reset() noexcept {
    if (!handle_) {
      pending_javascript_callbacks_.clear();
      return LTV_OK;
    }
    const ltv_error_t result = ltv_view_destroy(handle_);
    if (result == LTV_OK) {
      handle_ = nullptr;
      pending_javascript_callbacks_.clear();
    }
    return result;
  }

  ltv_error_t SetClient(const ltv_view_client_t* client) noexcept {
    return ltv_view_set_client(handle_, client);
  }

  ltv_error_t LoadHtml(std::string_view html, std::string_view base_url = {}) {
    const std::string html_copy(html);
    const std::string base_url_copy(base_url);
    return ltv_view_load_html(handle_, html_copy.c_str(),
                              base_url_copy.c_str());
  }

  ltv_error_t Navigate(std::string_view url) {
    const std::string copy(url);
    return ltv_view_navigate(handle_, copy.c_str());
  }

  ltv_error_t GoBack() noexcept { return ltv_view_go_back(handle_); }
  ltv_error_t GoForward() noexcept { return ltv_view_go_forward(handle_); }
  ltv_error_t Reload() noexcept { return ltv_view_reload(handle_); }
  ltv_error_t Stop() noexcept { return ltv_view_stop(handle_); }

  ltv_error_t GetNavigationState(ltv_navigation_state_t* state) noexcept {
    return ltv_view_get_navigation_state(handle_, state);
  }

  EvalResult Eval(std::string_view script) {
    const std::string copy(script);
    return EvalResult(ltv_view_eval(handle_, copy.c_str()));
  }

  ltv_error_t ExecuteJavaScript(std::string_view script,
                                JavaScriptCallback callback = {}) {
    const std::string copy(script);
    if (!callback) {
      return ltv_view_execute_javascript(handle_, copy.c_str(), nullptr,
                                         nullptr);
    }
    auto* state =
        new (std::nothrow) PendingJavaScriptCallback{this, std::move(callback)};
    if (!state) {
      return LTV_ERR_RESOURCE_EXHAUSTED;
    }
    try {
      pending_javascript_callbacks_.emplace(
          state, std::unique_ptr<PendingJavaScriptCallback>(state));
    } catch (const std::bad_alloc&) {
      delete state;
      return LTV_ERR_RESOURCE_EXHAUSTED;
    }
    const ltv_error_t result = ltv_view_execute_javascript(
        handle_, copy.c_str(), &DispatchJavaScriptResult, state);
    if (result != LTV_OK) {
      pending_javascript_callbacks_.erase(state);
    }
    return result;
  }

  ltv_error_t PostJavaScriptMessage(std::string_view json_value) {
    const std::string copy(json_value);
    return ltv_view_post_javascript_message(handle_, copy.c_str());
  }

 private:
  explicit View(ltv_view_t handle) noexcept : handle_(handle) {}

  static void LTV_CALLBACK
  DispatchJavaScriptResult(ltv_view_t,
                           const ltv_javascript_result_t* result,
                           void* user_data) noexcept {
    auto* state = static_cast<PendingJavaScriptCallback*>(user_data);
    View* owner = state->owner;
    auto pending = owner->pending_javascript_callbacks_.find(state);
    if (pending == owner->pending_javascript_callbacks_.end()) {
      return;
    }
    std::unique_ptr<PendingJavaScriptCallback> callback =
        std::move(pending->second);
    owner->pending_javascript_callbacks_.erase(pending);
    if (result) {
      callback->callback(*result);
    }
  }

  void RebindPendingJavaScriptCallbacks() noexcept {
    for (auto& [state, callback] : pending_javascript_callbacks_) {
      callback->owner = this;
    }
  }

  ltv_view_t handle_ = nullptr;
  std::unordered_map<PendingJavaScriptCallback*,
                     std::unique_ptr<PendingJavaScriptCallback>>
      pending_javascript_callbacks_;
};

}  // namespace litheview

#endif  // LITHEVIEW_PUBLIC_LITHEVIEW_HPP_
