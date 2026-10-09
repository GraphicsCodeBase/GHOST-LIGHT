# Finds Visual Studio 2022+ with the C++ workload and loads its x64 developer environment into this process only.

function Find-VisualStudio {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) {
        Stop-Ghost "Visual Studio was not found. Install Visual Studio 2022 or newer (Community is fine) with the 'Desktop development with C++' workload. See README -> Requirements."
    }
    # 17.5 is the first VS 2022 release that bundles CMake 3.25 (our minimum).
    $query = Get-CommandOutput ('"' + $vswhere + '" -latest -products * -version [17.5,) -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json -utf8')
    $instances = @(($query.Lines -join "`n") | ConvertFrom-Json)
    if ($instances.Count -eq 0 -or -not $instances[0].installationPath) {
        Stop-Ghost "Visual Studio C++ tools were not found. Open the Visual Studio Installer, click Modify, tick 'Desktop development with C++' and install (VS 2022 17.5 or newer). See README -> Requirements."
    }
    return [pscustomobject]@{
        Path    = $instances[0].installationPath
        Version = $instances[0].installationVersion
        Name    = $instances[0].displayName
    }
}

# Returns VS's bundled cmake.exe or ninja.exe (never a standalone install that happens to be on PATH).
function Get-VsTool($Vs, [string]$Name) {
    $relative = @{
        'cmake' = 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
        'ninja' = 'Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe'
    }[$Name]
    $path = Join-Path $Vs.Path $relative
    if (-not (Test-Path -LiteralPath $path)) {
        Stop-Ghost "Visual Studio's bundled $Name was not found. In the Visual Studio Installer, click Modify and make sure 'C++ CMake tools for Windows' is ticked (part of 'Desktop development with C++')."
    }
    return $path
}

# Runs vcvars64.bat in a child cmd.exe and copies the resulting environment into this PowerShell process.
function Enter-VsDevEnvironment($Vs) {
    $vcvars = Join-Path $Vs.Path 'VC\Auxiliary\Build\vcvars64.bat'
    if (-not (Test-Path -LiteralPath $vcvars)) {
        Stop-Ghost "vcvars64.bat was not found in $($Vs.Path). Repair Visual Studio with the 'Desktop development with C++' workload."
    }
    $result = Get-CommandOutput ('"' + $vcvars + '" >nul 2>&1 && set')
    if ($result.ExitCode -ne 0) {
        Stop-Ghost "Loading the Visual Studio developer environment failed (vcvars64.bat exit code $($result.ExitCode))."
    }
    foreach ($line in $result.Lines) {
        $eq = $line.IndexOf('=')
        if ($eq -gt 0) {
            [System.Environment]::SetEnvironmentVariable($line.Substring(0, $eq), $line.Substring($eq + 1), 'Process')
        }
    }
}

# Removes Vulkan SDK settings from this process so neither the build nor the engine can depend on an SDK install.
function Clear-VulkanSdkEnvironment {
    foreach ($name in 'VULKAN_SDK', 'VK_SDK_PATH', 'VK_LAYER_PATH', 'VK_ADD_LAYER_PATH', 'VK_INSTANCE_LAYERS',
                      'VK_LOADER_LAYERS_ENABLE', 'VK_LOADER_LAYERS_DISABLE', 'VK_DRIVER_FILES', 'VK_ICD_FILENAMES') {
        [System.Environment]::SetEnvironmentVariable($name, $null, 'Process')
    }
    $kept = $env:PATH -split ';' | Where-Object { $_ -and ($_ -notmatch '\\VulkanSDK\\') }
    $env:PATH = $kept -join ';'
}
