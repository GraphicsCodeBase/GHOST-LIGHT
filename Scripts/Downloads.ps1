# Pinned downloads for dependencies, tools and assets: fetch once, verify SHA-256, extract or copy, skip what is already present.
# An entry is either one file ("url" + "sha256") or a set of files ("baseUrl" + "files": [{path, sha256, sizeBytes}]).

function Get-Sha256([string]$Path) {
    return (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash.ToLowerInvariant()
}

function Format-Size([double]$Bytes) {
    return '{0:N1} MB' -f ($Bytes / 1MB)
}

function Test-HasProperty($Object, [string]$Name) {
    return $null -ne $Object.PSObject.Properties[$Name]
}

function Get-UrlFileName([string]$Url) {
    return [System.IO.Path]::GetFileName(([System.Uri]$Url).AbsolutePath)
}

# Every file an entry consists of: download URL, expected hash, size, path inside the destination, and cache location.
function Get-EntryFiles($Entry, [string]$CacheDir) {
    if (Test-HasProperty $Entry 'files') {
        return @($Entry.files | ForEach-Object {
            [pscustomobject]@{
                Url     = $Entry.baseUrl + $_.path
                Sha256  = $_.sha256
                Size    = [double]$_.sizeBytes
                Path    = $_.path
                Cached  = Join-Path (Join-Path $CacheDir $Entry.name) ($_.path -replace '/', '\')
            }
        })
    }
    $fileName = Get-UrlFileName $Entry.url
    return @([pscustomobject]@{
        Url    = $Entry.url
        Sha256 = $Entry.sha256
        Size   = [double]$Entry.sizeBytes
        Path   = $fileName
        Cached = Join-Path $CacheDir "$($Entry.name)--$fileName"
    })
}

function Test-IsArchive([string]$Path) {
    return $Path -match '\.(zip|tar\.gz|tgz)$'
}

# The stamp records exactly which pinned files a destination was installed from, so a version bump reinstalls it.
function Get-EntryStamp($Entry) {
    if (Test-HasProperty $Entry 'files') {
        return "$($Entry.baseUrl)|" + (($Entry.files | ForEach-Object { $_.sha256 }) -join ',')
    }
    return "$($Entry.url)|$($Entry.sha256)"
}

function Get-StampPath([string]$Root, $Entry) {
    return Join-Path $Root ".tools\stamps\$($Entry.name).stamp"
}

function Test-EntryInstalled($Entry, [string]$Dest, [string]$Root, [string]$CacheDir) {
    $stampFile = Get-StampPath $Root $Entry
    if (-not (Test-Path -LiteralPath $stampFile)) { return $false }
    if ((Get-Content -LiteralPath $stampFile -Raw).Trim() -ne (Get-EntryStamp $Entry)) { return $false }
    $files = @(Get-EntryFiles $Entry $CacheDir)
    if ($files.Count -eq 1 -and (Test-IsArchive $files[0].Path)) {
        return (Test-Path -LiteralPath $Dest)
    }
    foreach ($file in $files) {
        if (-not (Test-Path -LiteralPath (Join-Path $Dest ($file.Path -replace '/', '\')))) { return $false }
    }
    return $true
}

# Downloads one file into the cache (unless a verified copy is already there) and checks its SHA-256.
function Get-PinnedFile($File, [string]$EntryName, [bool]$RequirePinned) {
    $dir = Split-Path -Parent $File.Cached
    if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }

    $needsDownload = -not (Test-Path -LiteralPath $File.Cached)
    if (-not $needsDownload -and $File.Sha256 -and ((Get-Sha256 $File.Cached) -ne $File.Sha256.ToLowerInvariant())) {
        $needsDownload = $true
    }
    if ($needsDownload) {
        $partial = "$($File.Cached).part"
        $curl = Join-Path $env:WINDIR 'System32\curl.exe'
        $code = Invoke-Logged $curl @('-L', '--fail', '--retry', '3', '--silent', '--show-error', '-o', $partial, $File.Url) -Quiet
        if ($code -ne 0) {
            Remove-Item -LiteralPath $partial -ErrorAction SilentlyContinue
            Stop-Ghost "Could not download $EntryName ($($File.Url)). Check your internet connection and run again."
        }
        Move-Item -LiteralPath $partial -Destination $File.Cached -Force
    }

    $actual = Get-Sha256 $File.Cached
    if ($File.Sha256) {
        if ($actual -ne $File.Sha256.ToLowerInvariant()) {
            Remove-Item -LiteralPath $File.Cached -ErrorAction SilentlyContinue
            Stop-Ghost "Checksum mismatch for $EntryName ($($File.Path)): expected $($File.Sha256), got $actual. The download was corrupted or changed upstream. Run again; if it repeats, the pin needs updating."
        }
    } elseif ($RequirePinned) {
        Stop-Ghost "$EntryName ($($File.Path)) has no pinned sha256 (computed $actual). Add it to the manifest before committing."
    } else {
        Write-Warn "$EntryName ($($File.Path)) is not pinned yet. sha256 = $actual"
    }
}

# Downloads (or reuses cached copies of) an entry's files, verifies them, and installs them at $Dest.
# A single archive is extracted (Dest is replaced); a single plain file is copied into Dest (other files stay);
# a file set replaces Dest with exactly those files.
function Install-PinnedEntry($Entry, [string]$Dest, [string]$Root, [string]$CacheDir, [bool]$RequirePinned) {
    $files = @(Get-EntryFiles $Entry $CacheDir)
    $total = ($files | Measure-Object -Property Size -Sum).Sum
    Write-Info ("  {0} {1} ({2}, {3} file(s))" -f $Entry.name, $Entry.version, (Format-Size $total), $files.Count)
    foreach ($file in $files) {
        Get-PinnedFile $file $Entry.name $RequirePinned
    }

    $parent = Split-Path -Parent $Dest
    if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }

    if ($files.Count -eq 1 -and (Test-IsArchive $files[0].Path)) {
        if (Test-Path -LiteralPath $Dest) { Remove-Item -LiteralPath $Dest -Recurse -Force }
        $staging = "$Dest.extracting"
        if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
        New-Item -ItemType Directory -Path $staging -Force | Out-Null
        # Windows' own bsdtar (not Git's GNU tar) extracts both zip and tar.gz.
        $tar = Join-Path $env:WINDIR 'System32\tar.exe'
        $code = Invoke-Logged $tar @('-xf', $files[0].Cached, '-C', $staging) -Quiet
        if ($code -ne 0) { Stop-Ghost "Could not extract $($files[0].Cached). Delete it and run again." }
        $source = $staging
        if ((Test-HasProperty $Entry 'stripTopLevel') -and $Entry.stripTopLevel) {
            $items = @(Get-ChildItem -LiteralPath $staging -Force)
            if ($items.Count -ne 1 -or -not $items[0].PSIsContainer) {
                Stop-Ghost "Unexpected archive layout for $($Entry.name): expected a single top-level folder."
            }
            $source = $items[0].FullName
        }
        Move-Item -LiteralPath $source -Destination $Dest
        if (Test-Path -LiteralPath $staging) { Remove-Item -LiteralPath $staging -Recurse -Force }
    } elseif ($files.Count -eq 1) {
        New-Item -ItemType Directory -Path $Dest -Force | Out-Null
        Copy-Item -LiteralPath $files[0].Cached -Destination (Join-Path $Dest $files[0].Path) -Force
    } else {
        if (Test-Path -LiteralPath $Dest) { Remove-Item -LiteralPath $Dest -Recurse -Force }
        foreach ($file in $files) {
            $target = Join-Path $Dest ($file.Path -replace '/', '\')
            $targetDir = Split-Path -Parent $target
            if (-not (Test-Path -LiteralPath $targetDir)) { New-Item -ItemType Directory -Path $targetDir -Force | Out-Null }
            Copy-Item -LiteralPath $file.Cached -Destination $target -Force
        }
    }

    $stampFile = Get-StampPath $Root $Entry
    $stampDir = Split-Path -Parent $stampFile
    if (-not (Test-Path -LiteralPath $stampDir)) { New-Item -ItemType Directory -Path $stampDir -Force | Out-Null }
    Set-Content -LiteralPath $stampFile -Value (Get-EntryStamp $Entry) -Encoding ASCII
}

# Installs every entry of a manifest whose destination is missing or outdated. Shows the total download size first.
function Install-Manifest([string]$ManifestPath, [string]$ArrayName, [string]$Root, [bool]$RequirePinned) {
    $manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
    $entries = @($manifest.$ArrayName)
    $cacheDir = Join-Path $Root '.tools\downloads'

    $pending = @()
    foreach ($entry in $entries) {
        $dest = Join-Path $Root $entry.dest
        if ($RequirePinned) {
            foreach ($file in (Get-EntryFiles $entry $cacheDir)) {
                if (-not $file.Sha256) { Stop-Ghost "$($entry.name) ($($file.Path)) in $(Split-Path -Leaf $ManifestPath) has no pinned sha256." }
            }
        }
        if (-not (Test-EntryInstalled $entry $dest $Root $cacheDir)) { $pending += [pscustomobject]@{ Entry = $entry; Dest = $dest } }
    }

    if ($pending.Count -eq 0) {
        Write-Info "  all $($entries.Count) up to date"
        return
    }
    $downloadBytes = 0.0
    foreach ($item in $pending) {
        foreach ($file in (Get-EntryFiles $item.Entry $cacheDir)) {
            if (-not (Test-Path -LiteralPath $file.Cached)) { $downloadBytes += $file.Size }
        }
    }
    if ($downloadBytes -gt 0) {
        Write-Info ("  {0} of {1} to install, {2} to download (first run only)" -f $pending.Count, $entries.Count, (Format-Size $downloadBytes))
    } else {
        Write-Info ("  {0} of {1} to install from the local download cache" -f $pending.Count, $entries.Count)
    }
    foreach ($item in $pending) {
        Install-PinnedEntry $item.Entry $item.Dest $Root $cacheDir $RequirePinned
    }
}
