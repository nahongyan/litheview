#ifndef LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_DEMO_SUPPORT_H_
#define LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_DEMO_SUPPORT_H_

#include <windows.h>

#include <litheview/litheview.h>
#include <litheview/litheview_win.h>
#include <stdint.h>

#include <string>
#include <string_view>
#include <vector>

namespace litheview_demo {

constexpr wchar_t kWindowClass[] = L"LitheViewDemo";
constexpr wchar_t kWindowTitle[] = L"LitheView Demo";
constexpr wchar_t kNavigationFailedTitle[] =
    L"LitheView Demo - navigation failed";
constexpr wchar_t kRendererTerminatedTitle[] =
    L"LitheView Demo - renderer terminated";
constexpr int kAddressId = 1001;
constexpr int kMenuId = 1002;
constexpr int kRenderSurfaceId = 1003;
constexpr int kBackId = 1004;
constexpr int kForwardId = 1005;
constexpr int kReloadId = 1006;
constexpr int kExitId = 1100;
constexpr int kDevToolsId = 1101;
constexpr int kDemoPageFirstId = 2000;
constexpr UINT kInitializePageMessage = WM_APP + 1;
constexpr UINT kViewClosedMessage = WM_APP + 2;
constexpr int kNavigationBarHeight = 28;
constexpr int kViewWidth = 1280;
constexpr int kViewHeight = 864;
constexpr int kDefaultWindowWidth = 1280;
constexpr int kDefaultWindowHeight = 900;
constexpr int kMinimumWindowWidth = 720;
constexpr int kMinimumWindowHeight = 480;
constexpr char kDefaultInitialUrl[] = "https://litheview.com/";

struct PixelSize {
  int width = 0;
  int height = 0;

  bool operator==(const PixelSize&) const = default;
};

HMENU ControlId(int id);
std::string WideToUtf8(const std::wstring& value);
std::wstring Utf8ToWide(const std::string& value);
std::vector<uint16_t> ReadImeUtf16(HIMC context, DWORD index);
std::vector<uint8_t> ReadImeBytes(HIMC context, DWORD index);
bool IsImeTargetAttribute(uint8_t attribute);
std::string FixupUrl(std::string url);

}  // namespace litheview_demo

#endif  // LITHEVIEW_EXAMPLES_LITHEVIEW_DEMO_DEMO_SUPPORT_H_
