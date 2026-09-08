param(
    [Parameter(Mandatory = $true)][string]$Header,
    [Parameter(Mandatory = $true)][string]$Map,
    [Parameter(Mandatory = $true)][string]$Output
)

$ErrorActionPreference = "Stop"
$ids = @{}
foreach ($line in [IO.File]::ReadLines($Header)) {
    if ($line -match '^#define\s+([A-Z0-9_]+)\s+.*?,\s*(\d+)\)\s*$') {
        $ids[$matches[1]] = [int]$matches[2]
    }
}

$entries = @{}
foreach ($line in [IO.File]::ReadLines($Map)) {
    if ($line -match '^\s*\{"([^"]+)",\s*([A-Z0-9_]+)') {
        if (-not $ids.ContainsKey($matches[2])) {
            throw "Resource id is missing for $($matches[2])."
        }
        if ($entries.ContainsKey($matches[1])) {
            throw "Duplicate DevTools resource path: $($matches[1])."
        }
        $entries[$matches[1]] = $ids[$matches[2]]
    }
}
if ($entries.Count -eq 0) {
    throw "The DevTools resource map is empty."
}

$paths = [string[]]$entries.Keys
[Array]::Sort($paths, [StringComparer]::Ordinal)
for ($index = 1; $index -lt $paths.Count; ++$index) {
    if ([StringComparer]::Ordinal.Compare(
            $paths[$index - 1], $paths[$index]) -ge 0) {
        throw "DevTools resource paths are not strictly ordinal-sorted."
    }
}
$lines = $paths | ForEach-Object {
    '  {"' + $_ + '", ' + $entries[$_] + '},'
}
[IO.File]::WriteAllLines($Output, $lines, [Text.UTF8Encoding]::new($false))
