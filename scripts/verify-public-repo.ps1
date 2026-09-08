[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot

$required = @(
    "LICENSE",
    "NOTICE",
    "README.md",
    "RUNTIME-LICENSE.md",
    "THIRD_PARTY_NOTICES.md",
    "TRADEMARKS.md",
    "CONTRIBUTING.md",
    "CODE_OF_CONDUCT.md",
    "SECURITY.md",
    "SUPPORT.md",
    "SPONSORS.md",
    "CHANGELOG.md",
    "RELEASING.md",
    "scripts\verify-release-package.ps1",
    "include\litheview\litheview.h",
    "examples\litheview_demo\CMakeLists.txt",
    "examples\litheview_qt_demo\CMakeLists.txt"
)
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $repoRoot $relative) -PathType Leaf)) {
        throw "Required public repository file is missing: $relative"
    }
}

$tracked = @(& git -C $repoRoot ls-files -- .)
if ($LASTEXITCODE -ne 0) {
    throw "Could not enumerate tracked files."
}

$forbiddenPatterns = @(
    '(^|/)(build|out|CMakeFiles|\.qtcreator|\.vs)(/|$)',
    '\.(dll|exe|lib|pdb|zip|7z)$'
)
foreach ($file in $tracked) {
    $normalized = $file -replace '\\', '/'
    foreach ($pattern in $forbiddenPatterns) {
        if ($normalized -match $pattern) {
            throw "Generated or binary file must not be committed: $file"
        }
    }
}

$publicText = Get-Content -Raw -Encoding utf8 -LiteralPath `
    (Join-Path $repoRoot "README.md")
if ($publicText -notmatch [regex]::Escape("1061517027@qq.com")) {
    throw "README.md does not contain the current public contact address."
}

$runtimeLicense = Get-Content -Raw -Encoding utf8 -LiteralPath `
    (Join-Path $repoRoot "RUNTIME-LICENSE.md")
foreach ($requiredSection in @(2, 3, 4, 7, 9, 10, 11)) {
    $heading = "## $requiredSection."
    if ($runtimeLicense -notmatch [regex]::Escape($heading)) {
        throw "Runtime license is missing required section: $requiredSection"
    }
}

Write-Host "Public SDK repository policy and tracked content are valid."
