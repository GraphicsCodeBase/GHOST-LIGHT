# Shared helpers for GHOST LIGHT scripts: step banners, Build/run.log logging, logged native commands.

$script:GhostLogFile = $null

# Starts a fresh log file. Everything printed through these helpers is also appended to it.
function Initialize-GhostLog([string]$Path) {
    $dir = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
    $script:GhostLogFile = $Path
    Set-Content -LiteralPath $Path -Value "GHOST LIGHT run log, $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -Encoding UTF8
}

function Write-GhostLog([string]$Text) {
    if ($script:GhostLogFile) { Add-Content -LiteralPath $script:GhostLogFile -Value $Text -Encoding UTF8 }
}

function Write-Info([string]$Text) {
    Write-Host $Text
    Write-GhostLog $Text
}

function Write-Warn([string]$Text) {
    Write-Host "WARNING: $Text" -ForegroundColor Yellow
    Write-GhostLog "WARNING: $Text"
}

function Write-Step([int]$Index, [int]$Count, [string]$Title) {
    $line = "[$Index/$Count] $Title"
    Write-Host ''
    Write-Host $line -ForegroundColor Cyan
    Write-GhostLog $line
}

# Stops the run. The message must tell the user what to do next, not just what went wrong.
function Stop-Ghost([string]$Message) {
    throw $Message
}

# Quotes one command-line argument for cmd.exe when it contains spaces or is empty.
function ConvertTo-CommandArgument([string]$Value) {
    if ($Value -eq '' -or $Value -match '[\s&()^|<>]') { return '"' + $Value + '"' }
    return $Value
}

# Runs a native program through cmd.exe with stdout and stderr merged, streaming each line to the
# console and the log. Uses System.Diagnostics.Process so quoting is exact on Windows PowerShell 5.1.
# Returns the program's exit code.
function Invoke-Logged([string]$Exe, [string[]]$Arguments, [switch]$Quiet) {
    $parts = @(ConvertTo-CommandArgument $Exe) + @($Arguments | ForEach-Object { ConvertTo-CommandArgument $_ })
    $commandLine = $parts -join ' '
    Write-GhostLog "> $commandLine"

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = Join-Path $env:WINDIR 'System32\cmd.exe'
    $psi.Arguments = '/d /s /c "' + $commandLine + ' 2>&1"'
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $process = [System.Diagnostics.Process]::Start($psi)
    while ($null -ne ($line = $process.StandardOutput.ReadLine())) {
        if (-not $Quiet) { Write-Host $line }
        Write-GhostLog $line
    }
    $process.WaitForExit()
    return $process.ExitCode
}

# Runs a command through cmd.exe and returns its output lines without logging them (used for queries).
function Get-CommandOutput([string]$CommandLine) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = Join-Path $env:WINDIR 'System32\cmd.exe'
    $psi.Arguments = '/d /s /c "' + $CommandLine + '"'
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $process = [System.Diagnostics.Process]::Start($psi)
    $lines = New-Object System.Collections.Generic.List[string]
    while ($null -ne ($line = $process.StandardOutput.ReadLine())) { $lines.Add($line) }
    $process.WaitForExit()
    return [pscustomobject]@{ ExitCode = $process.ExitCode; Lines = $lines }
}
