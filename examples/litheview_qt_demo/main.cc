#include <litheview/litheview.h>
#include <litheview/litheview_win.h>

#include <QApplication>
#include <QScreen>
#include <memory>
#include <vector>

#include "browser_window.h"
#include "demo_config.h"

namespace litheview_qt_demo {
namespace {

struct RuntimeState {
  QApplication* application = nullptr;
  std::unique_ptr<BrowserWindow> window;
  int exit_code = 1;
};

void LTV_CALLBACK OnRuntimeReady(void* user_data) {
  auto* state = static_cast<RuntimeState*>(user_data);
  state->window = std::make_unique<BrowserWindow>();
  state->window->show();
  if (!state->window->Initialize()) {
    state->window.reset();
    ltv_shutdown();
    return;
  }

  if (ltv_win_set_os_modal_loop(1) != LTV_OK) {
    state->window->close();
    state->window.reset();
    ltv_shutdown();
    return;
  }
  state->exit_code = state->application->exec();
  state->window.reset();
  ltv_win_set_os_modal_loop(0);
  ltv_shutdown();
}

}  // namespace
}  // namespace litheview_qt_demo

int main(int argc, char* argv[]) {
  QApplication application(argc, argv);
  application.setApplicationName(QStringLiteral("LitheView Qt SDK Demo"));

  const qreal scale = application.primaryScreen()
                          ? application.primaryScreen()->devicePixelRatio()
                          : 1.0;
  const ltv_settings_t settings = {
      .struct_size = sizeof(ltv_settings_t),
      .version = LTV_STRUCT_VERSION,
      .width_dip = litheview_qt_demo::kInitialWidth,
      .height_dip = litheview_qt_demo::kInitialHeight,
      .device_scale_factor = static_cast<float>(scale),
  };
  if (ltv_initialize(&settings) != LTV_OK) {
    return 1;
  }

  std::vector<const char*> arguments;
  arguments.reserve(static_cast<size_t>(argc));
  for (int index = 0; index < argc; ++index) {
    arguments.push_back(argv[index]);
  }

  litheview_qt_demo::RuntimeState state = {
      .application = &application,
  };
  const int runtime_exit_code = ltv_run(
      argc, arguments.data(), &litheview_qt_demo::OnRuntimeReady, &state);
  state.window.reset();
  return runtime_exit_code == 0 ? state.exit_code : runtime_exit_code;
}
