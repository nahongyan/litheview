param(
    [string]$Executable = (Join-Path $PSScriptRoot `
        "..\..\..\..\out\wr-minimal\litheview-qt-example\litheview_qt_demo.exe"),
    [string]$CaptureDirectory = ""
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading;

public static class LitheViewQtWindowTest {
  public delegate bool EnumWindowsCallback(IntPtr window, IntPtr state);

  [StructLayout(LayoutKind.Sequential)]
  public struct Rect {
    public int Left;
    public int Top;
    public int Right;
    public int Bottom;
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct CursorInfo {
    public int size;
    public int flags;
    public IntPtr cursor;
    public System.Drawing.Point screen_position;
  }

  [StructLayout(LayoutKind.Sequential)]
  public struct GuiThreadInfo {
    public int size;
    public int flags;
    public IntPtr active;
    public IntPtr focus;
    public IntPtr capture;
    public IntPtr menu_owner;
    public IntPtr move_size;
    public IntPtr caret;
    public Rect caret_rect;
  }

  [DllImport("user32.dll")]
  public static extern bool EnumChildWindows(
      IntPtr parent, EnumWindowsCallback callback, IntPtr state);

  [DllImport("user32.dll")]
  public static extern bool GetWindowRect(IntPtr window, out Rect rect);

  [DllImport("user32.dll", EntryPoint = "GetWindowLongPtrW")]
  public static extern IntPtr GetWindowLongPtr(IntPtr window, int index);

  [DllImport("user32.dll")]
  public static extern bool SetForegroundWindow(IntPtr window);

  [DllImport("user32.dll")]
  public static extern IntPtr GetForegroundWindow();

  [DllImport("user32.dll")]
  public static extern bool SetWindowPos(
      IntPtr window, IntPtr insert_after, int x, int y, int width, int height,
      uint flags);

  [DllImport("user32.dll")]
  public static extern bool SetCursorPos(int x, int y);

  [DllImport("user32.dll")]
  public static extern void mouse_event(
      uint flags, uint x, uint y, uint data, UIntPtr extra_info);

  [DllImport("user32.dll")]
  public static extern bool GetCursorInfo(ref CursorInfo cursor_info);

  [DllImport("user32.dll")]
  public static extern uint GetWindowThreadProcessId(
      IntPtr window, IntPtr process_id);

  [DllImport("user32.dll")]
  public static extern bool GetGUIThreadInfo(
      uint thread_id, ref GuiThreadInfo info);

  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern IntPtr LoadCursor(IntPtr instance, IntPtr resource);

  [DllImport("user32.dll")]
  public static extern bool PostMessage(
      IntPtr window, uint message, IntPtr wparam, IntPtr lparam);

  public static Thread BeginMouseDrag(
      int start_x, int start_y, int end_x, int end_y,
      uint mouse_left_down, uint mouse_left_up) {
    var thread = new Thread(() => {
      SetCursorPos(start_x, start_y);
      mouse_event(mouse_left_down, 0, 0, 0, UIntPtr.Zero);
      Thread.Sleep(30);
      for (int step = 1; step <= 120; ++step) {
        SetCursorPos(start_x + (end_x - start_x) * step / 120,
                     start_y + (end_y - start_y) * step / 120);
        Thread.Sleep(8);
      }
      mouse_event(mouse_left_up, 0, 0, 0, UIntPtr.Zero);
    });
    thread.IsBackground = true;
    thread.Start();
    return thread;
  }
}
'@ -ReferencedAssemblies System.Drawing

$mouseLeftDown = 0x0002
$mouseLeftUp = 0x0004
$wmClose = 0x0010
$handCursorResource = 32649
$hwndTopmost = [IntPtr](-1)
$hwndNotTopmost = [IntPtr](-2)
$swpNoSize = 0x0001
$swpNoMove = 0x0002
$swpShowWindow = 0x0040
$gwlExStyle = -20
$wsExNoRedirectionBitmap = 0x00200000
$script:captureIndex = 0
if ($CaptureDirectory) {
    [void](New-Item -ItemType Directory -Force -Path $CaptureDirectory)
}

function Wait-ForMainWindow([Diagnostics.Process]$process) {
    for ($attempt = 0; $attempt -lt 150; ++$attempt) {
        Start-Sleep -Milliseconds 100
        $process.Refresh()
        if ($process.HasExited) {
            throw "Qt demo exited during startup with code $($process.ExitCode)."
        }
        if ($process.MainWindowHandle -ne 0) {
            return [IntPtr]$process.MainWindowHandle
        }
    }
    throw "Qt demo did not create a main window."
}

function Get-WindowRect([IntPtr]$window) {
    $rect = [LitheViewQtWindowTest+Rect]::new()
    if (-not [LitheViewQtWindowTest]::GetWindowRect($window, [ref]$rect)) {
        throw "Could not read window bounds."
    }
    return $rect
}

function Wait-ForSystemMoveSizeExit([IntPtr]$window) {
    $thread = [LitheViewQtWindowTest]::GetWindowThreadProcessId(
        $window, [IntPtr]::Zero)
    for ($attempt = 0; $attempt -lt 100; ++$attempt) {
        $info = [LitheViewQtWindowTest+GuiThreadInfo]::new()
        $info.size = [Runtime.InteropServices.Marshal]::SizeOf($info)
        if ([LitheViewQtWindowTest]::GetGUIThreadInfo($thread, [ref]$info) -and
            ($info.flags -band 0x0002) -eq 0) {
            Start-Sleep -Milliseconds 16
            return
        }
        Start-Sleep -Milliseconds 10
    }
    throw "Qt window did not leave the system move/size loop."
}

function Wait-ForForegroundWindow([IntPtr]$window) {
    [void][LitheViewQtWindowTest]::SetForegroundWindow($window)
    for ($attempt = 0; $attempt -lt 10; ++$attempt) {
        if ([LitheViewQtWindowTest]::GetForegroundWindow() -eq $window) {
            return
        }
        Start-Sleep -Milliseconds 100
    }

    # Windows can deny a programmatic foreground request. Put the window at
    # the top only while a title-bar click follows the user's activation path.
    $flags = $swpNoSize -bor $swpNoMove -bor $swpShowWindow
    [void][LitheViewQtWindowTest]::SetWindowPos(
        $window, $hwndTopmost, 0, 0, 0, 0, $flags)
    try {
        $rect = Get-WindowRect $window
        [void][LitheViewQtWindowTest]::SetCursorPos(
            [int](($rect.Left + $rect.Right) / 2), $rect.Top + 12)
        [LitheViewQtWindowTest]::mouse_event(
            $mouseLeftDown, 0, 0, 0, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 100
        [LitheViewQtWindowTest]::mouse_event(
            $mouseLeftUp, 0, 0, 0, [UIntPtr]::Zero)
        for ($attempt = 0; $attempt -lt 20; ++$attempt) {
            if ([LitheViewQtWindowTest]::GetForegroundWindow() -eq $window) {
                return
            }
            Start-Sleep -Milliseconds 100
        }
        throw ("Qt demo could not become the foreground window; current=" +
            "$([LitheViewQtWindowTest]::GetForegroundWindow()).")
    } finally {
        [void][LitheViewQtWindowTest]::SetWindowPos(
            $window, $hwndNotTopmost, 0, 0, 0, 0, $flags)
    }
}

function Get-BrowserSurface([IntPtr]$mainWindow) {
    $windows = [Collections.Generic.List[object]]::new()
    $callback = [LitheViewQtWindowTest+EnumWindowsCallback] {
        param([IntPtr]$window, [IntPtr]$state)
        $rect = [LitheViewQtWindowTest+Rect]::new()
        if ([LitheViewQtWindowTest]::GetWindowRect($window, [ref]$rect)) {
            $windows.Add([pscustomobject]@{
                Window = $window
                Width = $rect.Right - $rect.Left
                Height = $rect.Bottom - $rect.Top
                ExtendedStyle = [LitheViewQtWindowTest]::GetWindowLongPtr(
                    $window, $gwlExStyle).ToInt64()
            })
        }
        return $true
    }
    [void][LitheViewQtWindowTest]::EnumChildWindows(
        $mainWindow, $callback, [IntPtr]::Zero)
    $surface = $windows |
        Where-Object {
            $_.Width -gt 500 -and $_.Height -gt 400 -and
            ($_.ExtendedStyle -band $wsExNoRedirectionBitmap) -ne 0
        } |
        Sort-Object Height -Descending |
        Select-Object -First 1
    if (-not $surface) {
        throw "Qt demo browser surface was not found."
    }
    return $surface.Window
}

function Measure-ScreenFrame([LitheViewQtWindowTest+Rect]$rect) {
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    $bitmap = [Drawing.Bitmap]::new($width, $height)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen(
            $rect.Left, $rect.Top, 0, 0,
            [Drawing.Size]::new($width, $height),
            [Drawing.CopyPixelOperation]::SourceCopy)
        $visibleSamples = 0
        $sampleBytes = [Collections.Generic.List[byte]]::new()
        $colorBuckets = [Collections.Generic.HashSet[string]]::new()
        for ($y = 20; $y -lt $height - 20; $y += 40) {
            for ($x = 20; $x -lt $width - 20; $x += 40) {
                $pixel = $bitmap.GetPixel($x, $y)
                $sampleBytes.Add($pixel.R)
                $sampleBytes.Add($pixel.G)
                $sampleBytes.Add($pixel.B)
                [void]$colorBuckets.Add(("{0:X1}{1:X1}{2:X1}" -f
                    ($pixel.R -shr 5), ($pixel.G -shr 5),
                    ($pixel.B -shr 5)))
                if ([Math]::Max($pixel.R,
                        [Math]::Max($pixel.G, $pixel.B)) -ge 40) {
                    ++$visibleSamples
                }
            }
        }
        if ($CaptureDirectory -and $colorBuckets.Count -le 2) {
            ++$script:captureIndex
            $capturePath = Join-Path $CaptureDirectory (
                "low-color-{0:D3}-{1}x{2}.png" -f
                $script:captureIndex, $width, $height)
            $bitmap.Save($capturePath)
        }
        $sha256 = [Security.Cryptography.SHA256]::Create()
        try {
            $fingerprint = [BitConverter]::ToString(
                $sha256.ComputeHash($sampleBytes.ToArray())).Replace("-", "")
        } finally {
            $sha256.Dispose()
        }
        return [pscustomobject]@{
            VisibleSamples = $visibleSamples
            ColorBuckets = $colorBuckets.Count
            Fingerprint = $fingerprint
            Samples = $sampleBytes.ToArray()
        }
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Measure-SurfaceFrame([IntPtr]$window) {
    return Measure-ScreenFrame (Get-WindowRect $window)
}

function Measure-VisibleSamples([IntPtr]$window) {
    return (Measure-SurfaceFrame $window).VisibleSamples
}

function Wait-ForRichSurface([IntPtr]$window) {
    for ($attempt = 0; $attempt -lt 100; ++$attempt) {
        $frame = Measure-SurfaceFrame $window
        if ($frame.ColorBuckets -ge 3) {
            return $frame
        }
        Start-Sleep -Milliseconds 100
    }
    throw "Resize fixture did not become visually ready."
}

function Measure-FrameDifference($first, $second) {
    if ($first.Samples.Count -ne $second.Samples.Count) {
        throw "Frame sample dimensions changed during comparison."
    }
    [long]$difference = 0
    for ($index = 0; $index -lt $first.Samples.Count; ++$index) {
        $difference += [Math]::Abs(
            [int]$first.Samples[$index] - [int]$second.Samples[$index])
    }
    return $difference / [double]$first.Samples.Count
}

function Measure-FrameLayoutDifference($first, $second) {
    if ($first.Samples.Count -ne $second.Samples.Count) {
        throw "Frame sample dimensions changed during layout comparison."
    }
    $changedPixels = 0
    $pixelCount = $first.Samples.Count / 3
    for ($index = 0; $index -lt $first.Samples.Count; $index += 3) {
        $maximumChannelDifference = 0
        for ($channel = 0; $channel -lt 3; ++$channel) {
            $maximumChannelDifference = [Math]::Max(
                $maximumChannelDifference,
                [Math]::Abs([int]$first.Samples[$index + $channel] -
                    [int]$second.Samples[$index + $channel]))
        }
        if ($maximumChannelDifference -gt 32) {
            ++$changedPixels
        }
    }
    return $changedPixels / [double]$pixelCount
}

function Move-MouseWithButton([IntPtr]$surface,
                              [int]$startX, [int]$startY,
                              [int]$endX, [int]$endY,
                              [switch]$TrackSurfaceWithPointer,
                              [switch]$ConcurrentInput) {
    $surfaceStart = Get-WindowRect $surface
    [void][LitheViewQtWindowTest]::SetCursorPos($startX, $startY)
    $minimumVisibleSamples = [int]::MaxValue
    $minimumColorBuckets = [int]::MaxValue
    $maximumSurfacePositionError = 0
    $fingerprints = [Collections.Generic.HashSet[string]]::new()
    $lastInteractionFrame = $null
    $interactionSamples = 0
    if ($ConcurrentInput) {
        if ($TrackSurfaceWithPointer) {
            throw "Concurrent input does not support moving surface tracking."
        }
        $thread = [LitheViewQtWindowTest]::BeginMouseDrag(
            $startX, $startY, $endX, $endY,
            $mouseLeftDown, $mouseLeftUp)
        while ($thread.IsAlive) {
            $frame = Measure-SurfaceFrame $surface
            $minimumVisibleSamples = [Math]::Min(
                $minimumVisibleSamples, $frame.VisibleSamples)
            $minimumColorBuckets = [Math]::Min(
                $minimumColorBuckets, $frame.ColorBuckets)
            [void]$fingerprints.Add($frame.Fingerprint)
            $lastInteractionFrame = $frame
            ++$interactionSamples
            Start-Sleep -Milliseconds 1
        }
        $thread.Join()
    } else {
        [LitheViewQtWindowTest]::mouse_event(
            $mouseLeftDown, 0, 0, 0, [UIntPtr]::Zero)
        for ($step = 1; $step -le 15; ++$step) {
            $stepX = [int](($endX - $startX) * $step / 15)
            $stepY = [int](($endY - $startY) * $step / 15)
            [void][LitheViewQtWindowTest]::SetCursorPos(
                $startX + $stepX, $startY + $stepY)
            Start-Sleep -Milliseconds 50
            if ($TrackSurfaceWithPointer) {
                $expected = [LitheViewQtWindowTest+Rect]::new()
                $expected.Left = $surfaceStart.Left + $stepX
                $expected.Top = $surfaceStart.Top + $stepY
                $expected.Right = $surfaceStart.Right + $stepX
                $expected.Bottom = $surfaceStart.Bottom + $stepY
                $actual = Get-WindowRect $surface
                $maximumSurfacePositionError = [Math]::Max(
                    $maximumSurfacePositionError,
                    [Math]::Max([Math]::Abs($actual.Left - $expected.Left),
                        [Math]::Abs($actual.Top - $expected.Top)))
                $frame = Measure-ScreenFrame $expected
            } else {
                $frame = Measure-SurfaceFrame $surface
            }
            $minimumVisibleSamples = [Math]::Min(
                $minimumVisibleSamples, $frame.VisibleSamples)
            $minimumColorBuckets = [Math]::Min(
                $minimumColorBuckets, $frame.ColorBuckets)
            [void]$fingerprints.Add($frame.Fingerprint)
            $lastInteractionFrame = $frame
            ++$interactionSamples
        }
        [LitheViewQtWindowTest]::mouse_event(
            $mouseLeftUp, 0, 0, 0, [UIntPtr]::Zero)
    }
    Wait-ForSystemMoveSizeExit $surface
    $releaseMinimumColorBuckets = [int]::MaxValue
    $maximumReleaseFrameDifference = 0.0
    $maximumReleaseLayoutDifference = 0.0
    $releaseColorBuckets = [Collections.Generic.List[int]]::new()
    $releaseFrameDifferences = [Collections.Generic.List[double]]::new()
    $releaseReferenceFrame = Wait-ForRichSurface $surface
    for ($sample = 0; $sample -lt 24; ++$sample) {
        $releaseFrame = Measure-SurfaceFrame $surface
        $releaseColorBuckets.Add($releaseFrame.ColorBuckets)
        $releaseMinimumColorBuckets = [Math]::Min(
            $releaseMinimumColorBuckets, $releaseFrame.ColorBuckets)
        $releaseDifference =
            Measure-FrameDifference $releaseReferenceFrame $releaseFrame
        $releaseFrameDifferences.Add($releaseDifference)
        $maximumReleaseFrameDifference = [Math]::Max(
            $maximumReleaseFrameDifference, $releaseDifference)
        $maximumReleaseLayoutDifference = [Math]::Max(
            $maximumReleaseLayoutDifference,
            (Measure-FrameLayoutDifference $releaseReferenceFrame $releaseFrame))
        Start-Sleep -Milliseconds 16
    }
    return [pscustomobject]@{
        MinimumVisibleSamples = $minimumVisibleSamples
        MinimumColorBuckets = $minimumColorBuckets
        MaximumSurfacePositionError = $maximumSurfacePositionError
        ReleaseMinimumColorBuckets = $releaseMinimumColorBuckets
        MaximumReleaseFrameDifference = $maximumReleaseFrameDifference
        MaximumReleaseLayoutDifference = $maximumReleaseLayoutDifference
        ReleaseColorBuckets = $releaseColorBuckets.ToArray()
        ReleaseFrameDifferences = $releaseFrameDifferences.ToArray()
        DistinctFrames = $fingerprints.Count
        InteractionSamples = $interactionSamples
    }
}

$layoutPage = @'
<!doctype html><meta charset="utf-8"><title>LitheView Resize Regression</title>
<style>
*{box-sizing:border-box}html,body{margin:0;height:100%;font:16px Arial;background:#f5f7fa;color:#18202b}
header{height:96px;padding:24px 32px;background:#14213d;color:white}h1{margin:0 0 8px;font-size:24px}
main{min-height:calc(100% - 96px);padding:28px 32px;background:linear-gradient(90deg,#f5f7fa 0 25%,#e6f0ff 25% 50%,#fff0e6 50% 75%,#e8f7ee 75%)}.grid{display:grid;grid-template-columns:repeat(2,1fr);gap:18px}
a{color:inherit;text-decoration:none}article{height:180px;padding:22px;border:2px solid #263b63;background:white;box-shadow:inset 0 8px #2f80ed}
article:nth-child(2){box-shadow:inset 0 8px #27ae60}article:nth-child(3){box-shadow:inset 0 8px #eb5757}
article:nth-child(4){box-shadow:inset 0 8px #f2c94c}h2{margin:18px 0 12px;font-size:20px}
.bar{height:16px;margin-top:10px;background:#d7deea}.bar.short{width:58%;background:#7b8ca8}
@media(min-width:1100px){.grid{grid-template-columns:repeat(4,1fr)}article{height:360px}}
</style><header><h1>Native resize layout</h1><div>Deterministic visual continuity fixture</div></header>
<main><div class="grid"><a href="#render"><article><h2>Render</h2><div class="bar"></div><div class="bar short"></div></article></a>
<a href="#compose"><article><h2>Compose</h2><div class="bar"></div><div class="bar short"></div></article></a>
<a href="#present"><article><h2>Present</h2><div class="bar"></div><div class="bar short"></div></article></a>
<a href="#interact"><article><h2>Interact</h2><div class="bar"></div><div class="bar short"></div></article></a></div></main>
'@
$previousTestUrl = $env:LITHEVIEW_QT_DEMO_URL
$env:LITHEVIEW_QT_DEMO_URL =
    "data:text/html," + [Uri]::EscapeDataString($layoutPage)
$process = Start-Process -FilePath $Executable `
    -WorkingDirectory (Split-Path $Executable) -PassThru
$mainWindow = [IntPtr]::Zero
try {
    $mainWindow = Wait-ForMainWindow $process
    Start-Sleep -Seconds 4
    $process.Refresh()
    if ($process.MainWindowTitle -notmatch "LitheView Resize Regression") {
        throw "Resize test page did not load: $($process.MainWindowTitle)"
    }

    $surface = Get-BrowserSurface $mainWindow
    [void](Wait-ForRichSurface $surface)
    $surfaceExtendedStyle =
        [LitheViewQtWindowTest]::GetWindowLongPtr($surface, $gwlExStyle).ToInt64()
    if (($surfaceExtendedStyle -band $wsExNoRedirectionBitmap) -eq 0) {
        throw "LitheView accelerated HWND kept its GDI redirection bitmap."
    }
    $mainBefore = Get-WindowRect $mainWindow
    $surfaceBefore = Get-WindowRect $surface
    $resizeBaseline = Measure-SurfaceFrame $surface
    Wait-ForForegroundWindow $mainWindow
    $resizeX = $mainBefore.Right - 2
    $resizeY = [int](($mainBefore.Top + $mainBefore.Bottom) / 2)
    $resizeMetrics = Move-MouseWithButton `
        $surface $resizeX $resizeY ($resizeX + 160) $resizeY -ConcurrentInput
    $minimumVisibleSamples = $resizeMetrics.MinimumVisibleSamples
    Start-Sleep -Seconds 2

    $process.Refresh()
    $mainAfter = Get-WindowRect $mainWindow
    $surfaceAfter = Get-WindowRect $surface
    $mainGrowth = ($mainAfter.Right - $mainAfter.Left) -
        ($mainBefore.Right - $mainBefore.Left)
    $surfaceGrowth = ($surfaceAfter.Right - $surfaceAfter.Left) -
        ($surfaceBefore.Right - $surfaceBefore.Left)
    $finalVisibleSamples = Measure-VisibleSamples $surface
    $minimumExpectedResizeColorBuckets = [Math]::Max(
        2, [int]($resizeBaseline.ColorBuckets * 0.4))
    if (-not $process.Responding -or $mainGrowth -lt 100 -or
        [Math]::Abs($surfaceGrowth - $mainGrowth) -gt 2) {
        throw ("Resize did not keep the Qt surface and LitheView viewport " +
            "synchronized: main growth=$mainGrowth, surface growth=" +
            "$surfaceGrowth, responding=$($process.Responding), " +
            "main before=$($mainBefore.Left),$($mainBefore.Top)," +
            "$($mainBefore.Right),$($mainBefore.Bottom), main after=" +
            "$($mainAfter.Left),$($mainAfter.Top),$($mainAfter.Right)," +
            "$($mainAfter.Bottom).")
    }
    if ($minimumVisibleSamples -lt 20 -or $finalVisibleSamples -lt 20 -or
        $resizeMetrics.MinimumColorBuckets -lt
            $minimumExpectedResizeColorBuckets -or
        $resizeMetrics.ReleaseMinimumColorBuckets -lt
            $minimumExpectedResizeColorBuckets) {
        throw ("LitheView content became blank during live resize: baseline " +
            "color buckets=$($resizeBaseline.ColorBuckets), minimum=" +
            "$($resizeMetrics.MinimumColorBuckets), release minimum=" +
            "$($resizeMetrics.ReleaseMinimumColorBuckets), release buckets=" +
            "$($resizeMetrics.ReleaseColorBuckets -join ','), release " +
            "differences=$($resizeMetrics.ReleaseFrameDifferences -join ',').")
    }
    if ($resizeMetrics.MaximumReleaseLayoutDifference -gt 0.05) {
        throw ("LitheView layout jumped after the resize pointer was " +
            "released: changed sample ratio=" +
            "$($resizeMetrics.MaximumReleaseLayoutDifference), average " +
            "channel difference=" +
            "$($resizeMetrics.MaximumReleaseFrameDifference), release " +
            "buckets=$($resizeMetrics.ReleaseColorBuckets -join ','), " +
            "release differences=" +
            "$($resizeMetrics.ReleaseFrameDifferences -join ',').")
    }

    $moveMainBefore = Get-WindowRect $mainWindow
    $moveSurfaceBefore = Get-WindowRect $surface
    $moveBaseline = Measure-SurfaceFrame $surface
    $moveX = [int](($moveMainBefore.Left + $moveMainBefore.Right) / 2)
    $moveY = $moveMainBefore.Top + 12
    $moveMetrics = Move-MouseWithButton `
        $surface $moveX $moveY ($moveX + 180) ($moveY + 60) `
        -TrackSurfaceWithPointer
    $moveVisibleSamples = $moveMetrics.MinimumVisibleSamples
    Start-Sleep -Milliseconds 500
    $moveMainAfter = Get-WindowRect $mainWindow
    $moveSurfaceAfter = Get-WindowRect $surface
    $mainMoveX = $moveMainAfter.Left - $moveMainBefore.Left
    $surfaceMoveX = $moveSurfaceAfter.Left - $moveSurfaceBefore.Left
    $minimumExpectedColorBuckets = [Math]::Max(
        2, [int]($moveBaseline.ColorBuckets * 0.4))
    if ($mainMoveX -lt 100 -or [Math]::Abs($surfaceMoveX - $mainMoveX) -gt 2 -or
        $moveVisibleSamples -lt 20 -or $moveMetrics.DistinctFrames -lt 3 -or
        $moveMetrics.MinimumColorBuckets -lt $minimumExpectedColorBuckets -or
        $moveMetrics.ReleaseMinimumColorBuckets -lt
            $minimumExpectedColorBuckets) {
        throw ("Title drag did not keep the Qt and LitheView surfaces live: " +
            "main move=$mainMoveX, surface move=$surfaceMoveX, minimum " +
            "visible samples=$moveVisibleSamples, distinct frames=" +
            "$($moveMetrics.DistinctFrames), baseline color buckets=" +
            "$($moveBaseline.ColorBuckets), minimum color buckets=" +
            "$($moveMetrics.MinimumColorBuckets), maximum position error=" +
            "$($moveMetrics.MaximumSurfacePositionError), release minimum=" +
            "$($moveMetrics.ReleaseMinimumColorBuckets), release buckets=" +
            "$($moveMetrics.ReleaseColorBuckets -join ','), release " +
            "differences=" +
            "$($moveMetrics.ReleaseFrameDifferences -join ',').")
    }

    $handCursor = [LitheViewQtWindowTest]::LoadCursor(
        [IntPtr]::Zero, [IntPtr]$handCursorResource)
    $linkCursorFound = $false
    foreach ($point in @(
        @(50, 160),
        @(150, 240),
        @(150, 320)
    )) {
        [void][LitheViewQtWindowTest]::SetCursorPos(
            $moveSurfaceAfter.Left + $point[0],
            $moveSurfaceAfter.Top + $point[1])
        Start-Sleep -Milliseconds 300
        $cursorInfo = [LitheViewQtWindowTest+CursorInfo]::new()
        $cursorInfo.size =
            [Runtime.InteropServices.Marshal]::SizeOf($cursorInfo)
        if (-not [LitheViewQtWindowTest]::GetCursorInfo([ref]$cursorInfo)) {
            throw "Could not read the active cursor."
        }
        if ($cursorInfo.cursor -eq $handCursor) {
            $linkCursorFound = $true
            break
        }
    }
    if (-not $linkCursorFound) {
        throw "Web link cursor remained an arrow instead of becoming a hand."
    }

    [pscustomobject]@{
        Page = $process.MainWindowTitle
        NoRedirectionBitmap = $true
        MainWindowGrowth = $mainGrowth
        BrowserSurfaceGrowth = $surfaceGrowth
        MinimumVisibleSamples = $minimumVisibleSamples
        FinalVisibleSamples = $finalVisibleSamples
        ResizeMinimumColorBuckets = $resizeMetrics.MinimumColorBuckets
        ResizeInteractionSamples = $resizeMetrics.InteractionSamples
        ResizeReleaseColorBuckets = $resizeMetrics.ReleaseMinimumColorBuckets
        ResizeReleaseFrameDifference =
            $resizeMetrics.MaximumReleaseFrameDifference
        ResizeReleaseLayoutDifference =
            $resizeMetrics.MaximumReleaseLayoutDifference
        MainWindowMove = $mainMoveX
        BrowserSurfaceMove = $surfaceMoveX
        MoveVisibleSamples = $moveVisibleSamples
        MoveDistinctFrames = $moveMetrics.DistinctFrames
        MoveMinimumColorBuckets = $moveMetrics.MinimumColorBuckets
        MoveMaximumPositionError = $moveMetrics.MaximumSurfacePositionError
        MoveReleaseColorBuckets = $moveMetrics.ReleaseMinimumColorBuckets
        LinkCursor = "hand"
        Responsive = $process.Responding
    } | Format-List
} finally {
    if (-not $process.HasExited -and $mainWindow -ne [IntPtr]::Zero) {
        [void][LitheViewQtWindowTest]::PostMessage(
            $mainWindow, $wmClose, [IntPtr]::Zero, [IntPtr]::Zero)
        [void]$process.WaitForExit(10000)
    }
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
    }
    $env:LITHEVIEW_QT_DEMO_URL = $previousTestUrl
}
