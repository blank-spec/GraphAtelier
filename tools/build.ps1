param(
    [string]$QtKit = 'D:/Qt/6.11.0/mingw_64',
    [string]$CompilerBin = 'D:/Qt/Tools/mingw1310_64/bin',
    [string]$CMakeBin = 'D:/Qt/Tools/CMake_64/bin',
    [string]$NinjaBin = 'D:/Qt/Tools/Ninja',
    [switch]$Deploy
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$env:PATH = "$CompilerBin;$CMakeBin;$NinjaBin;$QtKit/bin;$env:PATH"

Push-Location -LiteralPath $projectRoot

try {
    & "$CMakeBin/cmake.exe" -S . -B build -G Ninja `
        -DCMAKE_BUILD_TYPE=Release `
        -DATELIER_UI_CAPTURE=OFF `
        "-DCMAKE_PREFIX_PATH=$QtKit" `
        "-DCMAKE_CXX_COMPILER=$CompilerBin/g++.exe" `
        "-DCMAKE_MAKE_PROGRAM=$NinjaBin/ninja.exe"

    if ($LASTEXITCODE -ne 0) {
        throw 'CMake configuration failed.'
    }

    & "$CMakeBin/cmake.exe" --build build --parallel

    if ($LASTEXITCODE -ne 0) {
        throw 'Build failed.'
    }

    & "$CMakeBin/ctest.exe" --test-dir build --output-on-failure

    if ($LASTEXITCODE -ne 0) {
        throw 'Algorithm checks failed.'
    }

    if ($Deploy) {
        New-Item -ItemType Directory -Force -Path app | Out-Null
        Copy-Item -LiteralPath build/GraphAtelier.exe -Destination app/GraphAtelier.exe -Force

        & "$QtKit/bin/windeployqt.exe" --release --compiler-runtime --qmldir qml `
            --translations ru,en --skip-plugin-types qmltooling,platforminputcontexts `
            --dir app app/GraphAtelier.exe

        if ($LASTEXITCODE -ne 0) {
            throw 'Qt deployment failed.'
        }

        @'
[Paths]
Prefix=.
Plugins=.
QmlImports=qml
Translations=translations
'@ | Set-Content -LiteralPath app/qt.conf -Encoding ascii

        Copy-Item -LiteralPath README.md -Destination app -Force
        Copy-Item -LiteralPath licenses -Destination app -Recurse -Force
    }
}
finally {
    Pop-Location
}
