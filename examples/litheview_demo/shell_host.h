#ifndef LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_SHELL_HOST_H_
#define LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_SHELL_HOST_H_

#include <memory>
#include <string>
#include <vector>

#include "demo_pages.h"
#include "demo_support.h"
#include "shared_texture_presenter.h"

#include <commctrl.h>
#include <imm.h>
#include <oleidl.h>
#include <shellapi.h>
#include <windows.h>
#include <windowsx.h>

namespace litheview_demo {

class ShellHost;

class FileDropTarget final : public IDropTarget {
 public:
  FileDropTarget(ShellHost* host, HWND target);

  ULONG STDMETHODCALLTYPE AddRef() override;
  ULONG STDMETHODCALLTYPE Release() override;
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** object) override;
  HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* data_object,
                                      DWORD key_state,
                                      POINTL point,
                                      DWORD* effect) override;
  HRESULT STDMETHODCALLTYPE DragOver(DWORD key_state,
                                     POINTL point,
                                     DWORD* effect) override;
  HRESULT STDMETHODCALLTYPE DragLeave() override;
  HRESULT STDMETHODCALLTYPE Drop(IDataObject* data_object,
                                 DWORD key_state,
                                 POINTL point,
                                 DWORD* effect) override;

 private:
  ULONG ref_count_ = 1;
  ShellHost* const host_;
  const HWND target_;
  std::vector<std::wstring> paths_;
};

class ShellApp;

enum class ShellWindowMode {
  kBrowser,
  kInspector,
};

class ShellHost {
 public:
  ShellHost(ShellApp* app,
            ltv_view_t view,
            std::string initial_url,
            bool navigate_on_open,
            ShellWindowMode window_mode);
  ~ShellHost();

  bool Open();

 private:
  friend class FileDropTarget;

  static void LTV_CALLBACK OnUrlChanged(ltv_view_t,
                                        const char* url,
                                        void* user_data);
  static ltv_console_decision_t LTV_CALLBACK
  OnConsoleMessage(ltv_view_t,
                   const ltv_console_message_t* message,
                   void* user_data);
  static void LTV_CALLBACK
  OnJavaScriptMessage(ltv_view_t view,
                      const ltv_javascript_message_t* message,
                      void* user_data);
  static void LTV_CALLBACK
  OnNavigationStateChanged(ltv_view_t,
                           const ltv_navigation_state_t* state,
                           void* user_data);
  static void LTV_CALLBACK
  OnCustomJavaScriptResult(ltv_view_t,
                           const ltv_javascript_result_t* result,
                           void* user_data);
  static void LTV_CALLBACK OnTitleChanged(ltv_view_t,
                                          const char* title,
                                          void* user_data);
  static void LTV_CALLBACK
  OnLoadError(ltv_view_t, const char*, int32_t, const char*, void* user_data);
  static void LTV_CALLBACK OnClosed(ltv_view_t, void* user_data);
  static void LTV_CALLBACK
  OnRendererTerminated(ltv_view_t,
                       ltv_renderer_termination_status_t,
                       void* user_data);
  static ltv_close_decision_t LTV_CALLBACK OnCloseRequested(ltv_view_t, void*);
  static ltv_popup_decision_t LTV_CALLBACK
  OnBeforePopup(ltv_view_t, const ltv_popup_request_t*, void*);
  static void LTV_CALLBACK OnPopupCreated(ltv_view_t,
                                          ltv_view_t popup,
                                          const ltv_popup_request_t* request,
                                          void* user_data);
  static ltv_dialog_handling_t LTV_CALLBACK
  OnJavaScriptDialog(ltv_view_t view,
                     const ltv_javascript_dialog_request_t* request,
                     void* user_data);
  static ltv_file_dialog_handling_t LTV_CALLBACK
  OnFileDialog(ltv_view_t view,
               const ltv_file_dialog_request_t* request,
               void* user_data);
  static ltv_download_handling_t LTV_CALLBACK
  OnBeforeDownload(ltv_view_t view,
                   const ltv_download_request_t* request,
                   void* user_data);
  static void LTV_CALLBACK
  OnDownloadUpdated(ltv_view_t,
                    const ltv_download_update_t* update,
                    void* user_data);
  static void LTV_CALLBACK OnGpuFrame(ltv_view_t view,
                                      const ltv_gpu_frame_t* frame,
                                      void* user_data);

  static LRESULT CALLBACK AddressProc(HWND window,
                                      UINT message,
                                      WPARAM wparam,
                                      LPARAM lparam);
  static LRESULT CALLBACK ButtonProc(HWND window,
                                     UINT message,
                                     WPARAM wparam,
                                     LPARAM lparam);
  static LRESULT CALLBACK WindowProc(HWND window,
                                     UINT message,
                                     WPARAM wparam,
                                     LPARAM lparam);
  static LRESULT CALLBACK RenderSurfaceProc(HWND window,
                                            UINT message,
                                            WPARAM wparam,
                                            LPARAM lparam);

  LRESULT HandleMessage(UINT message, WPARAM wparam, LPARAM lparam);
  LRESULT HandleRenderSurfaceMessage(HWND window,
                                     UINT message,
                                     WPARAM wparam,
                                     LPARAM lparam);
  LRESULT HandlePageMessage(HWND source,
                            UINT message,
                            WPARAM wparam,
                            LPARAM lparam);
  void HandleImeComposition(HWND window, LPARAM flags);
  void UpdateImeWindows(HIMC context);
  bool HandleDevToolsKeyMessage(UINT message, WPARAM wparam, LPARAM lparam);

  ltv_view_client_t MakeViewClient();
  bool InitializeView();
  bool LoadDemoPage(DemoPage page);
  bool ConfigureOutputForPage(DemoPage page);
  bool AttachView();
  bool StartGpuOutput();
  void SuspendGpuOutputForResize();
  void ResumeGpuOutputAfterResize();
  void StopGpuOutput();
  void DestroyView();

  bool CreateApplicationMenu();
  void ShowApplicationMenu();
  void UpdateDemoPageMenu();
  void AppendLog(std::string message);
  void Layout();
  int ScaleNativeDip(int value) const;
  int NavigationBarHeightPixels() const;
  bool HasNavigationControls() const;
  PixelSize PageSizePixels() const;
  void ResizeView();
  LPARAM ToScreenPosition(HWND source, int x, int y) const;
  bool SetPageCursor() const;

  bool RegisterFileDropTargets();
  void RevokeFileDropTargets();
  HRESULT HandleFileDragEnter(HWND source,
                              const std::vector<std::wstring>& paths,
                              POINT screen_point,
                              DWORD* effect);
  HRESULT HandleFileDragOver(HWND source, POINT screen_point, DWORD* effect);
  HRESULT HandleFileDragLeave();
  HRESULT HandleFileDrop(HWND source,
                         const std::vector<std::wstring>& paths,
                         POINT screen_point,
                         DWORD* effect);
  void Navigate();
  void Paint();

  ShellApp* const app_;
  ltv_view_t view_ = nullptr;
  std::string initial_url_;
  const bool navigate_on_open_;
  const ShellWindowMode window_mode_;
  HWND window_ = nullptr;
  HWND back_button_ = nullptr;
  HWND forward_button_ = nullptr;
  HWND menu_button_ = nullptr;
  HWND reload_button_ = nullptr;
  HWND address_ = nullptr;
  HWND render_window_ = nullptr;
  HWND gpu_output_window_ = nullptr;
  HMENU application_menu_ = nullptr;
  WNDPROC address_window_proc_ = nullptr;
  WNDPROC button_window_proc_ = nullptr;
  WNDPROC render_window_proc_ = nullptr;
  DemoPage current_page_ = DemoPage::kExternal;
  std::string current_url_;
  PixelSize view_size_dips_;
  PixelSize surface_size_pixels_;
  std::unique_ptr<SharedTexturePresenter> presenter_;

  std::unique_ptr<FileDropTarget> render_drop_target_;
  std::unique_ptr<FileDropTarget> gpu_drop_target_;
  float native_scale_factor_ = 1.0f;
  float device_scale_factor_ = 1.0f;
  bool view_attached_ = false;
  bool gpu_output_started_ = false;
  bool gpu_output_pending_ = false;
  bool gpu_output_suspended_for_resize_ = false;
  bool is_loading_ = false;
  bool mouse_button_captured_ = false;
  HWND mouse_capture_window_ = nullptr;
  UINT captured_button_up_message_ = 0;
  bool ime_window_update_in_progress_ = false;
  bool window_ready_ = false;
};

}  // namespace litheview_demo

#endif  // LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_SHELL_HOST_H_
