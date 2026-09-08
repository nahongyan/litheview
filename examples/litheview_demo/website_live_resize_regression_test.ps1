param(
    [string]$Executable = (Join-Path $PSScriptRoot '..\..\..\..\out\wr-minimal\litheview_demo.exe'),
    [string]$Url = 'http://127.0.0.1:3000/#capabilities'
)

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class LitheViewWebsiteResizeTest {
  [StructLayout(LayoutKind.Sequential)]
  public struct Rect {
    public int Left;
    public int Top;
    public int Right;
    public int Bottom;
  }

  [DllImport("user32.dll")]
  public static extern IntPtr GetDlgItem(IntPtr parent, int id);

  [DllImport("user32.dll")]
  public static extern bool GetWindowRect(IntPtr window, out Rect rect);

  [DllImport("user32.dll")]
  public static extern bool SetForegroundWindow(IntPtr window);

  [DllImport("user32.dll")]
  public static extern bool SetCursorPos(int x, int y);

  [DllImport("user32.dll")]
  public static extern void mouse_event(
      uint flags, uint x, uint y, uint data, UIntPtr extraInfo);
}
'@

$renderSurfaceControlId = 1003
$mouseLeftDown = 0x0002
$mouseLeftUp = 0x0004

function Wait-ForMainWindow([Diagnostics.Process]$process) {
    for ($attempt = 0; $attempt -lt 100; ++$attempt) {
        $process.Refresh()
        if ($process.MainWindowHandle -ne 0) {
            return [IntPtr]$process.MainWindowHandle
        }
        Start-Sleep -Milliseconds 100
    }
    throw 'The demo did not create its main window.'
}

function Get-WindowRect([IntPtr]$window) {
    $rect = [LitheViewWebsiteResizeTest+Rect]::new()
    if (-not [LitheViewWebsiteResizeTest]::GetWindowRect(
            $window, [ref]$rect)) {
        throw 'GetWindowRect failed.'
    }
    return $rect
}

function Measure-SurfaceContent([IntPtr]$surface) {
    $rect = Get-WindowRect $surface
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    $bitmap = [Drawing.Bitmap]::new($width, $height)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen(
            $rect.Left, $rect.Top, 0, 0,
            [Drawing.Size]::new($width, $height),
            [Drawing.CopyPixelOperation]::SourceCopy)
        $contentPixels = 0
        for ($y = 16; $y -lt $height - 16; $y += 8) {
            for ($x = 16; $x -lt $width - 16; $x += 8) {
                $pixel = $bitmap.GetPixel($x, $y)
                $maximum = [Math]::Max(
                    $pixel.R, [Math]::Max($pixel.G, $pixel.B))
                $minimum = [Math]::Min(
                    $pixel.R, [Math]::Min($pixel.G, $pixel.B))
                if ($maximum - $minimum -ge 18 -or $maximum -le 215) {
                    ++$contentPixels
                }
            }
        }
        return $contentPixels
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Stop-NewDemoProcesses([int[]]$baselineProcessIds) {
    $processes = @(Get-Process litheview_demo -ErrorAction SilentlyContinue |
        Where-Object { $_.Id -notin $baselineProcessIds })
    $processes | Stop-Process -Force -ErrorAction SilentlyContinue
    $processes | Wait-Process -Timeout 5 -ErrorAction SilentlyContinue
}

if (-not (Test-Path -LiteralPath $Executable)) {
    throw "Demo executable not found: $Executable"
}

$baselineProcessIds = @(
    Get-Process litheview_demo -ErrorAction SilentlyContinue |
        ForEach-Object Id)
$process = Start-Process -FilePath $Executable -ArgumentList $Url -PassThru
try {
    $mainWindow = Wait-ForMainWindow $process
    Start-Sleep -Seconds 6
    $surface = [LitheViewWebsiteResizeTest]::GetDlgItem(
        $mainWindow, $renderSurfaceControlId)
    if ($surface -eq [IntPtr]::Zero) {
        throw 'The render surface was not created.'
    }

    $baseline = Measure-SurfaceContent $surface
    if ($baseline -lt 500) {
        throw "The website 3D baseline is blank ($baseline content samples)."
    }

    $windowRect = Get-WindowRect $mainWindow
    $startX = $windowRect.Right - 2
    $startY = [int](($windowRect.Top + $windowRect.Bottom) / 2)
    [void][LitheViewWebsiteResizeTest]::SetForegroundWindow($mainWindow)
    [void][LitheViewWebsiteResizeTest]::SetCursorPos($startX, $startY)
    [LitheViewWebsiteResizeTest]::mouse_event(
        $mouseLeftDown, 0, 0, 0, [UIntPtr]::Zero)

    $samples = @()
    try {
        for ($step = 1; $step -le 24; ++$step) {
            $x = $startX + $step * 6
            [void][LitheViewWebsiteResizeTest]::SetCursorPos($x, $startY)
            $samples += Measure-SurfaceContent $surface
            Start-Sleep -Milliseconds 12
        }
    } finally {
        [LitheViewWebsiteResizeTest]::mouse_event(
            $mouseLeftUp, 0, 0, 0, [UIntPtr]::Zero)
    }

    $process.Refresh()
    if (-not $process.Responding) {
        throw 'Live resize left the demo unresponsive.'
    }
    $minimum = ($samples | Measure-Object -Minimum).Minimum
    $threshold = [Math]::Max(500, [int]($baseline * 0.45))
    [pscustomobject]@{
        BaselineContentSamples = $baseline
        MinimumLiveContentSamples = $minimum
        RequiredMinimum = $threshold
        Samples = $samples -join ','
    } | Format-List
    if ($minimum -lt $threshold) {
        throw "Live resize discarded the previous frame: $minimum < $threshold."
    }
} finally {
    Stop-NewDemoProcesses $baselineProcessIds
}
