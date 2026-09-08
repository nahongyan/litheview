#include <litheview/litheview.h>
#include <litheview/litheview_win.h>

#include "shell.h"

#if defined(_WIN32)
#include <windows.h>

#if !defined(WIN_CONSOLE_APP)
int APIENTRY wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int) {
  return litheview_demo::RunDemo(0, nullptr);
}
#else
int main() {
  return litheview_demo::RunDemo(0, nullptr);
}
#endif

#else
int main(int argc, const char** argv) {
  return litheview_demo::RunDemo(argc, argv);
}
#endif
