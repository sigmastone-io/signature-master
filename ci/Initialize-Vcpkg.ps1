# SPDX-FileCopyrightText: 2026 Sigmastone OÜ
# SPDX-License-Identifier: Apache-2.0

<#
.SYNOPSIS
    Gives a CI job a vcpkg that knows this project's baseline, and a binary cache.

.DESCRIPTION
    The self-hosted Windows runner is persistent, so vcpkg lives on it between jobs: a checkout
    at -Root (the runner's VCPKG_ROOT if it has one, C:\vcpkg otherwise), cloned on the first
    run, and a files binary cache at -Cache. The first job on a runner builds Qt from source,
    which takes an hour or more; every later one restores it from the cache in a minute.

    When the checkout does not have the commit named by `builtin-baseline` in vcpkg.json, it is
    fast-forwarded and vcpkg is bootstrapped again. VCPKG_ROOT and VCPKG_BINARY_SOURCES are
    written to GITHUB_ENV for the steps that follow.
#>

[CmdletBinding()]
param(
    [string] $Root = $(if ($env:VCPKG_ROOT) { $env:VCPKG_ROOT } else { 'C:\vcpkg' }),
    [string] $Cache = 'C:\vcpkg-cache'
)

$ErrorActionPreference = 'Stop'

$baseline = (Get-Content (Join-Path $PSScriptRoot '..\vcpkg.json') -Raw | ConvertFrom-Json).'builtin-baseline'
$bootstrap = $false

if (-not (Test-Path (Join-Path $Root '.vcpkg-root'))) {
    Write-Host "Cloning vcpkg into $Root"
    git clone --quiet https://github.com/microsoft/vcpkg.git $Root
    if ($LASTEXITCODE -ne 0) { throw 'git clone of vcpkg failed' }
    $bootstrap = $true
}

git -C $Root cat-file -e "$baseline^{commit}" 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "vcpkg at $Root lacks the baseline $baseline; updating it"
    git -C $Root pull --quiet --ff-only
    if ($LASTEXITCODE -ne 0) { throw 'git pull of vcpkg failed' }
    git -C $Root cat-file -e "$baseline^{commit}"
    if ($LASTEXITCODE -ne 0) { throw "vcpkg has no commit $baseline even after an update" }
    $bootstrap = $true
}

if ($bootstrap -or -not (Test-Path (Join-Path $Root 'vcpkg.exe'))) {
    & (Join-Path $Root 'bootstrap-vcpkg.bat') -disableMetrics
    if ($LASTEXITCODE -ne 0) { throw 'bootstrap-vcpkg failed' }
}

New-Item -ItemType Directory -Force $Cache | Out-Null

if ($env:GITHUB_ENV) {
    Add-Content $env:GITHUB_ENV "VCPKG_ROOT=$Root"
    Add-Content $env:GITHUB_ENV "VCPKG_BINARY_SOURCES=clear;files,$Cache,readwrite"
    Add-Content $env:GITHUB_ENV 'VCPKG_DISABLE_METRICS=1'
}
& (Join-Path $Root 'vcpkg.exe') version | Select-Object -First 1
