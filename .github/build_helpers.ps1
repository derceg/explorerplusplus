function Initialize-DeveloperEnvironment {
    param(
        [Parameter(Mandatory)]
        [string] $Platform
    )

    $ErrorActionPreference = 'Stop'
    $PSNativeCommandUseErrorActionPreference = $true

    # See https://github.com/microsoft/vswhere/wiki/Start-Developer-Command-Prompt#using-powershell.
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsInstallDir = &$vsWhere -latest -property installationPath

    if (!$vsInstallDir) {
        throw "Couldn't determine VS install directory"
    }

    $vsDevCmd = Join-Path $vsInstallDir 'Common7\Tools\VsDevCmd.bat'

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

    foreach ($line in $developerEnvironment) {
        $name, $value = $line -split '=', 2
        Set-Item -LiteralPath "Env:$name" -Value $value
    }
}