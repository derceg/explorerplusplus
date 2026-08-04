param(
    [Parameter(Mandatory)]
    [string] $DeploymentDirectory,

    [Parameter(Mandatory)]
    [string] $BucketPathPrefix,

    [Parameter(Mandatory)]
    [string] $BuildNumber
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true

function Get-FullVersionNumber {
    param(
        [Parameter(Mandatory)]
        [string] $BuildNumber
    )

    [xml] $versionProps = Get-Content -LiteralPath 'Explorer++/VersionNumber.props' -Raw

    $propertyGroup = $versionProps.Project.PropertyGroup |
        Where-Object { $_.ExplorerPlusPlusMajorVersion } |
        Select-Object -First 1

    if ($null -eq $propertyGroup) {
        throw "Version properties weren't found in VersionNumber.props"
    }

    $components = @(
        $propertyGroup.ExplorerPlusPlusMajorVersion
        $propertyGroup.ExplorerPlusPlusMinorVersion
        $propertyGroup.ExplorerPlusPlusMicroVersion
        $BuildNumber
    )

    if ($components | Where-Object { $_ -notmatch '^\d+$' }) {
        throw 'All version components must be unsigned integers'
    }

    return $components -join '.'
}

function Copy-ToS3 {
    param(
        [Parameter(Mandatory)]
        [string] $Source,

        [Parameter(Mandatory)]
        [string] $DestinationPrefix,

        [switch] $AllowOverwrite
    )

    $additionalArguments = @(
        '--dryrun'
    )

    if (!$AllowOverwrite) {
        $additionalArguments += '--no-overwrite'
    }

    & aws s3 cp $Source "s3://explorerplusplus-builds/$DestinationPrefix/" @additionalArguments
}

if ($BuildNumber -eq 0) {
    # Only official builds are deployed and all official builds should be assigned non-0 build numbers.
    throw "Invalid build number"
}

$fullVersion = Get-FullVersionNumber -BuildNumber $BuildNumber

$artifactsToDeploy = @(
    'Explorer++.Release.Win32/explorerpp_x86.zip'
    'Explorer++.Release.Win32/explorerpp_x86_setup.msi'
    'Explorer++.Release.Win32/explorerpp_translations.zip'
    'Explorer++.Release.Win32/explorerpp_x86_symbols.zip'

    'Explorer++.Release.x64/explorerpp_x64.zip'
    'Explorer++.Release.x64/explorerpp_x64_setup.msi'
    'Explorer++.Release.x64/explorerpp_x64_symbols.zip'

    'Explorer++.Release.ARM64/explorerpp_arm64.zip'
    'Explorer++.Release.ARM64/explorerpp_arm64_symbols.zip'
)

$artifactsToDeploy = $artifactsToDeploy | ForEach-Object {
    Join-Path $DeploymentDirectory $_
}

# It's necessary for each artifact listed above to exist for a deployment to take place. If any of the artifacts are
# missing, the deployment as a whole is invalid.
foreach ($artifact in $artifactsToDeploy) {
    if (!(Test-Path -LiteralPath $artifact -PathType Leaf)) {
        throw "Required deployment artifact not found: $artifact"
    }
}

foreach ($artifact in $artifactsToDeploy) {
    Copy-ToS3 -Source $artifact -DestinationPrefix "$BucketPathPrefix/$fullVersion"
}

$latestVersionPath = Join-Path $DeploymentDirectory 'latest_version.txt'
$fullVersion | Out-File -NoNewline $latestVersionPath
Copy-ToS3 -Source $latestVersionPath -DestinationPrefix $BucketPathPrefix -AllowOverwrite