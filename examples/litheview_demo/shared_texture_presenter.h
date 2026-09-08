#ifndef LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_SHARED_TEXTURE_PRESENTER_H_
#define LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_SHARED_TEXTURE_PRESENTER_H_

#include <litheview/litheview.h>
#include <windows.h>

#include <memory>

namespace litheview_demo {

class SharedTexturePresenter {
 public:
  explicit SharedTexturePresenter(HWND target_window);
  ~SharedTexturePresenter();

  SharedTexturePresenter(const SharedTexturePresenter&) = delete;
  SharedTexturePresenter& operator=(const SharedTexturePresenter&) = delete;

  bool Present(const ltv_gpu_frame_t& frame);
  void Reset();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace litheview_demo

#endif  // LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_SHARED_TEXTURE_PRESENTER_H_
