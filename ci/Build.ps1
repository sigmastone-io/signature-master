# SPDX-FileCopyrightText: 2026 Sigmastone OÜ
# SPDX-License-Identifier: Apache-2.0

<#
.SYNOPSIS
    Builds the `release` preset and runs the tests, as CI does.

.PARAMETER Version
    Passed to CMake as SM_VERSION; see CMakeLists.txt. Empty for the project version.
#>

[CmdletBinding()]
param([string] $Version)

$ErrorActionPreference = 'Stop'

Push-Location (Join-Path $PSScriptRoot '..')
try {
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if (-not $vs) { throw 'No Visual Studio with the C++ tools was found' }
        # The developer shell points VCPKG_ROOT at the copy of vcpkg inside Visual Studio, which
        # is read-only under Program Files; the caller's vcpkg is the one to use.
        $vcpkgRoot = $env:VCPKG_ROOT
        & (Join-Path $vs 'Common7\Tools\Launch-VsDevShell.ps1') -Arch amd64 -SkipAutomaticLocation | Out-Null
        if ($vcpkgRoot) { $env:VCPKG_ROOT = $vcpkgRoot }
    }
    if (-not $env:VCPKG_ROOT) { throw 'VCPKG_ROOT is not set' }

    cmake --preset release "-DSM_VERSION=$Version"
    if ($LASTEXITCODE -ne 0) { throw 'cmake --preset release failed' }
    cmake --build --preset release
    if ($LASTEXITCODE -ne 0) { throw 'The release build failed' }
    ctest --test-dir build\release --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'The tests failed' }
}
finally {
    Pop-Location
}
