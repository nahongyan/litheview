#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "demo_support.h"
#include "shell_host.h"

namespace litheview_demo {

namespace {

constexpr DWORD kDropEffectMask =
    DROPEFFECT_COPY | DROPEFFECT_LINK | DROPEFFECT_MOVE;

uint32_t ToLitheViewDragOperations(DWORD effect) {
  uint32_t operations = LTV_WIN_DRAG_OPERATION_NONE;
  if (effect & DROPEFFECT_COPY) {
    operations |= LTV_WIN_DRAG_OPERATION_COPY;
  }
  if (effect & DROPEFFECT_LINK) {
    operations |= LTV_WIN_DRAG_OPERATION_LINK;
  }
  if (effect & DROPEFFECT_MOVE) {
    operations |= LTV_WIN_DRAG_OPERATION_MOVE;
  }
  return operations;
}

DWORD PreferredDropEffect(DWORD allowed_effects) {
  if (allowed_effects & DROPEFFECT_COPY) {
    return DROPEFFECT_COPY;
  }
  if (allowed_effects & DROPEFFECT_MOVE) {
    return DROPEFFECT_MOVE;
  }
  if (allowed_effects & DROPEFFECT_LINK) {
    return DROPEFFECT_LINK;
  }
  return DROPEFFECT_NONE;
}

bool BuildUtf8PathPointers(const std::vector<std::wstring>& paths,
                           std::vector<std::string>* utf8_paths,
                           std::vector<const char*>* pointers) {
  if (!utf8_paths || !pointers || paths.empty()) {
    return false;
  }
  utf8_paths->clear();
  pointers->clear();
  utf8_paths->reserve(paths.size());
  pointers->reserve(paths.size());
  for (const std::wstring& path : paths) {
    if (path.empty()) {
      return false;
    }
    utf8_paths->push_back(WideToUtf8(path));
  }
  for (const std::string& path : *utf8_paths) {
    pointers->push_back(path.c_str());
  }
  return true;
}

std::vector<std::wstring> ExtractFilePaths(IDataObject* data_object) {
  if (!data_object) {
    return {};
  }

  FORMATETC format = {};
  format.cfFormat = CF_HDROP;
  format.dwAspect = DVASPECT_CONTENT;
  format.lindex = -1;
  format.tymed = TYMED_HGLOBAL;

  STGMEDIUM medium = {};
  if (FAILED(data_object->GetData(&format, &medium))) {
    return {};
  }

  std::vector<std::wstring> paths;
  HDROP drop = reinterpret_cast<HDROP>(medium.hGlobal);
  const UINT count = DragQueryFileW(drop, 0xFFFFFFFFu, nullptr, 0);
  paths.reserve(count);
  for (UINT index = 0; index < count; ++index) {
    const UINT length = DragQueryFileW(drop, index, nullptr, 0);
    if (length == 0) {
      continue;
    }
    std::wstring path(length + 1, L'\0');
    if (DragQueryFileW(drop, index, path.data(), length + 1) == length) {
      path.resize(length);
      paths.push_back(std::move(path));
    }
  }
  ReleaseStgMedium(&medium);
  return paths;
}

POINT ScreenPointFromDragPoint(POINTL point) {
  return {point.x, point.y};
}

}  // namespace

FileDropTarget::FileDropTarget(ShellHost* host, HWND target)
    : host_(host), target_(target) {}

ULONG STDMETHODCALLTYPE FileDropTarget::AddRef() {
  return ++ref_count_;
}

ULONG STDMETHODCALLTYPE FileDropTarget::Release() {
  const ULONG ref_count = --ref_count_;
  if (ref_count == 0) {
    delete this;
  }
  return ref_count;
}

HRESULT STDMETHODCALLTYPE FileDropTarget::QueryInterface(REFIID iid,
                                                         void** object) {
  if (!object) {
    return E_POINTER;
  }
  if (iid == IID_IUnknown || iid == IID_IDropTarget) {
    *object = static_cast<IDropTarget*>(this);
    AddRef();
    return S_OK;
  }
  *object = nullptr;
  return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE FileDropTarget::DragEnter(IDataObject* data_object,
                                                    DWORD,
                                                    POINTL point,
                                                    DWORD* effect) {
  paths_ = ExtractFilePaths(data_object);
  if (paths_.empty()) {
    if (effect) {
      *effect = DROPEFFECT_NONE;
    }
    return S_OK;
  }
  return host_->HandleFileDragEnter(target_, paths_,
                                    ScreenPointFromDragPoint(point), effect);
}

HRESULT STDMETHODCALLTYPE FileDropTarget::DragOver(DWORD,
                                                   POINTL point,
                                                   DWORD* effect) {
  return host_->HandleFileDragOver(target_, ScreenPointFromDragPoint(point),
                                   effect);
}

HRESULT STDMETHODCALLTYPE FileDropTarget::DragLeave() {
  paths_.clear();
  return host_->HandleFileDragLeave();
}

HRESULT STDMETHODCALLTYPE FileDropTarget::Drop(IDataObject* data_object,
                                               DWORD,
                                               POINTL point,
                                               DWORD* effect) {
  std::vector<std::wstring> paths = ExtractFilePaths(data_object);
  if (paths.empty()) {
    paths = paths_;
  }
  paths_.clear();
  return host_->HandleFileDrop(target_, paths, ScreenPointFromDragPoint(point),
                               effect);
}

bool ShellHost::RegisterFileDropTargets() {
  if (!render_window_ || !gpu_output_window_) {
    return false;
  }

  RevokeFileDropTargets();
  render_drop_target_ = std::make_unique<FileDropTarget>(this, render_window_);
  if (FAILED(RegisterDragDrop(render_window_, render_drop_target_.get()))) {
    render_drop_target_.reset();
    return false;
  }

  gpu_drop_target_ = std::make_unique<FileDropTarget>(this, gpu_output_window_);
  if (FAILED(RegisterDragDrop(gpu_output_window_, gpu_drop_target_.get()))) {
    RevokeDragDrop(render_window_);
    render_drop_target_.reset();
    gpu_drop_target_.reset();
    return false;
  }
  return true;
}

void ShellHost::RevokeFileDropTargets() {
  if (render_drop_target_ && render_window_) {
    RevokeDragDrop(render_window_);
    render_drop_target_.reset();
  }
  if (gpu_drop_target_ && gpu_output_window_) {
    RevokeDragDrop(gpu_output_window_);
    gpu_drop_target_.reset();
  }
}

HRESULT ShellHost::HandleFileDragEnter(HWND source,
                                       const std::vector<std::wstring>& paths,
                                       POINT screen_point,
                                       DWORD* effect) {
  if (!view_ || !effect || paths.empty()) {
    if (effect) {
      *effect = DROPEFFECT_NONE;
    }
    return S_OK;
  }

  std::vector<std::string> utf8_paths;
  std::vector<const char*> path_pointers;
  const DWORD allowed_effects = *effect & kDropEffectMask;
  const uint32_t operations = ToLitheViewDragOperations(allowed_effects);
  if (operations == static_cast<uint32_t>(LTV_WIN_DRAG_OPERATION_NONE) ||
      !BuildUtf8PathPointers(paths, &utf8_paths, &path_pointers)) {
    *effect = DROPEFFECT_NONE;
    return S_OK;
  }

  POINT client_point = screen_point;
  if (!ScreenToClient(source, &client_point) ||
      ltv_win_view_drag_files_enter(view_, path_pointers.data(),
                                    static_cast<uint32_t>(path_pointers.size()),
                                    client_point.x, client_point.y,
                                    screen_point.x, screen_point.y,
                                    operations) != LTV_OK) {
    ltv_win_view_drag_files_leave(view_);
    *effect = DROPEFFECT_NONE;
    return S_OK;
  }

  *effect = PreferredDropEffect(allowed_effects);
  return S_OK;
}

HRESULT ShellHost::HandleFileDragOver(HWND source,
                                      POINT screen_point,
                                      DWORD* effect) {
  if (!view_ || !effect) {
    if (effect) {
      *effect = DROPEFFECT_NONE;
    }
    return S_OK;
  }

  const DWORD allowed_effects = *effect & kDropEffectMask;
  const uint32_t operations = ToLitheViewDragOperations(allowed_effects);
  POINT client_point = screen_point;
  if (operations == static_cast<uint32_t>(LTV_WIN_DRAG_OPERATION_NONE) ||
      !ScreenToClient(source, &client_point) ||
      ltv_win_view_drag_files_over(view_, client_point.x, client_point.y,
                                   screen_point.x, screen_point.y,
                                   operations) != LTV_OK) {
    *effect = DROPEFFECT_NONE;
    return S_OK;
  }
  *effect = PreferredDropEffect(allowed_effects);
  return S_OK;
}

HRESULT ShellHost::HandleFileDragLeave() {
  if (view_) {
    ltv_win_view_drag_files_leave(view_);
  }
  return S_OK;
}

HRESULT ShellHost::HandleFileDrop(HWND source,
                                  const std::vector<std::wstring>& paths,
                                  POINT screen_point,
                                  DWORD* effect) {
  if (!view_ || !effect || paths.empty()) {
    if (effect) {
      *effect = DROPEFFECT_NONE;
    }
    return S_OK;
  }

  std::vector<std::string> utf8_paths;
  std::vector<const char*> path_pointers;
  if (!BuildUtf8PathPointers(paths, &utf8_paths, &path_pointers)) {
    *effect = DROPEFFECT_NONE;
    ltv_win_view_drag_files_leave(view_);
    return S_OK;
  }

  POINT client_point = screen_point;
  const DWORD allowed_effects = *effect & kDropEffectMask;
  if (!ScreenToClient(source, &client_point) ||
      ltv_win_view_drag_files_drop(view_, path_pointers.data(),
                                   static_cast<uint32_t>(path_pointers.size()),
                                   client_point.x, client_point.y,
                                   screen_point.x, screen_point.y) != LTV_OK) {
    *effect = DROPEFFECT_NONE;
    ltv_win_view_drag_files_leave(view_);
    return S_OK;
  }

  *effect = PreferredDropEffect(allowed_effects);
  return S_OK;
}

}  // namespace litheview_demo
