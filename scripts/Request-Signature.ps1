# SPDX-FileCopyrightText: 2026 Sigmastone OÜ
# SPDX-License-Identifier: Apache-2.0

<#
.SYNOPSIS
    Has the signing desk sign release files, and replaces them with the signed copies.

.DESCRIPTION
    The job-side half of Signature Master, meant for a release workflow step on a self-hosted
    runner that can reach the desk:

      1. checks that the desk is listening;
      2. opens a request with the files' names, sizes and SHA-256 hashes, and what GitHub says
         about this run (read from the GITHUB_* variables unless given);
      3. uploads each file;
      4. waits until the request is signed, or ends otherwise, or the timeout passes (then the
         request is cancelled);
      5. fetches each signed copy, checks its signature, and puts it in place of the original.

    Any outcome but "signed" exits non-zero with the desk's reason, so the job fails with a line
    that says why. Nothing is written over an original until every signed copy has been fetched
    and checked.

        .\Request-Signature.ps1 -Desk http://192.0.2.10:7443 `
            -Files target\msi\neurotrader-0.1.0-x64.msi `
            -Thumbprint <sha1-thumbprint>

.PARAMETER Desk
    The desk's address, e.g. http://192.0.2.10:7443. Defaults to $env:SIGNING_DESK.

.PARAMETER Files
    The files to sign. Each is replaced in place by its signed copy.

.PARAMETER Thumbprint
    The SHA-1 thumbprint the signature must carry. Defaults to $env:SIGNING_THUMBPRINT; without
    one the signer is not checked, only that a signature is present.

.PARAMETER Token
    A GitHub Actions OIDC token, sent as the bearer. Under GitHub Actions with `id-token: write`
    on the job it is requested here, with an audience bound to this request's manifest, so pass
    nothing. Without a token the desk lists the request as unverified: a person can still
    approve it, the desk never signs it automatically.

.PARAMETER TimeoutMinutes
    How long to wait for a person at the desk. 20 by default.

.PARAMETER AllowUntrustedSigner
    Accept a signature whose chain does not end in a trusted root, as a self-signed test
    certificate's does. Never for a release.

.PARAMETER RequireTimestamp
    Fail when the signature carries no timestamp. On by default; a signature without one dies
    with the certificate.
#>

[CmdletBinding()]
param(
    [string] $Desk = $env:SIGNING_DESK,
    [Parameter(Mandatory = $true)] [string[]] $Files,
    [string] $Thumbprint = $env:SIGNING_THUMBPRINT,
    [string] $Token = $env:SIGNING_TOKEN,
    [string] $Repository = $env:GITHUB_REPOSITORY,
    [string] $Ref = $env:GITHUB_REF,
    [string] $Commit = $env:GITHUB_SHA,
    [string] $Workflow = $env:GITHUB_WORKFLOW,
    [string] $Job = $env:GITHUB_JOB,
    [string] $RunId = $env:GITHUB_RUN_ID,
    [string] $RunUrl,
    [string] $Actor = $env:GITHUB_ACTOR,
    [int] $TimeoutMinutes = 20,
    [int] $PollSeconds = 5,
    [switch] $AllowUntrustedSigner,
    [bool] $RequireTimestamp = $true
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

if (-not $Desk) { throw "No desk address: pass -Desk or set SIGNING_DESK" }
$Desk = $Desk.TrimEnd('/')
if (-not $Repository) { throw "No repository: pass -Repository or run under GitHub Actions" }
if (-not $Ref) { throw "No ref: pass -Ref or run under GitHub Actions" }
if (-not $RunUrl -and $env:GITHUB_SERVER_URL -and $RunId) {
    $RunUrl = "$env:GITHUB_SERVER_URL/$Repository/actions/runs/$RunId"
}
$headers = @{}
if ($Token) { $headers["Authorization"] = "Bearer $Token" }

function Invoke-Desk {
    param([string] $Method, [string] $Path, $Body, [string] $ContentType = "application/json", [string] $InFile, [string] $OutFile)
    $uri = "$Desk$Path"
    $arguments = @{ Method = $Method; Uri = $uri; Headers = $headers; UseBasicParsing = $true; TimeoutSec = 600 }
    if ($Body) { $arguments["Body"] = $Body; $arguments["ContentType"] = $ContentType }
    if ($InFile) { $arguments["InFile"] = $InFile; $arguments["ContentType"] = "application/octet-stream" }
    if ($OutFile) { $arguments["OutFile"] = $OutFile }
    try {
        $response = Invoke-WebRequest @arguments
        # With -OutFile nothing comes back; no exception means the file was written.
        if ($response) { $status = [int] $response.StatusCode } else { $status = 200 }
        $content = if ($OutFile -or -not $response) { $null } else { $response.Content }
    } catch [System.Net.Http.HttpRequestException], [Microsoft.PowerShell.Commands.HttpResponseException], [System.Net.WebException] {
        $response = $_.Exception.Response
        if (-not $response) { throw "The desk at $Desk did not answer ($Method $Path): $($_.Exception.Message)" }
        $status = [int] $response.StatusCode
        $content = $_.ErrorDetails.Message
    }
    $json = $null
    if ($content) { try { $json = $content | ConvertFrom-Json } catch { $json = $null } }
    [pscustomobject]@{ Status = $status; Body = $json; Raw = $content }
}

function Get-Reason {
    param($Reply)
    if ($Reply.Body -and $Reply.Body.reason) { return $Reply.Body.reason }
    if ($Reply.Body -and $Reply.Body.error) { return $Reply.Body.error }
    if ($Reply.Raw) { return $Reply.Raw }
    return "HTTP $($Reply.Status)"
}

# 1. Is the desk there?
$health = Invoke-Desk -Method Get -Path "/health"
if ($health.Status -ne 200 -or $health.Body.status -ne "ok") {
    throw "The desk at $Desk is not listening (HTTP $($health.Status))"
}
Write-Host "Desk $Desk is listening (version $($health.Body.version), $($health.Body.pending) pending)"

# 2. The manifest.
$resolved = @()
foreach ($file in $Files) {
    $item = Get-Item -LiteralPath $file
    $hash = (Get-FileHash -LiteralPath $item.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $resolved += [pscustomobject]@{ Path = $item.FullName; Name = $item.Name; Size = $item.Length; Sha256 = $hash }
}
$names = $resolved | Select-Object -ExpandProperty Name
if (($names | Select-Object -Unique).Count -ne $names.Count) {
    throw "Two files share a name; the desk keys files by name within a request"
}
$manifest = [ordered]@{
    repository = $Repository
    ref        = $Ref
    commit     = $Commit
    workflow   = $Workflow
    job        = $Job
    run_id     = $RunId
    run_url    = $RunUrl
    actor      = $Actor
    files      = @($resolved | ForEach-Object { [ordered]@{ name = $_.Name; size = $_.Size; sha256 = $_.Sha256 } })
}
# The body is sent as these exact bytes: the token's audience is their SHA-256, which is how the
# desk knows the token was minted for this manifest and no other.
$body = [System.Text.Encoding]::UTF8.GetBytes(($manifest | ConvertTo-Json -Depth 5 -Compress))
$bodyHash = ([System.Security.Cryptography.SHA256]::Create().ComputeHash($body) | ForEach-Object { $_.ToString("x2") }) -join ''
$audience = "signature-master:$bodyHash"

if (-not $Token -and $env:ACTIONS_ID_TOKEN_REQUEST_URL -and $env:ACTIONS_ID_TOKEN_REQUEST_TOKEN) {
    # GitHub Actions: an ID token for this run, bound to this manifest. Needs `id-token: write`.
    $tokenUri = "$env:ACTIONS_ID_TOKEN_REQUEST_URL&audience=$([uri]::EscapeDataString($audience))"
    $issued = Invoke-RestMethod -Uri $tokenUri -Headers @{ Authorization = "Bearer $env:ACTIONS_ID_TOKEN_REQUEST_TOKEN" }
    $Token = $issued.value
    if (-not $Token) { throw "GitHub did not issue an ID token; is id-token: write set on the job?" }
    $headers["Authorization"] = "Bearer $Token"
    Write-Host "Holding a GitHub Actions token bound to this manifest"
} elseif (-not $Token) {
    Write-Host "No token: the desk will list this request as unverified, for approval by hand"
}

$created = Invoke-Desk -Method Post -Path "/requests" -Body $body
if ($created.Status -eq 403) {
    throw "The desk refused the request: $(Get-Reason $created)"
}
if ($created.Status -ne 201) {
    throw "The desk did not accept the request (HTTP $($created.Status)): $(Get-Reason $created)"
}
$id = $created.Body.id
Write-Host "Request $id opened for $($resolved.Count) file(s)"

# 3. The files.
foreach ($entry in $resolved) {
    Write-Host "Uploading $($entry.Name) ($([math]::Round($entry.Size / 1MB, 1)) MB)"
    $put = Invoke-Desk -Method Put -Path "/requests/$id/files/$([uri]::EscapeDataString($entry.Name))" -InFile $entry.Path
    if ($put.Status -ne 200) {
        throw "The desk did not take $($entry.Name) (HTTP $($put.Status)): $(Get-Reason $put)"
    }
}

# 4. Wait for the person at the desk.
$deadline = (Get-Date).AddMinutes($TimeoutMinutes)
$lastState = ""
$final = $null
while ($true) {
    $current = Invoke-Desk -Method Get -Path "/requests/$id"
    if ($current.Status -ne 200) { throw "The request vanished from the desk (HTTP $($current.Status))" }
    $state = $current.Body.state
    if ($state -ne $lastState) {
        Write-Host "Request $id is $state"
        $lastState = $state
    }
    if ($state -in @("signed", "refused", "rejected", "failed", "expired")) { $final = $current.Body; break }
    if ((Get-Date) -gt $deadline) {
        Invoke-Desk -Method Delete -Path "/requests/$id" | Out-Null
        throw "No signature within $TimeoutMinutes minutes; the request was cancelled"
    }
    Start-Sleep -Seconds $PollSeconds
}
if ($final.state -ne "signed") {
    throw "The request ended $($final.state): $($final.reason)"
}

# 5. Fetch, check, replace.
$fetched = @()
$temporaries = @()
try {
    foreach ($entry in $resolved) {
        $temporary = Join-Path ([System.IO.Path]::GetDirectoryName($entry.Path)) ("$($entry.Name).signed-$id")
        $temporaries += $temporary
        $download = Invoke-Desk -Method Get -Path "/requests/$id/signed/$([uri]::EscapeDataString($entry.Name))" -OutFile $temporary
        if ($download.Status -ne 200) {
            throw "The signed copy of $($entry.Name) could not be fetched (HTTP $($download.Status))"
        }
        $signature = Get-AuthenticodeSignature -LiteralPath $temporary
        if (-not $signature.SignerCertificate) {
            throw "$($entry.Name) came back without a signature"
        }
        $signer = $signature.SignerCertificate.Thumbprint.ToLowerInvariant()
        if ($Thumbprint -and $signer -ne $Thumbprint.ToLowerInvariant()) {
            throw "$($entry.Name) is signed by $signer, not by $Thumbprint"
        }
        $acceptable = @("Valid")
        if ($AllowUntrustedSigner) { $acceptable += @("UnknownError", "NotTrusted") }
        if ($signature.Status -notin $acceptable) {
            throw "$($entry.Name): the signature is $($signature.Status) ($($signature.StatusMessage))"
        }
        if ($RequireTimestamp -and -not $signature.TimeStamperCertificate) {
            throw "$($entry.Name): the signature carries no timestamp"
        }
        $fetched += [pscustomobject]@{ Entry = $entry; Temporary = $temporary; Signer = $signer; Timestamper = $signature.TimeStamperCertificate.Subject }
    }
    foreach ($item in $fetched) {
        Move-Item -LiteralPath $item.Temporary -Destination $item.Entry.Path -Force
        Write-Host "Signed $($item.Entry.Name) with $($item.Signer), timestamped by $($item.Timestamper)"
    }
}
finally {
    foreach ($temporary in $temporaries) {
        if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force }
    }
}

[pscustomobject]@{
    RequestId  = $id
    ApprovedBy = $final.approved_by
    Files      = @($fetched | ForEach-Object { $_.Entry.Path })
}
