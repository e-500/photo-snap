$ErrorActionPreference = 'Stop'

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$outputDirectory = Join-Path $projectRoot 'output'
$buildDirectory = Join-Path $outputDirectory 'build'
$null = New-Item -ItemType Directory -Force -Path $outputDirectory

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    throw 'CMake was not found on PATH. Install CMake 3.16 or newer and reopen this terminal.'
}

$configureArgs = @(
    '-S', $projectRoot,
    '-B', $buildDirectory,
    '-DCMAKE_BUILD_TYPE=Release',
    '-DCMAKE_INSTALL_BINDIR=bin',
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=$(Join-Path $buildDirectory 'runtime')",
    "-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=$(Join-Path $buildDirectory 'runtime')"
)

if (-not [string]::IsNullOrWhiteSpace($env:CMAKE_PREFIX_PATH)) {
    $configureArgs += "-DCMAKE_PREFIX_PATH=$env:CMAKE_PREFIX_PATH"
}
elseif (-not [string]::IsNullOrWhiteSpace($env:Qt6_DIR)) {
    $configureArgs += "-DQt6_DIR=$env:Qt6_DIR"
}
elseif (-not [string]::IsNullOrWhiteSpace($env:Qt5_DIR)) {
    $configureArgs += "-DQt5_DIR=$env:Qt5_DIR"
}
else {
    $qtInstallRoot = 'C:\source_personal\qt'
    if (Test-Path -LiteralPath $qtInstallRoot) {
        $qtConfig = Get-ChildItem -Path $qtInstallRoot -Filter 'Qt6Config.cmake' -File -Recurse -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\msvc[^\\]*\\lib\\cmake\\Qt6\\Qt6Config\.cmake$' } |
            Select-Object -First 1
        if ($qtConfig) {
            $qtPrefix = $qtConfig.Directory.Parent.Parent.Parent.FullName
            $configureArgs += "-DCMAKE_PREFIX_PATH=$qtPrefix"
            Write-Host "Using Qt from: $qtPrefix"
        }
    }
}

Write-Host 'Configuring Photo Snap for Release...'
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE."
}

Write-Host 'Building Photo Snap...'
& cmake --build $buildDirectory --config Release --parallel
if ($LASTEXITCODE -ne 0) {
    throw "The build failed with exit code $LASTEXITCODE."
}

Write-Host 'Installing and deploying Qt runtime files...'
& cmake --install $buildDirectory --config Release --prefix $outputDirectory
if ($LASTEXITCODE -ne 0) {
    throw "Install or Qt runtime deployment failed with exit code $LASTEXITCODE."
}

$application = Join-Path $outputDirectory 'bin\photo-snap.exe'
$scanner = Join-Path $outputDirectory 'bin\photo-snap-cache-scanner.exe'
if (-not (Test-Path -LiteralPath $application)) {
    throw "Build completed, but the executable was not found at $application."
}
if (-not (Test-Path -LiteralPath $scanner)) {
    throw "Build completed, but the cache scanner was not found at $scanner."
}

$cachePath = Join-Path $buildDirectory 'CMakeCache.txt'
$deploymentConfigured = Select-String -Path $cachePath -Pattern '^PHOTOSNAP_QT_DEPLOYMENT_API:INTERNAL=ON$' -Quiet
if (-not $deploymentConfigured) {
    $qtDirEntry = Select-String -Path $cachePath -Pattern '^Qt(6|5)_DIR:PATH=' | Select-Object -First 1
    $qtPrefix = $null
    if ($qtDirEntry) {
        $qtConfigDirectory = $qtDirEntry.Line.Substring($qtDirEntry.Line.IndexOf('=') + 1)
        $qtPrefix = (Get-Item -LiteralPath $qtConfigDirectory).Parent.Parent.Parent.FullName
    }
    $deployTool = if ($qtPrefix) { Join-Path $qtPrefix 'bin\windeployqt.exe' }
    $deployCommand = Get-Command windeployqt -ErrorAction SilentlyContinue
    if ($deployTool -and (Test-Path -LiteralPath $deployTool)) {
        & $deployTool --release $application
    }
    elseif ($deployCommand) {
        & $deployCommand.Source --release $application
    }
    else {
        throw 'Qt deployment support is unavailable. Use Qt 6.3 or newer, or install windeployqt for the selected Qt kit.'
    }
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt failed with exit code $LASTEXITCODE."
    }
    if ($deployTool -and (Test-Path -LiteralPath $deployTool)) {
        & $deployTool --release $scanner
    }
    elseif ($deployCommand) {
        & $deployCommand.Source --release $scanner
    }
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt failed to deploy the cache scanner with exit code $LASTEXITCODE."
    }
}

Write-Host "Deployable application created: $application"
Write-Host "Cache scanner created: $scanner" -ForegroundColor Green
