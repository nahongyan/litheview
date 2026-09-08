# LitheView Qt 6.8.3 example

This example embeds a LitheView SDK view in a native Qt Widgets window. It
uses only the installed CMake packages `Qt6::Widgets` and
`LitheView::LitheView`; no Chromium-private headers or libraries are needed.

The example keeps framework responsibilities separate:

- `main.cc` owns process and LitheView runtime lifetime.
- `browser_window.*` owns the Qt navigation UI.
- `browser_widget.*` owns the Qt container, SDK view and DPI lifecycle.
- `native_browser_surface.*` owns the accelerated HWND, input, cursor and
  native resize contracts.
- `demo_config.h` contains the shared startup configuration.

`ltv_run()` initializes the runtime first, then the ready callback enters
`QApplication::exec()` as an allowed native nested loop on the same UI thread.
This keeps both frameworks' posted events, paints and timers active without
moving LitheView calls to an unsupported secondary thread. The top-level HWND
also wraps `WM_SYSCOMMAND` in a nested modal-loop scope. For `SC_MOVE` and
`SC_SIZE` separately. Move places the currently presented browser frame in a
short-lived GDI child visual because DWM does not carry Qt's child
DirectComposition visual in its system move representation. Resize keeps the
attached GPU surface live, activates the Qt child layout for every top-level
`WM_SIZE`, commits each matching viewport from `WM_WINDOWPOSCHANGED`, and relies
on Aura's same-size bounds check to avoid a second renderer layout while still
allowing compositor redraw requests.

At `WM_EXITSIZEMOVE`, resize re-enables presentation, requests a full compositor
redraw, and continues directly on the attached GPU surface. The accelerated
surface is a dedicated child HWND created with `WS_EX_NOREDIRECTIONBITMAP`, so
DWM cannot expose a stale Qt backing surface between live DComp frames. Move
uses a short-lived, owner-owned layered popup for its final handoff after the
interaction visual.

From a Visual Studio 2022 x64 developer shell, configure against Qt 6.8.3 and
an unpacked LitheView SDK:

```powershell
$env:QTDIR = 'D:\Qt5\6.8.3\msvc2022_64'
$sdk = 'F:\path\to\litheview-sdk-0.1.0-win-x64'

cmake -S "$sdk\examples\litheview_qt_demo" `
      -B "$sdk\build\litheview_qt_demo" `
      -G Ninja `
      -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_PREFIX_PATH="$env:QTDIR" `
      -DLitheView_DIR="$sdk\lib\cmake\LitheView"
cmake --build "$sdk\build\litheview_qt_demo" --parallel 28
```

The post-build step copies `litheview.dll` and deploys the required Qt runtime
beside `litheview_qt_demo.exe`.

Run the native integration regression after building:

```powershell
powershell -ExecutionPolicy Bypass `
  -File examples/litheview_qt_demo/window_integration_regression_test.ps1 `
  -Executable build/litheview_qt_demo/litheview_qt_demo.exe
```
