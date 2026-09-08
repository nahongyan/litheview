#ifndef LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_NATIVE_DIALOGS_H_
#define LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_NATIVE_DIALOGS_H_

#include <windows.h>

#include <litheview/litheview.h>

#include <string>
#include <vector>

namespace litheview_demo {

int RunNativeMessageBox(HWND owner, const std::wstring& message, UINT type);
std::vector<std::wstring> RunNativeFilePicker(
    HWND owner,
    const ltv_file_dialog_request_t& request);
std::vector<std::wstring> RunNativeFolderPicker(
    HWND owner,
    const ltv_file_dialog_request_t& request);
std::wstring RunNativeDownloadPicker(HWND owner,
                                     const ltv_download_request_t& request);
bool RunNativePrompt(HWND owner,
                     const std::wstring& message,
                     const std::wstring& default_value,
                     std::wstring* value);

}  // namespace litheview_demo

#endif  // LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_NATIVE_DIALOGS_H_
