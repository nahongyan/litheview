param(
    [string]$Executable = (Join-Path $PSScriptRoot '..\..\..\..\out\wr-minimal\litheview_demo.exe'),
    [switch]$WebGlOnly
)

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;

public static class LitheViewDemoWindowTest {
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
  public static extern int GetWindowText(
      IntPtr window, StringBuilder text, int count);

  [DllImport("user32.dll")]
  public static extern IntPtr SendMessage(
      IntPtr window, uint message, IntPtr wparam, IntPtr lparam);

  [DllImport("user32.dll")]
  public static extern bool SetForegroundWindow(IntPtr window);

  [DllImport("user32.dll")]
  public static extern bool SetCursorPos(int x, int y);

  [DllImport("user32.dll")]
  public static extern void mouse_event(
      uint flags, uint x, uint y, uint data, UIntPtr extra_info);
}
'@

$wmCommand = 0x0111
$gpuOutputPageCommand = 2002
$webGlPageCommand = 2003
$mouseLeftDown = 0x0002
$mouseLeftUp = 0x0004
$addressControlId = 1001
$renderSurfaceControlId = 1003
$expectedNavigationHeight = 20

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
    $rect = [LitheViewDemoWindowTest+Rect]::new()
    if (-not [LitheViewDemoWindowTest]::GetWindowRect($window, [ref]$rect)) {
        throw 'GetWindowRect failed.'
    }
    return $rect
}

function Move-MouseWithButton([int]$startX, [int]$startY,
                              [int]$endX, [int]$endY) {
    [void][LitheViewDemoWindowTest]::SetCursorPos($startX, $startY)
    [LitheViewDemoWindowTest]::mouse_event(
        $mouseLeftDown, 0, 0, 0, [UIntPtr]::Zero)
    for ($step = 1; $step -le 15; ++$step) {
        $x = $startX + [int](($endX - $startX) * $step / 15)
        $y = $startY + [int](($endY - $startY) * $step / 15)
        [void][LitheViewDemoWindowTest]::SetCursorPos($x, $y)
        Start-Sleep -Milliseconds 40
    }
    [LitheViewDemoWindowTest]::mouse_event(
        $mouseLeftUp, 0, 0, 0, [UIntPtr]::Zero)
}

function Stop-NewDemoProcesses([int[]]$baselineProcessIds) {
    $processes = @(Get-Process litheview_demo -ErrorAction SilentlyContinue |
        Where-Object { $_.Id -notin $baselineProcessIds })
    $processes | Stop-Process -Force -ErrorAction SilentlyContinue
    $processes | Wait-Process -Timeout 5 -ErrorAction SilentlyContinue
}

function Measure-WebGlColorPixels([IntPtr]$mainWindow) {
    $surface = [LitheViewDemoWindowTest]::GetDlgItem(
        $mainWindow, $renderSurfaceControlId)
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
        $colorPixels = 0
        for ($y = [int]($height * 0.25); $y -lt [int]($height * 0.75);
             $y += 10) {
            for ($x = [int]($width * 0.25); $x -lt [int]($width * 0.75);
                 $x += 10) {
                $pixel = $bitmap.GetPixel($x, $y)
                $maximum = [Math]::Max($pixel.R, [Math]::Max($pixel.G, $pixel.B))
                $minimum = [Math]::Min($pixel.R, [Math]::Min($pixel.G, $pixel.B))
                if ($maximum -ge 70 -and $maximum - $minimum -ge 20) {
                    ++$colorPixels
                }
            }
        }
        return $colorPixels
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Invoke-Scenario([string]$name, [scriptblock]$drag) {
    $baselineProcessIds = @(
        Get-Process litheview_demo -ErrorAction SilentlyContinue |
            ForEach-Object Id)
    $process = Start-Process -FilePath $Executable -PassThru
    try {
        $mainWindow = Wait-ForMainWindow $process
        Start-Sleep -Seconds 2
        [void][LitheViewDemoWindowTest]::SendMessage(
            $mainWindow, $wmCommand, [IntPtr]$gpuOutputPageCommand,
            [IntPtr]::Zero)
        Start-Sleep -Seconds 2

        $before = Get-WindowRect $mainWindow
        [void][LitheViewDemoWindowTest]::SetForegroundWindow($mainWindow)
        & $drag $before
        Start-Sleep -Seconds 2

        $process.Refresh()
        $after = Get-WindowRect $mainWindow
        $address = [LitheViewDemoWindowTest]::GetDlgItem(
            $mainWindow, $addressControlId)
        $addressRect = Get-WindowRect $address
        $navigationHeight = $addressRect.Bottom - $addressRect.Top

        if (-not $process.Responding) {
            throw "$name left the demo unresponsive."
        }
        if ($navigationHeight -lt $expectedNavigationHeight) {
            throw "$name collapsed the navigation bar to ${navigationHeight}px."
        }
        return [pscustomobject]@{
            Scenario = $name
            Responsive = $process.Responding
            NavigationHeight = $navigationHeight
            Before = "$($before.Left),$($before.Top)-$($before.Right),$($before.Bottom)"
            After = "$($after.Left),$($after.Top)-$($after.Right),$($after.Bottom)"
        }
    } finally {
        Stop-NewDemoProcesses $baselineProcessIds
    }
}

function Invoke-WebGlScenario {
    $baselineProcessIds = @(
        Get-Process litheview_demo -ErrorAction SilentlyContinue |
            ForEach-Object Id)
    $process = Start-Process -FilePath $Executable -PassThru
    try {
        $mainWindow = Wait-ForMainWindow $process
        Start-Sleep -Seconds 2
        [void][LitheViewDemoWindowTest]::SendMessage(
            $mainWindow, $wmCommand, [IntPtr]$gpuOutputPageCommand,
            [IntPtr]::Zero)
        Start-Sleep -Seconds 3
        [void][LitheViewDemoWindowTest]::SendMessage(
            $mainWindow, $wmCommand, [IntPtr]$webGlPageCommand,
            [IntPtr]::Zero)
        Start-Sleep -Seconds 5

        $process.Refresh()
        $title = [Text.StringBuilder]::new(256)
        [void][LitheViewDemoWindowTest]::GetWindowText(
            $mainWindow, $title, $title.Capacity)
        if ($process.HasExited) {
            throw 'GPU output to WebGL exited the demo.'
        }
        if (-not $process.Responding) {
            throw 'GPU output to WebGL left the demo unresponsive.'
        }
        if ($title.ToString() -notmatch 'WebGL 3D') {
            throw "GPU output to WebGL did not finish loading: $title"
        }
        $colorPixels = Measure-WebGlColorPixels $mainWindow
        if ($colorPixels -lt 10) {
            throw "WebGL canvas remained blank ($colorPixels colored samples)."
        }
        return [pscustomobject]@{
            Scenario = 'GPU output to WebGL'
            Responsive = $process.Responding
            NavigationHeight = '-'
            Before = '-'
            After = "$title ($colorPixels colored samples)"
        }
    } finally {
        Stop-NewDemoProcesses $baselineProcessIds
    }
}

$results = @()
$failures = @()

if (-not $WebGlOnly) {
    try {
        $results += Invoke-Scenario 'title move' {
            param($rect)
            $x = [int](($rect.Left + $rect.Right) / 2)
            $y = $rect.Top + 12
            Move-MouseWithButton $x $y ($x + 105) ($y + 30)
        }
    } catch {
        $failures += $_.Exception.Message
    }

    try {
        $results += Invoke-Scenario 'border resize' {
            param($rect)
            $x = $rect.Right - 2
            $y = [int](($rect.Top + $rect.Bottom) / 2)
            Move-MouseWithButton $x $y ($x + 105) $y
        }
    } catch {
        $failures += $_.Exception.Message
    }
}

try {
    $results += Invoke-WebGlScenario
} catch {
    $failures += $_.Exception.Message
}

$results | Format-Table -AutoSize
if ($failures.Count -ne 0) {
    throw ($failures -join [Environment]::NewLine)
}
