# SPDX-FileCopyrightText: 2026 Sigmastone OÜ
# SPDX-License-Identifier: Apache-2.0

<#
.SYNOPSIS
    Builds the Signature Master MSI.

.DESCRIPTION
    The whole of the MSI build, so that CI and a developer's machine run the same thing:

      1. configure and build the `release` CMake preset (the Visual Studio developer environment
         is loaded if cl.exe is not already on the path);
      2. install the executable and its Qt runtime into a clean staging directory;
      3. package the staging directory with WiX.

        .\packaging\windows\build-msi.ps1

    The result is build\msi\signature-master-<version>-x64.msi. The WiX toolset is a .NET global
    tool and is installed on demand if it is not already there, pinned to 5.0.2 (WiX 6 and later
    carry a maintenance-fee EULA that is not accepted here).

.PARAMETER Version
    The product version, as a release tag carries it without the "v" (0.1.0, 0.1.0-rc.1).
    Defaults to the version in CMakeLists.txt, and must start with it: CMake refuses anything
    else. The whole of it goes into the build (SM_VERSION: what the application and its version
    resource report) and into the file name; the MSI's own ProductVersion is its three numeric
    fields, which is all Windows Installer accepts.

.PARAMETER SkipBuild
    Package what build\release and build\stage already hold.
#>

[CmdletBinding()]
param(
    [string] $Version,
    [switch] $SkipBuild,
    [string] $WixVersion = "5.0.2"
)

$ErrorActionPreference = "Stop"

# Every path below is relative to the repository root, whichever directory this was invoked from.
$root = Resolve-Path (Join-Path $PSScriptRoot "..\..")
Push-Location $root
try {
    if (-not $Version) {
        $match = Select-String -Path "CMakeLists.txt" -Pattern 'VERSION\s+(\d+\.\d+\.\d+)' | Select-Object -First 1
        if (-not $match) { throw "Could not read the project version from CMakeLists.txt" }
        $Version = $match.Matches[0].Groups[1].Value
    }
    $msiVersion = ($Version -split '[-+]')[0]
    if ($msiVersion -notmatch '^\d+\.\d+\.\d+$') {
        throw "Version '$Version' does not start with three numeric fields"
    }

    $stage = Join-Path $root "build\stage"
    $icon = Join-Path $root "packaging\windows\signature-master.ico"
    if (-not (Test-Path $icon)) { throw "No icon at '$icon' - build the 'icon' target first" }

    if (-not $SkipBuild) {
        if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
            $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
            $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
            if (-not $vs) { throw "No Visual Studio with the C++ tools was found" }
            # The developer shell points VCPKG_ROOT at the copy of vcpkg inside Visual Studio,
            # which is read-only under Program Files; the caller's vcpkg is the one to use.
            $vcpkgRoot = $env:VCPKG_ROOT
            & (Join-Path $vs "Common7\Tools\Launch-VsDevShell.ps1") -Arch amd64 -SkipAutomaticLocation | Out-Null
            if ($vcpkgRoot) { $env:VCPKG_ROOT = $vcpkgRoot }
        }
        if (-not $env:VCPKG_ROOT) { throw "VCPKG_ROOT is not set" }

        cmake --preset release "-DSM_VERSION=$Version"
        if ($LASTEXITCODE -ne 0) { throw "cmake --preset release failed" }
        cmake --build --preset release
        if ($LASTEXITCODE -ne 0) { throw "The release build failed" }

        if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
        cmake --install build\release --prefix $stage
        if ($LASTEXITCODE -ne 0) { throw "cmake --install failed" }
    }
    if (-not (Test-Path (Join-Path $stage "signature-master.exe"))) {
        throw "No executable in '$stage'"
    }

    $env:PATH = "$env:USERPROFILE\.dotnet\tools;$env:PATH"
    if (-not (Get-Command wix -ErrorAction SilentlyContinue)) {
        Write-Host "Installing the WiX toolset $WixVersion"
        dotnet tool install --global wix --version $WixVersion
        if ($LASTEXITCODE -ne 0) { throw "Could not install the WiX toolset" }
    }
    # The Util extension, for closing a running desk before an upgrade; a no-op when present.
    # Global, so that no .wix cache lands beside the project.
    wix extension add --global "WixToolset.Util.wixext/$WixVersion" 2>&1 | Out-Null

    $outputDir = Join-Path $root "build\msi"
    New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
    $output = Join-Path $outputDir "signature-master-$Version-x64.msi"

    wix build `
        -arch x64 `
        -ext WixToolset.Util.wixext `
        -d Version="$msiVersion" `
        -d StageDir="$stage" `
        -d IconFile="$icon" `
        -o $output `
        packaging\windows\signature-master.wxs
    if ($LASTEXITCODE -ne 0) { throw "wix build failed" }

    Write-Host "Built $output"
    $output
}
finally {
    Pop-Location
}
