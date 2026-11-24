param(
    [Parameter(Mandatory)] [string]$Iscc,
    [string]$Version = '0.1.0'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$module = 'warudo-spout-bridge'
$dll = Join-Path $root "build\Release\$module.dll"
$dist = Join-Path $root 'dist'
$null = New-Item -ItemType Directory -Force -Path $dist
$zip = Join-Path $dist "Spout2-iGPU-Capture-$Version-Portable-x64.zip"
$setup = Join-Path $dist "Spout2-iGPU-Capture-$Version-Setup-x64.exe"
& $Iscc "/DReleaseVersion=$Version" (Join-Path $PSScriptRoot 'installer.iss')
if ($LASTEXITCODE) { throw 'Installer compilation failed.' }

$temporaryZip = "$zip.$([guid]::NewGuid().ToString('N')).tmp"
$archive = [IO.Compression.ZipFile]::Open($temporaryZip, [IO.Compression.ZipArchiveMode]::Create)
try {
    $entries = [ordered]@{
        "$module.dll" = $dll
        'data/README.md' = Join-Path $root 'README.md'
        'data/GPL-2.0.txt' = Join-Path $root 'LICENSE'
        'data/Spout-LICENSE.txt' = Join-Path $root 'third_party\spout\LICENSE'
    }
    foreach ($entry in $entries.GetEnumerator()) {
        $null = [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $archive, $entry.Value, "plugins/$module/$($entry.Key)", [IO.Compression.CompressionLevel]::Optimal
        )
    }
} finally {
    $archive.Dispose()
}
[IO.File]::Move($temporaryZip, $zip, $true)

Get-FileHash -LiteralPath $setup,$zip | ForEach-Object {
    '{0}  {1}' -f $_.Hash.ToLowerInvariant(), (Split-Path -Leaf $_.Path)
} | Set-Content -LiteralPath (Join-Path $dist 'SHA256SUMS.txt') -Encoding ascii
