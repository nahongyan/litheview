[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ArchivePath,
    [string]$ChecksumPath = "$ArchivePath.sha256",
    [switch]$RequireSignature
)

$ErrorActionPreference = "Stop"
$archive = [IO.Path]::GetFullPath($ArchivePath)
$checksum = [IO.Path]::GetFullPath($ChecksumPath)
foreach ($path in @($archive, $checksum)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Release asset is missing: $path"
    }
}

$archiveName = [IO.Path]::GetFileName($archive)
if ($archiveName -notmatch '^litheview-sdk-([0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?)-win-x64\.zip$') {
    throw "Release archive name is invalid: $archiveName"
}
$version = $Matches[1]

$checksumLine = (Get-Content -Raw -LiteralPath $checksum).Trim()
if ($checksumLine -notmatch '^([0-9a-f]{64})  (.+\.zip)$') {
    throw "Release checksum file has an invalid format."
}
$expectedArchiveHash = $Matches[1]
$expectedArchiveName = $Matches[2]
$actualArchiveHash = (Get-FileHash -Algorithm SHA256 `
    -LiteralPath $archive).Hash.ToLowerInvariant()
if ($archiveName -ne $expectedArchiveName -or
    $actualArchiveHash -ne $expectedArchiveHash) {
    throw "Release archive checksum does not match."
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    foreach ($entry in $zip.Entries) {
        $normalized = $entry.FullName -replace '\\', '/'
        if ($normalized.StartsWith('/') -or
            $normalized -match '(^|/)\.\.(/|$)' -or
            [IO.Path]::IsPathRooted($normalized)) {
            throw "Release archive contains an unsafe path: $($entry.FullName)"
        }
    }
} finally {
    $zip.Dispose()
}

$extractRoot = Join-Path ([IO.Path]::GetTempPath()) `
    ("litheview-release-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $extractRoot | Out-Null
try {
    [IO.Compression.ZipFile]::ExtractToDirectory($archive, $extractRoot)

    $required = @(
        "LICENSE", "NOTICE", "README.md", "RUNTIME-LICENSE.md",
        "THIRD_PARTY_NOTICES.txt", "SBOM.spdx.json", "VERSION",
        "manifest.json", "SHA256SUMS.txt", "bin\litheview.dll",
        "lib\litheview.lib", "include\litheview\litheview.h"
    )
    foreach ($relative in $required) {
        if (-not (Test-Path -LiteralPath (Join-Path $extractRoot $relative) -PathType Leaf)) {
            throw "Release package is missing: $relative"
        }
    }

    $packagedVersion = (Get-Content -Raw -LiteralPath `
        (Join-Path $extractRoot "VERSION")).Trim()
    if ($packagedVersion -ne $version) {
        throw "Archive version does not match VERSION."
    }

    $manifest = Get-Content -Raw -Encoding utf8 -LiteralPath `
        (Join-Path $extractRoot "manifest.json") | ConvertFrom-Json
    if ($manifest.version -ne $version -or $manifest.platform -ne "win-x64") {
        throw "Archive name does not match manifest.json."
    }

    if ($RequireSignature) {
        $binaries = Get-ChildItem -LiteralPath (Join-Path $extractRoot "bin") `
            -File | Where-Object Extension -in @(".dll", ".exe")
        foreach ($binary in $binaries) {
            $signature = Get-AuthenticodeSignature -LiteralPath $binary.FullName
            if ($signature.Status -ne "Valid") {
                throw "Release binary does not have a valid Authenticode signature: $($binary.Name) ($($signature.Status))"
            }
        }
    }

    $notices = Get-Content -Raw -Encoding utf8 -LiteralPath `
        (Join-Path $extractRoot "THIRD_PARTY_NOTICES.txt")
    if ($notices.Length -lt 10000 -or
        $notices -notmatch 'License notice for The Chromium Project') {
        throw "Third-party notices are missing or incomplete."
    }

    $sbom = Get-Content -Raw -Encoding utf8 -LiteralPath `
        (Join-Path $extractRoot "SBOM.spdx.json") | ConvertFrom-Json
    if ($sbom.spdxVersion -ne "SPDX-2.2" -or
        -not ($sbom.packages.name -contains "Chromium")) {
        throw "SPDX SBOM is missing or invalid."
    }

    $expectedHashes = @{}
    foreach ($line in Get-Content -LiteralPath `
        (Join-Path $extractRoot "SHA256SUMS.txt")) {
        if ($line -notmatch '^([0-9a-f]{64})  (.+)$') {
            throw "Invalid package checksum entry: $line"
        }
        $expectedHashes[$Matches[2]] = $Matches[1]
    }
    $files = Get-ChildItem -LiteralPath $extractRoot -Recurse -File |
        Where-Object Name -ne "SHA256SUMS.txt"
    foreach ($file in $files) {
        $relative = $file.FullName.Substring($extractRoot.Length + 1) `
            -replace '\\', '/'
        if (-not $expectedHashes.ContainsKey($relative)) {
            throw "Package checksum is missing for: $relative"
        }
        $actual = (Get-FileHash -Algorithm SHA256 `
            -LiteralPath $file.FullName).Hash.ToLowerInvariant()
        if ($actual -ne $expectedHashes[$relative]) {
            throw "Package checksum does not match: $relative"
        }
    }
    if ($expectedHashes.Count -ne $files.Count) {
        throw "Package checksum list contains files not present in the archive."
    }
} finally {
    if (Test-Path -LiteralPath $extractRoot) {
        Remove-Item -LiteralPath $extractRoot -Recurse -Force
    }
}

Write-Host "LitheView Release archive $archiveName is valid."
