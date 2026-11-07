<#
.SYNOPSIS
    Fails fast when the signing desk is not there.

.DESCRIPTION
    For the start of a release job: a desk that is off should fail the run before the build is
    spent, not after. Exits non-zero unless the desk answers GET /health with status "ok".

        .\Test-SigningDesk.ps1 -Desk http://192.168.0.31:7443

.PARAMETER Desk
    The desk's address. Defaults to $env:SIGNING_DESK.
#>

[CmdletBinding()]
param(
    [string] $Desk = $env:SIGNING_DESK
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

if (-not $Desk) { throw "No desk address: pass -Desk or set SIGNING_DESK" }
$Desk = $Desk.TrimEnd('/')

try {
    $response = Invoke-WebRequest -Uri "$Desk/health" -UseBasicParsing -TimeoutSec 10
} catch {
    throw "The signing desk at $Desk did not answer: $($_.Exception.Message)"
}
$health = $response.Content | ConvertFrom-Json
if ($health.status -ne "ok") {
    throw "The signing desk at $Desk answered status '$($health.status)'"
}
Write-Host "Signing desk $Desk is listening: version $($health.version), $($health.pending) request(s) pending"
$health
