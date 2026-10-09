# GHOST LIGHT driver behind run.bat: check requirements, bootstrap tools, configure, build, fetch assets, then launch or test.
param([Parameter(Position = 0)][string]$Mode = '')

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
Set-StrictMode -Version 3.0

$Root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'Common.ps1')
. (Join-Path $PSScriptRoot 'Downloads.ps1')
. (Join-Path $PSScriptRoot 'VisualStudio.ps1')

$Modes = @{
    ''        = @{ Label = 'Debug build + launch';   Config = 'Debug';   Run = 'Engine' }
    'release' = @{ Label = 'Release build + launch'; Config = 'Release'; Run = 'Engine' }
    'test'    = @{ Label = 'Debug build + smoke test'; Config = 'Debug'; Run = 'SmokeTest' }
    'clean'   = @{ Label = 'wipe Build/ + Debug rebuild'; Config = 'Debug'; Run = 'None' }
}
$StepCount = 6

function Test-GpuRequirements {
    $names = @(Get-CimInstance -ClassName Win32_VideoController | ForEach-Object { $_.Name })
    $rtx = @($names | Where-Object { $_ -match 'NVIDIA' -and $_ -match 'RTX' })
    if ($rtx.Count -eq 0) {
        Stop-Ghost "No NVIDIA RTX GPU found (found: $($names -join ', ')). GHOST LIGHT needs an RTX 20-series or newer with a recent driver. See README -> Requirements."
    }
    Write-Info "  GPU: $($rtx[0])"
    if (-not (Test-Path -LiteralPath (Join-Path $env:WINDIR 'System32\vulkan-1.dll'))) {
        Stop-Ghost 'The Vulkan runtime (vulkan-1.dll) is missing. Install or update the NVIDIA driver from https://www.nvidia.com/drivers'
    }
}

$key = $Mode.ToLowerInvariant()
if (-not $Modes.ContainsKey($key)) {
    Write-Host "Unknown option '$Mode'. Usage: run.bat [release | test | clean]"
    exit 2
}
$selected = $Modes[$key]
$buildDir = Join-Path $Root 'Build'
$exitCode = 0

try {
    if ($key -eq 'clean' -and (Test-Path -LiteralPath $buildDir)) {
        Write-Host 'Removing Build/ ...'
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
    Initialize-GhostLog (Join-Path $buildDir 'run.log')
    Write-Info "GHOST LIGHT | $($selected.Label) | $Root"

    Write-Step 1 $StepCount 'Checking requirements'
    $vs = Find-VisualStudio
    Write-Info "  $($vs.Name) $($vs.Version)"
    Test-GpuRequirements

    Write-Step 2 $StepCount 'Bootstrapping tools and dependencies'
    Install-Manifest (Join-Path $PSScriptRoot 'Dependencies.json') 'dependencies' $Root ($key -eq 'test')

    Write-Step 3 $StepCount 'Configuring'
    Enter-VsDevEnvironment $vs
    Clear-VulkanSdkEnvironment
    $cmake = Get-VsTool $vs 'cmake'
    $ninja = Get-VsTool $vs 'ninja'
    $configDir = Join-Path $buildDir $selected.Config
    if (Test-Path -LiteralPath (Join-Path $configDir 'build.ninja')) {
        Write-Info '  already configured (CMake re-runs by itself when build files change)'
    } else {
        $code = Invoke-Logged $cmake @('-S', $Root, '-B', $configDir, '-G', 'Ninja',
                                       "-DCMAKE_BUILD_TYPE=$($selected.Config)", "-DCMAKE_MAKE_PROGRAM=$ninja")
        if ($code -ne 0) { Stop-Ghost 'CMake configuration failed. The reason is printed above (also in Build\run.log).' }
    }

    Write-Step 4 $StepCount "Building ($($selected.Config))"
    $code = Invoke-Logged $cmake @('--build', $configDir)
    if ($code -ne 0) { Stop-Ghost 'Build failed. The first compiler error is printed above (full output in Build\run.log).' }

    Write-Step 5 $StepCount 'Checking assets'
    $assetManifest = Join-Path $Root 'Content\AssetManifest.json'
    if (Test-Path -LiteralPath $assetManifest) {
        Install-Manifest $assetManifest 'assets' $Root ($key -eq 'test')
    } else {
        Write-Info '  no asset manifest yet'
    }

    switch ($selected.Run) {
        'Engine' {
            Write-Step 6 $StepCount 'Launching'
            $exe = Join-Path $configDir 'GhostLight.exe'
            Write-Info "  Build\$($selected.Config)\GhostLight.exe"
            & $exe
            $exitCode = $LASTEXITCODE
            if ($exitCode -ne 0) { Stop-Ghost "GhostLight.exe exited with code $exitCode. Its log is Build\GhostLight.log." }
        }
        'SmokeTest' {
            Write-Step 6 $StepCount 'Running smoke test'
            $exe = Join-Path $configDir 'GhostLightSmokeTest.exe'
            & $exe
            $exitCode = $LASTEXITCODE
            if ($exitCode -ne 0) { Stop-Ghost "Smoke test FAILED (exit code $exitCode). Details above and in Build\SmokeTest.log." }
            Write-Host ''
            Write-Host 'SMOKE TEST PASSED' -ForegroundColor Green
            Write-GhostLog 'SMOKE TEST PASSED'
        }
        default {
            Write-Step 6 $StepCount 'Done'
            Write-Info '  clean rebuild finished'
        }
    }
} catch {
    Write-Host ''
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    Write-GhostLog "ERROR: $($_.Exception.Message)"
    Write-Host 'Full log: Build\run.log'
    if ($exitCode -eq 0) { $exitCode = 1 }
}
exit $exitCode
