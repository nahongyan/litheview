param(
    [string]$Executable = (Join-Path $PSScriptRoot '..\..\..\..\out\wr-minimal\litheview_demo.exe')
)

$ErrorActionPreference = 'Stop'

Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class LitheViewAddressBarTest {
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

  [DllImport("user32.dll", EntryPoint = "GetWindowTextLengthW",
      ExactSpelling = true)]
  public static extern int GetWindowTextLength(IntPtr window);

  [DllImport("user32.dll", CharSet = CharSet.Unicode)]
  public static extern bool SetWindowText(IntPtr window, string text);

  [DllImport("user32.dll")]
  public static extern IntPtr SendMessage(
      IntPtr window, uint message, IntPtr wparam, IntPtr lparam);

  [DllImport("user32.dll")]
  public static extern bool SetForegroundWindow(IntPtr window);

  [DllImport("user32.dll")]
  public static extern IntPtr SetFocus(IntPtr window);

  [DllImport("user32.dll")]
  public static extern uint GetWindowThreadProcessId(
      IntPtr window, out uint processId);

  [DllImport("kernel32.dll")]
  public static extern uint GetCurrentThreadId();

  [DllImport("user32.dll")]
  public static extern bool AttachThreadInput(
      uint idAttach, uint idAttachTo, bool attach);

  [DllImport("user32.dll")]
  public static extern void keybd_event(
      byte virtualKey, byte scanCode, uint flags, UIntPtr extraInfo);
}
'@

$addressControlId = 1001
$emSetSel = 0x00B1
$emGetSel = 0x00B0
$keyUp = 0x0002
$vkControl = 0x11
$vkA = 0x41
$testUrl = 'https://example.test/address-bar-selection'

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
$process = Start-Process -FilePath $Executable -ArgumentList 'about:blank' -PassThru
try {
    $mainWindow = Wait-ForMainWindow $process
    Start-Sleep -Seconds 5
    $address = [LitheViewAddressBarTest]::GetDlgItem(
        $mainWindow, $addressControlId)
    if ($address -eq [IntPtr]::Zero) {
        throw 'The address bar was not created.'
    }

    if (-not [LitheViewAddressBarTest]::SetWindowText($address, $testUrl)) {
        throw 'Failed to populate the address bar.'
    }
    Start-Sleep -Milliseconds 500
    $length = $testUrl.Length
    $actualLength = [LitheViewAddressBarTest]::GetWindowTextLength($address)
    if ($actualLength -ne $length) {
        throw "The address bar changed before input ($actualLength chars)."
    }
    [void][LitheViewAddressBarTest]::SendMessage(
        $address, $emSetSel, [IntPtr]$length, [IntPtr]$length)

    $targetProcessId = 0
    $targetThread = [LitheViewAddressBarTest]::GetWindowThreadProcessId(
        $address, [ref]$targetProcessId)
    $currentThread = [LitheViewAddressBarTest]::GetCurrentThreadId()
    if (-not [LitheViewAddressBarTest]::AttachThreadInput(
            $currentThread, $targetThread, $true)) {
        throw 'Could not attach to the demo input thread.'
    }
    try {
        [void][LitheViewAddressBarTest]::SetForegroundWindow($mainWindow)
        [void][LitheViewAddressBarTest]::SetFocus($address)
        [void][LitheViewAddressBarTest]::SendMessage(
            $address, $emSetSel, [IntPtr]$length, [IntPtr]$length)
        [LitheViewAddressBarTest]::keybd_event(
            $vkControl, 0, 0, [UIntPtr]::Zero)
        [LitheViewAddressBarTest]::keybd_event(
            $vkA, 0, 0, [UIntPtr]::Zero)
        [LitheViewAddressBarTest]::keybd_event(
            $vkA, 0, $keyUp, [UIntPtr]::Zero)
        [LitheViewAddressBarTest]::keybd_event(
            $vkControl, 0, $keyUp, [UIntPtr]::Zero)
        Start-Sleep -Milliseconds 200
    } finally {
        [void][LitheViewAddressBarTest]::AttachThreadInput(
            $currentThread, $targetThread, $false)
    }

    $currentLength = [LitheViewAddressBarTest]::GetWindowTextLength($address)
    $selectionStart = [Runtime.InteropServices.Marshal]::AllocHGlobal(4)
    $selectionEnd = [Runtime.InteropServices.Marshal]::AllocHGlobal(4)
    try {
        [void][LitheViewAddressBarTest]::SendMessage(
            $address, $emGetSel, $selectionStart, $selectionEnd)
        $start = [Runtime.InteropServices.Marshal]::ReadInt32($selectionStart)
        $end = [Runtime.InteropServices.Marshal]::ReadInt32($selectionEnd)
    } finally {
        [Runtime.InteropServices.Marshal]::FreeHGlobal($selectionStart)
        [Runtime.InteropServices.Marshal]::FreeHGlobal($selectionEnd)
    }

    [pscustomobject]@{
        AddressLength = $currentLength
        SelectionStart = $start
        SelectionEnd = $end
    } | Format-List
    if ($currentLength -le 1) {
        throw "The address bar became empty ($currentLength chars)."
    }
    if ($start -ne 0 -or $end -ne $currentLength) {
        throw "Ctrl+A selected [$start,$end), expected [0,$currentLength)."
    }
} finally {
    Stop-NewDemoProcesses $baselineProcessIds
}
