$ErrorActionPreference = "Stop"

function Get-AsanDllPath {
    # See https://github.com/microsoft/vswhere/wiki/Find-VC#powershell.
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsInstallDir = &$vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.ASAN -property installationPath

    if (!$vsInstallDir) {
        throw "Couldn't determine VS install directory"
    }

    $versionPath = join-path $vsInstallDir "VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt"

    if (!(Test-Path $versionPath)) {
        throw "Couldn't find version file"
    }

    $version = Get-Content -raw $versionPath

    if (!$version) {
        throw "Couldn't read version file"
    }

    $version = $version.Trim()
    $asanDllPath = join-path $vsInstallDir "VC\Tools\MSVC\$version\bin\Hostx64\$env:arch"

    if (!(Test-Path $asanDllPath)) {
        throw "Couldn't find ASAN DLL path"
    }

    return $asanDllPath
}

if ($env:platform -eq "ARM64") {
    return;
}

# To be able to run the test executable when ASAN is enabled, the appropriate ASAN DLL needs to be on the path.
if ($env:configuration -eq "Debug-Asan") {
    $asanDllPath = Get-AsanDllPath
    $env:Path = $asanDllPath + ";" + $env:Path
}

Set-Location $env:APPVEYOR_BUILD_FOLDER\Explorer++\TestExplorer++\$env:platform\$env:configuration

# Run the tests.
.\TestExplorer++.exe --gtest_output=xml:TestExplorer++Output.xml

# Upload results to AppVeyor.
$wc = New-Object "System.Net.WebClient"
$wc.UploadFile("https://ci.appveyor.com/api/testresults/junit/$($env:APPVEYOR_JOB_ID)", (Resolve-Path .\TestExplorer++Output.xml))