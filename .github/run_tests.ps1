param(
    [Parameter(Mandatory)]
    [string] $Configuration,

    [Parameter(Mandatory)]
    [string] $Platform
)

$ErrorActionPreference = "Stop"
$PSNativeCommandUseErrorActionPreference = $true

if ($Configuration -eq "Debug-Asan") {
    # See https://github.com/microsoft/vswhere/wiki/Start-Developer-Command-Prompt#using-powershell.
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsInstallDir = &$vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.ASAN -property installationPath

    if (!$vsInstallDir) {
        throw "Couldn't determine VS install directory"
    }

    $vsDevCmd = Join-Path $vsInstallDir "Common7\Tools\VsDevCmd.bat"

    if (!(Test-Path $vsDevCmd)) {
        throw "VsDevCmd.bat not found: $vsDevCmd"
    }

    $architecture = switch ($Platform) {
        'Win32' { 'x86' }
        'x64' { 'x64' }
        'ARM64' { 'arm64' }
        default { throw "Unsupported platform: $Platform" }
    }

    $developerEnvironment = & $env:COMSPEC /s /c "`"$vsDevCmd`" -no_logo -arch=$architecture -host_arch=amd64 && set"

    # To be able to run the test executable when ASAN is enabled, the appropriate ASAN DLL needs to be on the path.
    # This is accomplished here by effectively setting up a developer environment.
    foreach ($line in $developerEnvironment) {
        $name, $value = $line -split "=", 2
        Set-Item -LiteralPath "Env:$name" -Value $value
    }
}

& ".\Explorer++\TestExplorer++\${Platform}\${Configuration}\TestExplorer++.exe"