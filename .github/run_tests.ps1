param(
    [Parameter(Mandatory)]
    [string] $Configuration,

    [Parameter(Mandatory)]
    [string] $Platform
)

$ErrorActionPreference = "Stop"
$PSNativeCommandUseErrorActionPreference = $true

. .\.github\build_helpers.ps1

if ($Configuration -eq "Debug-Asan") {
    # To be able to run the test executable when ASAN is enabled, the appropriate ASAN DLL needs to be on the path.
    # This is accomplished here by effectively setting up a developer environment.
    Initialize-DeveloperEnvironment -Platform $Platform
}

& ".\Explorer++\TestExplorer++\${Platform}\${Configuration}\TestExplorer++.exe"