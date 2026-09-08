param(
    [string]$Executable = (Join-Path $PSScriptRoot '..\..\..\..\out\wr-minimal\litheview_demo.exe')
)

$ErrorActionPreference = 'Stop'

Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;

public static class LitheViewWebShortcutTest {
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

  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern int GetWindowText(
      IntPtr window, StringBuilder text, int count);

  [DllImport("user32.dll")]
  public static extern bool SetForegroundWindow(IntPtr window);

  [DllImport("user32.dll")]
  public static extern bool SetCursorPos(int x, int y);

  [DllImport("user32.dll")]
  public static extern void mouse_event(
      uint flags, uint x, uint y, uint data, UIntPtr extraInfo);

  [DllImport("user32.dll")]
  public static extern void keybd_event(
      byte virtualKey, byte scanCode, uint flags, UIntPtr extraInfo);
}
'@

$renderSurfaceControlId = 1003
$mouseLeftDown = 0x0002
$mouseLeftUp = 0x0004
$keyUp = 0x0002
$vkControl = 0x11

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

function Get-WindowTitle([IntPtr]$window) {
    $title = [Text.StringBuilder]::new(512)
    [void][LitheViewWebShortcutTest]::GetWindowText(
        $window, $title, $title.Capacity)
    return $title.ToString()
}

function Wait-ForTitle([IntPtr]$window, [string]$expected) {
    for ($attempt = 0; $attempt -lt 100; ++$attempt) {
        $title = Get-WindowTitle $window
        if ($title.EndsWith($expected, [StringComparison]::Ordinal)) {
            return
        }
        Start-Sleep -Milliseconds 100
    }
    throw "Window title did not become '$expected': $(Get-WindowTitle $window)"
}

function Send-ControlKey([int]$virtualKey) {
    [LitheViewWebShortcutTest]::keybd_event(
        $vkControl, 0, 0, [UIntPtr]::Zero)
    [LitheViewWebShortcutTest]::keybd_event(
        $virtualKey, 0, 0, [UIntPtr]::Zero)
    [LitheViewWebShortcutTest]::keybd_event(
        $virtualKey, 0, $keyUp, [UIntPtr]::Zero)
    [LitheViewWebShortcutTest]::keybd_event(
        $vkControl, 0, $keyUp, [UIntPtr]::Zero)
}

function Send-Key([int]$virtualKey) {
    [LitheViewWebShortcutTest]::keybd_event(
        $virtualKey, 0, 0, [UIntPtr]::Zero)
    [LitheViewWebShortcutTest]::keybd_event(
        $virtualKey, 0, $keyUp, [UIntPtr]::Zero)
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

$html = @'
<!doctype html><meta charset="utf-8"><title>shortcut-ready</title>
<style>html,body{margin:0;background:white}input{position:fixed;left:20px;top:20px;width:420px;height:40px;font:18px sans-serif}</style>
<input id="target" value="abcdef" autofocus>
<script>
target.addEventListener('select',()=>document.title=`selection:${target.selectionStart}-${target.selectionEnd}`);
target.addEventListener('input',()=>document.title=`value:${target.value}`);
target.addEventListener('keydown',event=>{if(event.ctrlKey)document.title=`key:${event.key}:${event.code}`});
target.addEventListener('copy',()=>document.title='copy-event');
target.addEventListener('paste',()=>document.title='paste-event');
</script>
'@
$url = 'data:text/html;charset=utf-8,' + [Uri]::EscapeDataString($html)
$baselineProcessIds = @(
    Get-Process litheview_demo -ErrorAction SilentlyContinue |
        ForEach-Object Id)
$process = Start-Process -FilePath $Executable -ArgumentList $url -PassThru
try {
    $mainWindow = Wait-ForMainWindow $process
    Wait-ForTitle $mainWindow 'shortcut-ready'
    $surface = [LitheViewWebShortcutTest]::GetDlgItem(
        $mainWindow, $renderSurfaceControlId)
    if ($surface -eq [IntPtr]::Zero) {
        throw 'The render surface was not created.'
    }
    $rect = [LitheViewWebShortcutTest+Rect]::new()
    if (-not [LitheViewWebShortcutTest]::GetWindowRect($surface, [ref]$rect)) {
        throw 'GetWindowRect failed for the render surface.'
    }

    [void][LitheViewWebShortcutTest]::SetForegroundWindow($mainWindow)
    [void][LitheViewWebShortcutTest]::SetCursorPos(
        $rect.Left + 100, $rect.Top + 40)
    [LitheViewWebShortcutTest]::mouse_event(
        $mouseLeftDown, 0, 0, 0, [UIntPtr]::Zero)
    [LitheViewWebShortcutTest]::mouse_event(
        $mouseLeftUp, 0, 0, 0, [UIntPtr]::Zero)

    Send-ControlKey ([byte][char]'A')
    Wait-ForTitle $mainWindow 'selection:0-6'

    Send-ControlKey ([byte][char]'C')
    Start-Sleep -Milliseconds 250
    $copyTitle = Get-WindowTitle $mainWindow
    Send-Key ([byte][char]'X')
    Wait-ForTitle $mainWindow 'value:x'
    Send-ControlKey ([byte][char]'V')
    Wait-ForTitle $mainWindow 'value:xabcdef'
    $pasteTitle = Get-WindowTitle $mainWindow

    [pscustomobject]@{
        SelectAll = 'selection:0-6'
        CopyTitle = $copyTitle
        PasteTitle = $pasteTitle
        PastedValue = 'xabcdef'
        Responsive = $process.Responding
    } | Format-List
} finally {
    Stop-NewDemoProcesses $baselineProcessIds
}
