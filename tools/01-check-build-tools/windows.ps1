$ErrorActionPreference = 'Stop'

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host 'MISSING: CMake 3.16 or newer.' -ForegroundColor Red
    Write-Host 'Install it with: winget install --id Kitware.CMake -e'
    Write-Host 'Then close and reopen PowerShell so CMake is added to PATH.'
    exit 1
}

$cmakeVersionLine = & cmake --version | Select-Object -First 1
if ($cmakeVersionLine -match '([0-9]+\.[0-9]+(?:\.[0-9]+)?)') {
    $cmakeVersion = [version]$Matches[1]
} else {
    Write-Host 'MISSING: Could not determine the installed CMake version.' -ForegroundColor Red
    Write-Host 'Install CMake 3.16 or newer with: winget install --id Kitware.CMake -e'
    exit 1
}
if ($cmakeVersion -lt [version]'3.16') {
    Write-Host "MISSING: CMake $cmakeVersion is older than 3.16. Upgrade it with: winget upgrade --id Kitware.CMake -e" -ForegroundColor Red
    exit 1
}
Write-Host "OK: CMake $cmakeVersion"

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$visualStudioPath = $null
$visualStudioVersion = $null
if (Test-Path -LiteralPath $vswhere) {
    $visualStudioPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $visualStudioVersion = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion
}
if (-not $visualStudioPath -and -not (Get-Command cl -ErrorAction SilentlyContinue)) {
    Write-Host 'MISSING: A Visual C++ compiler and Windows SDK.' -ForegroundColor Red
    Write-Host 'Install Visual Studio 2022 Build Tools, select the “Desktop development with C++” workload and a Windows 10/11 SDK, then rerun this script.'
    Write-Host 'Installer: https://visualstudio.microsoft.com/visual-cpp-build-tools/'
    exit 1
}
if ($visualStudioVersion) {
    Write-Host "OK: Visual C++ build tools $visualStudioVersion"
} else {
    Write-Host 'OK: Visual C++ compiler found on PATH'
}

$qtPrefix = $env:CMAKE_PREFIX_PATH
$qtDirArg = $null
if ([string]::IsNullOrWhiteSpace($qtPrefix) -and -not [string]::IsNullOrWhiteSpace($env:Qt6_DIR)) {
    $qtDirArg = "-DQt6_DIR=$env:Qt6_DIR"
}
elseif ([string]::IsNullOrWhiteSpace($qtPrefix) -and -not [string]::IsNullOrWhiteSpace($env:Qt5_DIR)) {
    $qtDirArg = "-DQt5_DIR=$env:Qt5_DIR"
}
elseif ([string]::IsNullOrWhiteSpace($qtPrefix)) {
    $qtInstallRoots = @('C:\source_personal\qt', 'C:\Qt', (Join-Path $env:USERPROFILE 'Qt'))
    foreach ($qtInstallRoot in $qtInstallRoots) {
        if (-not (Test-Path -LiteralPath $qtInstallRoot)) { continue }
        foreach ($qtMajor in @('6', '5')) {
            $qtConfig = Get-ChildItem -Path $qtInstallRoot -Filter "Qt${qtMajor}Config.cmake" -File -Recurse -ErrorAction SilentlyContinue |
                Where-Object { $_.FullName -match "\\msvc[^\\]*\\lib\\cmake\\Qt${qtMajor}\\Qt${qtMajor}Config\.cmake$" } |
                Select-Object -First 1
            if ($qtConfig) {
                $qtPrefix = $qtConfig.Directory.Parent.Parent.Parent.FullName
                break
            }
        }
        if ($qtPrefix) { break }
    }
}

$buildDirectory = Join-Path ([System.IO.Path]::GetTempPath()) ("photo-snap-check-{0}" -f [guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Force -Path $buildDirectory
$configureLog = Join-Path $buildDirectory 'configure.log'
$buildLog = Join-Path $buildDirectory 'build.log'
$configureArgs = @('-S', $projectRoot, '-B', $buildDirectory, '-DCMAKE_BUILD_TYPE=Release')
if (-not [string]::IsNullOrWhiteSpace($qtPrefix)) {
    $configureArgs += "-DCMAKE_PREFIX_PATH=$qtPrefix"
}
elseif ($qtDirArg) {
    $configureArgs += $qtDirArg
}

& cmake @configureArgs *> $configureLog
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $configureLog
    if (-not (Select-String -Path $configureLog -Pattern 'Could not find.*QT|Qt[56].*(NOTFOUND|not found)|Qt Widgets|Qt[56]Config\.cmake' -Quiet)) {
        Write-Host 'MISSING: CMake could not configure the selected C++ toolchain.' -ForegroundColor Red
        Write-Host 'Install Visual Studio 2022 Build Tools with the “Desktop development with C++” workload and Windows 10/11 SDK, then rerun this script.'
    } else {
        Write-Host 'MISSING: Qt Widgets development files for the selected Visual C++ architecture.' -ForegroundColor Red
        Write-Host 'Install Qt 6 or Qt 5 with the matching MSVC 64-bit kit using the Qt Online Installer: https://www.qt.io/download-qt-installer-oss'
        Write-Host 'Set CMAKE_PREFIX_PATH to the kit folder (for example C:\Qt\6.x\msvc2022_64), or set Qt6_DIR to its lib\cmake\Qt6 folder, then rerun this script.'
    }
    Write-Host "Configuration log: $configureLog"
    Write-Host "Build directory kept for inspection: $buildDirectory"
    exit 1
}

$qtEntry = Select-String -Path (Join-Path $buildDirectory 'CMakeCache.txt') -Pattern '^Qt(6|5)_DIR:PATH=' | Select-Object -First 1
if ($qtEntry -and $qtEntry.Line -match '^Qt(6|5)_DIR:PATH=') {
    Write-Host "OK: Qt $($Matches[1]) Widgets"
} else {
    Write-Host 'MISSING: Qt Widgets was not selected by CMake.' -ForegroundColor Red
    Write-Host 'Install Qt 6 or Qt 5 with a matching MSVC kit using https://www.qt.io/download-qt-installer-oss, then set CMAKE_PREFIX_PATH to that kit folder.'
    Write-Host "Build directory kept for inspection: $buildDirectory"
    exit 1
}

& cmake --build $buildDirectory --config Release --parallel *> $buildLog
if ($LASTEXITCODE -ne 0) {
    Get-Content -LiteralPath $buildLog
    Write-Host 'MISSING: The configured build toolchain could not build Photo Snap.' -ForegroundColor Red
    Write-Host 'Install or repair Visual Studio 2022 Build Tools with the “Desktop development with C++” workload and Windows 10/11 SDK.'
    Write-Host "Build log: $buildLog"
    Write-Host "Build directory kept for inspection: $buildDirectory"
    exit 1
}

Write-Host 'OK: Photo Snap Release build'
Remove-Item -LiteralPath $buildDirectory -Recurse -Force
