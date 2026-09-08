#ifndef LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_DEMO_PAGES_H_
#define LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_DEMO_PAGES_H_

#include <string_view>

namespace litheview_demo {

enum class DemoPage {
  kExternal,
  kOverview,
  kBridge,
  kGpuOutput,
  kWebGl3D,
  kNativeApis,
};

struct DemoPageContent {
  const wchar_t* menu_title;
  const char* url;
  const char* html;
  bool uses_gpu_output;
};

inline constexpr DemoPage kDemoPages[] = {
    DemoPage::kOverview, DemoPage::kBridge, DemoPage::kGpuOutput,
    DemoPage::kWebGl3D, DemoPage::kNativeApis};

const DemoPageContent& GetDemoPageContent(DemoPage page);

}  // namespace litheview_demo

#endif  // LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_DEMO_PAGES_H_
