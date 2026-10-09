# Maintainer tool (not used by run.bat): builds the pinned Vulkan validation layer from source and packages
# the zip that is mirrored on GHOST-LIGHT's GitHub Releases. Needs the VS C++ workload and Python 3 on PATH
# (SPIRV-Tools generates code with it). Takes about 15 minutes. Usage: powershell -File Scripts\BuildValidationLayers.ps1
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
Set-StrictMode -Version 3.0

$Root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'Common.ps1')
. (Join-Path $PSScriptRoot 'Downloads.ps1')
. (Join-Path $PSScriptRoot 'VisualStudio.ps1')

$SourcesManifest = Join-Path $PSScriptRoot 'ValidationLayersSources.json'
$LayerVersion = (Get-Content -LiteralPath $SourcesManifest -Raw | ConvertFrom-Json).layerVersion
$Work = Join-Path $Root 'Build\ValidationLayers'
$Src = Join-Path $Work 'src'
$Install = Join-Path $Work 'install'
# Big generated files make each compiler process hungry; 8 jobs keeps a 16 GB machine out of swap.
$Jobs = [Math]::Min(8, [Environment]::ProcessorCount)
$StepCount = 5

# CMake reads -D paths as CMake strings, where a backslash starts an escape: always pass forward slashes.
function ConvertTo-CMakePath([string]$Path) { return $Path.Replace('\', '/') }

# Configures, builds and installs one CMake project into the shared install prefix (Release, static CRT).
function Build-CMakeProject([string]$Name, [string[]]$Options) {
    $source = Join-Path $Src $Name
    $buildDir = Join-Path $Work "build\$Name"
    Write-Info "  $Name"
    $configure = @('-S', $source, '-B', $buildDir, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release',
                   "-DCMAKE_MAKE_PROGRAM=$(ConvertTo-CMakePath $script:Ninja)", "-DCMAKE_INSTALL_PREFIX=$(ConvertTo-CMakePath $Install)", "-DCMAKE_PREFIX_PATH=$(ConvertTo-CMakePath $Install)",
                   '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded', '-DCMAKE_POLICY_DEFAULT_CMP0091=NEW') + $Options
    foreach ($step in @($configure, @('--build', $buildDir, '--parallel', "$Jobs"), @('--install', $buildDir))) {
        $code = Invoke-Logged $script:CMake $step -Quiet
        if ($code -ne 0) {
            Write-Host ''
            Get-Content -LiteralPath (Join-Path $Work 'build.log') -Tail 40 | ForEach-Object { Write-Host $_ }
            Stop-Ghost "Building $Name failed (last lines above, full log in Build\ValidationLayers\build.log)."
        }
    }
}

try {
    Initialize-GhostLog (Join-Path $Work 'build.log')
    Write-Info "Building Vulkan validation layers $LayerVersion (Release, static CRT) in Build\ValidationLayers"

    Write-Step 1 $StepCount 'Checking tools'
    $vs = Find-VisualStudio
    Write-Info "  $($vs.Name) $($vs.Version)"
    $python = (Get-Command python -ErrorAction SilentlyContinue | Select-Object -First 1)
    if (-not $python -or $python.Source -match 'WindowsApps') {
        Stop-Ghost 'Python 3 is needed to build SPIRV-Tools (maintainers only). Install it from python.org and make sure "python" is on PATH.'
    }
    Write-Info "  Python: $($python.Source)"
    Enter-VsDevEnvironment $vs
    Clear-VulkanSdkEnvironment
    $script:CMake = Get-VsTool $vs 'cmake'
    $script:Ninja = Get-VsTool $vs 'ninja'

    Write-Step 2 $StepCount 'Fetching pinned sources'
    Install-Manifest $SourcesManifest 'sources' $Root $false

    Write-Step 3 $StepCount 'Building dependencies'
    Build-CMakeProject 'Vulkan-Headers' @()
    Build-CMakeProject 'Vulkan-Utility-Libraries' @('-DBUILD_TESTS=OFF', '-DUPDATE_DEPS=OFF')
    Build-CMakeProject 'SPIRV-Headers' @('-DSPIRV_HEADERS_ENABLE_TESTS=OFF')
    Build-CMakeProject 'SPIRV-Tools' @("-DSPIRV-Headers_SOURCE_DIR=$(ConvertTo-CMakePath (Join-Path $Src 'SPIRV-Headers'))", '-DSPIRV_WERROR=OFF',
                                       '-DSPIRV_SKIP_TESTS=ON', '-DSPIRV_SKIP_EXECUTABLES=ON', "-DPython3_EXECUTABLE=$(ConvertTo-CMakePath $python.Source)")
    # MI_OVERRIDE=OFF: the layer defines its own operator new/delete on top of mimalloc; a second set fails to link.
    Build-CMakeProject 'mimalloc' @('-DMI_BUILD_STATIC=ON', '-DMI_BUILD_OBJECT=OFF', '-DMI_BUILD_SHARED=OFF', '-DMI_BUILD_TESTS=OFF', '-DMI_OVERRIDE=OFF')

    Write-Step 4 $StepCount 'Building the validation layer (the long part)'
    Build-CMakeProject 'Vulkan-ValidationLayers' @('-DUPDATE_DEPS=OFF', '-DBUILD_TESTS=OFF', '-DBUILD_WERROR=OFF')

    Write-Step 5 $StepCount 'Packaging'
    $package = Join-Path $Work 'package'
    if (Test-Path -LiteralPath $package) { Remove-Item -LiteralPath $package -Recurse -Force }
    New-Item -ItemType Directory -Path (Join-Path $package 'LICENSES') -Force | Out-Null
    foreach ($file in 'VkLayer_khronos_validation.dll', 'VkLayer_khronos_validation.json') {
        $found = Get-ChildItem -LiteralPath $Install -Recurse -Filter $file | Select-Object -First 1
        if (-not $found) { Stop-Ghost "$file was not produced by the build." }
        Copy-Item -LiteralPath $found.FullName -Destination $package
    }
    foreach ($name in 'Vulkan-ValidationLayers', 'Vulkan-Utility-Libraries', 'SPIRV-Tools', 'SPIRV-Headers', 'mimalloc') {
        $license = Get-ChildItem -LiteralPath (Join-Path $Src $name) -File | Where-Object { $_.Name -match '^LICENSE' } | Select-Object -First 1
        if ($license) { Copy-Item -LiteralPath $license.FullName -Destination (Join-Path $package "LICENSES\$name-$($license.Name)") }
    }
    $sources = (Get-Content -LiteralPath $SourcesManifest -Raw | ConvertFrom-Json).sources
    $provenance = @("Vulkan validation layer $LayerVersion for Windows x64, built by GHOST LIGHT's Scripts/BuildValidationLayers.ps1",
                    "(Release, static MSVC runtime, $($vs.Name) $($vs.Version)). Sources:") +
                  ($sources | ForEach-Object { "  $($_.name) $($_.version)  sha256 $($_.sha256)" })
    Set-Content -LiteralPath (Join-Path $package 'README.txt') -Value $provenance -Encoding ASCII

    $zipName = "validation-layers-$LayerVersion-win64.zip"
    $zip = Join-Path $Work $zipName
    if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
    Compress-Archive -Path (Join-Path $package '*') -DestinationPath $zip
    $hash = Get-Sha256 $zip
    $size = (Get-Item -LiteralPath $zip).Length

    # Seed run.bat's download cache so this machine can install the layer before the release exists.
    $cacheDir = Join-Path $Root '.tools\downloads'
    if (-not (Test-Path -LiteralPath $cacheDir)) { New-Item -ItemType Directory -Path $cacheDir -Force | Out-Null }
    Copy-Item -LiteralPath $zip -Destination (Join-Path $cacheDir "validation-layers--$zipName") -Force

    Write-Info ''
    Write-Info "Package: Build\ValidationLayers\$zipName"
    Write-Info "  sha256    $hash"
    Write-Info "  sizeBytes $size"
    Write-Info "Next: put these in the validation-layers entry of Scripts\Dependencies.json, then upload the zip to"
    Write-Info "GitHub -> Releases -> new release with tag deps-validation-layers-$LayerVersion (asset name unchanged)."
} catch {
    Write-Host ''
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    Write-GhostLog "ERROR: $($_.Exception.Message)"
    exit 1
}
