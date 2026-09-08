#include "native_dialogs.h"

#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>

#include <cwchar>
#include <utility>
#include <vector>

#include "demo_support.h"

namespace litheview_demo {

int RunNativeMessageBox(HWND owner, const std::wstring& message, UINT type) {
  const bool modal_loop_enabled = ltv_win_set_os_modal_loop(1) == LTV_OK;
  const int result = MessageBoxW(owner, message.c_str(), kWindowTitle, type);
  if (modal_loop_enabled) {
    ltv_win_set_os_modal_loop(0);
  }
  return result;
}

std::wstring BuildFileDialogFilter(const ltv_file_dialog_request_t& request) {
  std::wstring patterns;
  for (uint32_t index = 0; index < request.accept_type_count; ++index) {
    if (!request.accept_types || !request.accept_types[index] ||
        request.accept_types[index][0] != '.') {
      continue;
    }
    if (!patterns.empty()) {
      patterns.push_back(L';');
    }
    patterns.push_back(L'*');
    patterns.append(Utf8ToWide(request.accept_types[index]));
  }

  std::wstring filter;
  if (!patterns.empty()) {
    filter.append(L"Accepted files");
    filter.push_back(L'\0');
    filter.append(patterns);
    filter.push_back(L'\0');
  }
  filter.append(L"All files");
  filter.push_back(L'\0');
  filter.append(L"*.*");
  filter.push_back(L'\0');
  filter.push_back(L'\0');
  return filter;
}

std::vector<std::wstring> RunNativeFilePicker(
    HWND owner,
    const ltv_file_dialog_request_t& request) {
  constexpr DWORD kBufferCharacters = 65536;
  std::vector<wchar_t> file_buffer(kBufferCharacters, L'\0');
  const char* initial_file = request.default_file_name;
  if ((!initial_file || !*initial_file) && request.selected_file_count != 0 &&
      request.selected_files) {
    initial_file = request.selected_files[0];
  }
  if (initial_file && *initial_file) {
    const std::wstring initial_file_wide = Utf8ToWide(initial_file);
    wcsncpy_s(file_buffer.data(), file_buffer.size(), initial_file_wide.c_str(),
              _TRUNCATE);
  }

  const std::wstring title = Utf8ToWide(request.title ? request.title : "");
  const std::wstring filter = BuildFileDialogFilter(request);
  OPENFILENAMEW dialog = {};
  dialog.lStructSize = sizeof(dialog);
  dialog.hwndOwner = owner;
  dialog.lpstrTitle = title.empty() ? nullptr : title.c_str();
  dialog.lpstrFilter = filter.c_str();
  dialog.lpstrFile = file_buffer.data();
  dialog.nMaxFile = static_cast<DWORD>(file_buffer.size());
  dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

  const bool save = request.mode == LTV_FILE_DIALOG_SAVE;
  if (save) {
    dialog.Flags |= OFN_OVERWRITEPROMPT;
  } else {
    dialog.Flags |= OFN_FILEMUSTEXIST;
    if (request.mode == LTV_FILE_DIALOG_OPEN_MULTIPLE) {
      dialog.Flags |= OFN_ALLOWMULTISELECT;
    }
  }

  const bool modal_loop_enabled = ltv_win_set_os_modal_loop(1) == LTV_OK;
  const BOOL selected =
      save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
  if (modal_loop_enabled) {
    ltv_win_set_os_modal_loop(0);
  }
  if (!selected) {
    return {};
  }

  std::vector<std::wstring> paths;
  const wchar_t* first = file_buffer.data();
  const wchar_t* next = first + std::wcslen(first) + 1;
  if (!*next) {
    paths.emplace_back(first);
    return paths;
  }
  const std::wstring directory(first);
  while (*next) {
    std::wstring path = directory;
    if (!path.empty() && path.back() != L'\\' && path.back() != L'/') {
      path.push_back(L'\\');
    }
    path.append(next);
    paths.push_back(std::move(path));
    next += std::wcslen(next) + 1;
  }
  return paths;
}

std::vector<std::wstring> RunNativeFolderPicker(
    HWND owner,
    const ltv_file_dialog_request_t& request) {
  const std::wstring title = Utf8ToWide(request.title ? request.title : "");
  BROWSEINFOW dialog = {};
  dialog.hwndOwner = owner;
  dialog.lpszTitle = title.empty() ? nullptr : title.c_str();
  dialog.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

  const bool modal_loop_enabled = ltv_win_set_os_modal_loop(1) == LTV_OK;
  LPITEMIDLIST item = SHBrowseForFolderW(&dialog);
  if (modal_loop_enabled) {
    ltv_win_set_os_modal_loop(0);
  }
  if (!item) {
    return {};
  }
  wchar_t path[MAX_PATH] = {};
  const BOOL converted = SHGetPathFromIDListW(item, path);
  CoTaskMemFree(item);
  return converted ? std::vector<std::wstring>{path}
                   : std::vector<std::wstring>{};
}

std::wstring RunNativeDownloadPicker(HWND owner,
                                     const ltv_download_request_t& request) {
  constexpr DWORD kBufferCharacters = 32768;
  std::vector<wchar_t> file_buffer(kBufferCharacters, L'\0');
  const std::wstring suggested_file_name = Utf8ToWide(
      request.suggested_file_name ? request.suggested_file_name : "download");
  wcsncpy_s(file_buffer.data(), file_buffer.size(), suggested_file_name.c_str(),
            _TRUNCATE);

  const wchar_t filter[] = L"All files\0*.*\0\0";
  OPENFILENAMEW dialog = {};
  dialog.lStructSize = sizeof(dialog);
  dialog.hwndOwner = owner;
  dialog.lpstrTitle = L"Save download";
  dialog.lpstrFilter = filter;
  dialog.lpstrFile = file_buffer.data();
  dialog.nMaxFile = static_cast<DWORD>(file_buffer.size());
  dialog.Flags =
      OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

  const bool modal_loop_enabled = ltv_win_set_os_modal_loop(1) == LTV_OK;
  const BOOL selected = GetSaveFileNameW(&dialog);
  if (modal_loop_enabled) {
    ltv_win_set_os_modal_loop(0);
  }
  return selected ? std::wstring(file_buffer.data()) : std::wstring();
}

class PromptDialog {
 public:
  static bool Show(HWND owner,
                   const std::wstring& message,
                   const std::wstring& default_value,
                   std::wstring* value) {
    PromptDialog dialog(owner, message, default_value);
    if (!dialog.Run()) {
      return false;
    }
    *value = std::move(dialog.value_);
    return true;
  }

 private:
  static constexpr wchar_t kClassName[] = L"LitheViewPromptDialog";

  PromptDialog(HWND owner,
               const std::wstring& message,
               const std::wstring& default_value)
      : owner_(owner), message_(message), default_value_(default_value) {}

  int Scale(int value) const {
    return MulDiv(value, dpi_, USER_DEFAULT_SCREEN_DPI);
  }

  bool Run() {
    const HINSTANCE instance = GetModuleHandle(nullptr);
    WNDCLASS window_class = {};
    window_class.lpfnWndProc = &PromptDialog::WindowProc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    window_class.lpszClassName = kClassName;
    if (!RegisterClass(&window_class) &&
        GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
      return false;
    }

    dpi_ = owner_ ? GetDpiForWindow(owner_) : GetDpiForSystem();
    const int width = Scale(440);
    const int height = Scale(190);
    RECT owner_bounds = {};
    GetWindowRect(owner_, &owner_bounds);
    const int x =
        owner_ ? owner_bounds.left +
                     ((owner_bounds.right - owner_bounds.left) - width) / 2
               : CW_USEDEFAULT;
    const int y =
        owner_ ? owner_bounds.top +
                     ((owner_bounds.bottom - owner_bounds.top) - height) / 2
               : CW_USEDEFAULT;
    window_ =
        CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, kClassName,
                       kWindowTitle, WS_CAPTION | WS_SYSMENU | WS_POPUP, x, y,
                       width, height, owner_, nullptr, instance, this);
    if (!window_) {
      return false;
    }
    CreateControls(instance);

    if (owner_) {
      EnableWindow(owner_, FALSE);
    }
    ShowWindow(window_, SW_SHOW);
    UpdateWindow(window_);
    SetFocus(edit_);
    SendMessage(edit_, EM_SETSEL, 0, -1);

    const bool modal_loop_enabled = ltv_win_set_os_modal_loop(1) == LTV_OK;
    MSG message = {};
    bool received_quit = false;
    while (!finished_) {
      const BOOL status = GetMessage(&message, nullptr, 0, 0);
      if (status <= 0) {
        received_quit = status == 0;
        break;
      }
      if (!IsDialogMessage(window_, &message)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
      }
    }
    if (modal_loop_enabled) {
      ltv_win_set_os_modal_loop(0);
    }
    if (window_) {
      DestroyWindow(window_);
      window_ = nullptr;
    }
    if (owner_) {
      EnableWindow(owner_, TRUE);
      SetActiveWindow(owner_);
    }
    if (received_quit) {
      PostQuitMessage(static_cast<int>(message.wParam));
    }
    return accepted_;
  }

  void CreateControls(HINSTANCE instance) {
    const HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HWND label = CreateWindow(
        L"STATIC", message_.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT, Scale(16),
        Scale(16), Scale(400), Scale(48), window_, nullptr, instance, nullptr);
    edit_ = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", default_value_.c_str(),
                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                           Scale(16), Scale(72), Scale(400), Scale(24), window_,
                           nullptr, instance, nullptr);
    HWND ok = CreateWindow(
        L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        Scale(252), Scale(112), Scale(76), Scale(26), window_, ControlId(IDOK),
        instance, nullptr);
    HWND cancel =
        CreateWindow(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                     Scale(340), Scale(112), Scale(76), Scale(26), window_,
                     ControlId(IDCANCEL), instance, nullptr);
    for (HWND control : {label, edit_, ok, cancel}) {
      SendMessage(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
  }

  void Finish(bool accepted) {
    if (accepted) {
      const int length = GetWindowTextLength(edit_);
      value_.resize(length + 1);
      GetWindowText(edit_, value_.data(), length + 1);
      value_.resize(length);
    }
    accepted_ = accepted;
    finished_ = true;
    if (window_) {
      DestroyWindow(window_);
      window_ = nullptr;
    }
  }

  static LRESULT CALLBACK WindowProc(HWND window,
                                     UINT message,
                                     WPARAM wparam,
                                     LPARAM lparam) {
    auto* self = reinterpret_cast<PromptDialog*>(
        GetWindowLongPtr(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
      auto* create = reinterpret_cast<CREATESTRUCT*>(lparam);
      self = static_cast<PromptDialog*>(create->lpCreateParams);
      self->window_ = window;
      SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self) {
      return DefWindowProc(window, message, wparam, lparam);
    }
    if (message == WM_COMMAND) {
      const int command = LOWORD(wparam);
      if (command == IDOK || command == IDCANCEL) {
        self->Finish(command == IDOK);
        return 0;
      }
    } else if (message == WM_CLOSE) {
      self->Finish(false);
      return 0;
    }
    return DefWindowProc(window, message, wparam, lparam);
  }

  HWND owner_ = nullptr;
  HWND window_ = nullptr;
  HWND edit_ = nullptr;
  std::wstring message_;
  std::wstring default_value_;
  std::wstring value_;
  UINT dpi_ = USER_DEFAULT_SCREEN_DPI;
  bool accepted_ = false;
  bool finished_ = false;
};

bool RunNativePrompt(HWND owner,
                     const std::wstring& message,
                     const std::wstring& default_value,
                     std::wstring* value) {
  return PromptDialog::Show(owner, message, default_value, value);
}

}  // namespace litheview_demo
