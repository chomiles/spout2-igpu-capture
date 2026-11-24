param([string]$ObsBin = (Join-Path $env:ProgramFiles 'obs-studio\bin\64bit'))

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
Push-Location $PSScriptRoot
try {
    $obsVersion = '33.0.3'
    $obsCommit = 'f2aa9ce69caa7239ef26660b5889de4f53d9880e'
    $obsDll = Join-Path $ObsBin 'obs.dll'
    if (!(Test-Path -LiteralPath $obsDll) -or
        (Get-Item -LiteralPath $obsDll).VersionInfo.FileVersion -ne $obsVersion) {
        throw "Point -ObsBin to the bin\64bit folder of OBS Studio $obsVersion."
    }

    if (!(Test-Path -LiteralPath 'deps/obs/libobs/obs-module.h')) {
        git clone --depth 1 --filter=blob:none --sparse --branch $obsVersion https://github.com/obsproject/obs-studio.git deps/obs
        if ($LASTEXITCODE) { throw 'Could not download OBS headers.' }
        if ((git -C deps/obs rev-parse HEAD) -ne $obsCommit) { throw 'Unexpected OBS revision.' }
        git -C deps/obs sparse-checkout set libobs
        if ($LASTEXITCODE) { throw 'Could not check out OBS headers.' }
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $vs = & $vswhere -version '[17.0,18.0)' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (!$vs) { throw 'Install Visual Studio 2022 with the x64 C++ build tools and Windows SDK.' }
    $vc = Get-ChildItem "$vs\VC\Tools\MSVC" -Directory | Sort-Object { [version]$_.Name } -Descending | Select-Object -First 1
    $x64 = Join-Path $vc.FullName 'bin\Hostx64\x64'

    $null = New-Item -ItemType Directory -Force build
    $exports = & "$x64\dumpbin.exe" /nologo /exports $obsDll
    if ($LASTEXITCODE) { throw 'Could not inspect obs.dll.' }
    $names = foreach ($line in $exports) {
        if ($line -match '^\s*\d+\s+[\dA-F]+\s+[\dA-F]+\s+(\w+)(?:\s+=.*)?\s*$') { $Matches[1] }
    }
    if (!($names -contains 'obs_register_source_s')) { throw 'Missing OBS exports.' }
    @('LIBRARY obs.dll', 'EXPORTS') + $names | Set-Content build/obs.def -Encoding ascii
    & "$x64\lib.exe" /nologo /machine:x64 /def:build/obs.def /out:build/obs.lib
    if ($LASTEXITCODE) { throw 'Could not create OBS import library.' }

    cmake -S . -B build -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_GENERATOR_INSTANCE=$vs"
    if ($LASTEXITCODE) { throw 'CMake configuration failed.' }
    cmake --build build --config Release --parallel 1 -- /nodeReuse:false
    if ($LASTEXITCODE) { throw 'Build failed.' }
} finally {
    Pop-Location
}
