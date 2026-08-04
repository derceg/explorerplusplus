param(
    [Parameter(Mandatory)]
    [string] $Configuration,

    [Parameter(Mandatory)]
    [string] $Platform
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true

. .\.github\build_helpers.ps1

# The LLVM tools (used below to process coverage data) need to be on the path, which is why a developer environment is
# initialized here.
Initialize-DeveloperEnvironment -Platform $Platform

$testExecutable = ".\Explorer++\TestExplorer++\${Platform}\${Configuration}\TestExplorer++.exe"
$rawProfile = 'coverage.profraw'
$indexedProfile = 'coverage.profdata'

$env:LLVM_PROFILE_FILE = $rawProfile

& $testExecutable

if (!(Test-Path -LiteralPath $rawProfile -PathType Leaf)) {
    throw 'The test executable did not generate a Clang coverage profile'
}

& llvm-profdata merge -sparse $rawProfile -o $indexedProfile
& llvm-cov export $testExecutable "-instr-profile=$indexedProfile" -format=lcov > coverage.lcov