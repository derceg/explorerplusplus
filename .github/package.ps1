param(
    [Parameter(Mandatory)]
    [string] $Configuration,

    [Parameter(Mandatory)]
    [string] $Platform
)

$ErrorActionPreference = 'Stop'

$artifactsDirectory = 'artifacts'

function New-Artifact {
    param (
        [Parameter(Mandatory)]
        [string] $Name,

        [Parameter(Mandatory)]
        [string[]] $Files
    )
    
    $artifactPath = Join-Path $artifactsDirectory $Name
    & 7z a $artifactPath @Files

    if ($LASTEXITCODE -ne 0) {
        throw "7z failed with exit code $LASTEXITCODE"
    }
}

$architecture = switch ($Platform) {
    'Win32' { 'x86' }
    'x64' { 'x64' }
    'ARM64' { 'arm64' }
    default { throw "Unsupported platform: $Platform" }
}

$repositoryRoot = $PWD.Path

$applicationFiles = @(
    "$repositoryRoot\Explorer++\Explorer++\$Platform\$Configuration\Explorer++.exe"
    "$repositoryRoot\Explorer++\Explorer++\$Platform\$Configuration\Microsoft.Terminal.Control.dll"
    "$repositoryRoot\Documentation\User\History.txt"
    "$repositoryRoot\Documentation\User\License.txt"
    "$repositoryRoot\Documentation\User\Microsoft.Terminal.Control.LICENSE.txt"
    "$repositoryRoot\Documentation\User\Readme.txt"
)

New-Artifact `
    -Name "explorerpp_$architecture.zip" `
    -Files $applicationFiles

New-Artifact `
    -Name "explorerpp_${architecture}_symbols.zip" `
    -Files "$repositoryRoot\Explorer++\Explorer++\$Platform\$Configuration\Explorer++.pdb"

New-Artifact `
    -Name 'explorerpp_translations.zip' `
    -Files "$repositoryRoot\Explorer++\Win32\$Configuration\Explorer++*.dll"

if ($Platform -ne 'ARM64') {
    Copy-Item `
        "Explorer++\Installer\bin\$architecture\$Configuration\explorerpp_${architecture}_setup.msi" `
        $artifactsDirectory
}
