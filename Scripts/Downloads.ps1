# Pinned downloads for dependencies, tools and assets: fetch once, verify SHA-256, extract, skip what is already present.

function Get-Sha256([string]$Path) {
    return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()
}

function Format-Size([double]$Bytes) {
    return '{0:N1} MB' -f ($Bytes / 1MB)
}

# The stamp records which pinned file a destination was extracted from, so a version bump re-extracts it.
function Get-EntryStamp($Entry) {
    return "$($Entry.url)|$($Entry.sha256)"
}

function Test-EntryInstalled($Entry, [string]$Dest) {
    $stampFile = "$Dest.stamp"
    if (-not (Test-Path -LiteralPath $Dest) -or -not (Test-Path -LiteralPath $stampFile)) { return $false }
    return ((Get-Content -LiteralPath $stampFile -Raw).Trim() -eq (Get-EntryStamp $Entry))
}

# Downloads (or reuses the cached copy of) one pinned file, verifies it, and installs it at $Dest.
# Archives (.zip, .tar.gz) are extracted; any other file is copied into the $Dest folder.
function Get-UrlFileName($Entry) {
    return [System.IO.Path]::GetFileName(([System.Uri]$Entry.url).AbsolutePath)
}

# Downloaded files are kept in .tools/downloads/ so re-extracting never needs the internet.
function Get-CachedPath($Entry, [string]$CacheDir) {
    return Join-Path $CacheDir "$($Entry.name)--$(Get-UrlFileName $Entry)"
}

function Install-PinnedEntry($Entry, [string]$Dest, [string]$CacheDir, [bool]$RequirePinned) {
    $fileName = Get-UrlFileName $Entry
    $cached = Get-CachedPath $Entry $CacheDir
    if (-not (Test-Path -LiteralPath $CacheDir)) { New-Item -ItemType Directory -Path $CacheDir -Force | Out-Null }

    $needsDownload = -not (Test-Path -LiteralPath $cached)
    if (-not $needsDownload -and $Entry.sha256 -and ((Get-Sha256 $cached) -ne $Entry.sha256.ToLowerInvariant())) {
        $needsDownload = $true
    }
    if ($needsDownload) {
        Write-Info ("  downloading {0} {1} ({2}) from {3}" -f $Entry.name, $Entry.version, (Format-Size $Entry.sizeBytes), ([System.Uri]$Entry.url).Host)
        $partial = "$cached.part"
        $curl = Join-Path $env:WINDIR 'System32\curl.exe'
        $code = Invoke-Logged $curl @('-L', '--fail', '--retry', '3', '--silent', '--show-error', '-o', $partial, $Entry.url)
        if ($code -ne 0) {
            Remove-Item -LiteralPath $partial -ErrorAction SilentlyContinue
            Stop-Ghost "Could not download $($Entry.name) from $($Entry.url). Check your internet connection and run again."
        }
        Move-Item -LiteralPath $partial -Destination $cached -Force
    }

    $actual = Get-Sha256 $cached
    if ($Entry.sha256) {
        if ($actual -ne $Entry.sha256.ToLowerInvariant()) {
            Remove-Item -LiteralPath $cached -ErrorAction SilentlyContinue
            Stop-Ghost "Checksum mismatch for $($Entry.name): expected $($Entry.sha256), got $actual. The download was corrupted or changed upstream. Run again; if it repeats, the pin needs updating."
        }
    } elseif ($RequirePinned) {
        Stop-Ghost "$($Entry.name) has no pinned sha256 (computed $actual). Add it to the manifest before committing."
    } else {
        Write-Warn "$($Entry.name) is not pinned yet. sha256 = $actual"
    }

    if (Test-Path -LiteralPath $Dest) { Remove-Item -LiteralPath $Dest -Recurse -Force }
    $parent = Split-Path -Parent $Dest
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }

    if ($fileName -match '\.(zip|tar\.gz|tgz)$') {
        $staging = "$Dest.extracting"
        if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
        New-Item -ItemType Directory -Path $staging -Force | Out-Null
        # Windows' own bsdtar (not Git's GNU tar) extracts both zip and tar.gz.
        $tar = Join-Path $env:WINDIR 'System32\tar.exe'
        $code = Invoke-Logged $tar @('-xf', $cached, '-C', $staging) -Quiet
        if ($code -ne 0) { Stop-Ghost "Could not extract $cached. Delete it and run again." }

        $source = $staging
        if ($Entry.stripTopLevel) {
            $items = @(Get-ChildItem -LiteralPath $staging -Force)
            if ($items.Count -ne 1 -or -not $items[0].PSIsContainer) {
                Stop-Ghost "Unexpected archive layout for $($Entry.name): expected a single top-level folder."
            }
            $source = $items[0].FullName
        }
        Move-Item -LiteralPath $source -Destination $Dest
        if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
    } else {
        New-Item -ItemType Directory -Path $Dest -Force | Out-Null
        Copy-Item -LiteralPath $cached -Destination (Join-Path $Dest $fileName)
    }
    Set-Content -LiteralPath "$Dest.stamp" -Value (Get-EntryStamp $Entry) -Encoding ASCII
}

# Installs every entry of a manifest whose destination is missing or outdated. Shows the total size first.
function Install-Manifest([string]$ManifestPath, [string]$ArrayName, [string]$Root, [bool]$RequirePinned) {
    $manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
    $entries = @($manifest.$ArrayName)
    $cacheDir = Join-Path $Root '.tools\downloads'

    $pending = @()
    foreach ($entry in $entries) {
        $dest = Join-Path $Root $entry.dest
        if ($RequirePinned -and -not $entry.sha256) {
            Stop-Ghost "$($entry.name) in $(Split-Path -Leaf $ManifestPath) has no pinned sha256."
        }
        if (-not (Test-EntryInstalled $entry $dest)) { $pending += [pscustomobject]@{ Entry = $entry; Dest = $dest } }
    }

    if ($pending.Count -eq 0) {
        Write-Info "  all $($entries.Count) up to date"
        return
    }
    $toDownload = @($pending | Where-Object { -not (Test-Path -LiteralPath (Get-CachedPath $_.Entry $cacheDir)) })
    if ($toDownload.Count -gt 0) {
        $total = ($toDownload | ForEach-Object { [double]$_.Entry.sizeBytes } | Measure-Object -Sum).Sum
        Write-Info ("  {0} of {1} to install, {2} to download (first run only)" -f $pending.Count, $entries.Count, (Format-Size $total))
    } else {
        Write-Info ("  {0} of {1} to install from the local download cache" -f $pending.Count, $entries.Count)
    }
    foreach ($item in $pending) {
        Install-PinnedEntry $item.Entry $item.Dest $cacheDir $RequirePinned
    }
}
