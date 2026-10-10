# Creates Techniques/<Category>/<Name>/ from Techniques/_Template: copies the files and replaces the placeholders
# TechniqueName -> <Name> and TechniqueCategory -> <Category> in file names and contents. Run through new_technique.bat.
param(
    [Parameter(Position = 0)][string]$Category = '',
    [Parameter(Position = 1)][string]$Name = ''
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 3.0
$Root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path

function Stop-NewTechnique([string]$Message) {
    Write-Host "new_technique: $Message" -ForegroundColor Red
    exit 1
}

if (-not $Category -or -not $Name) {
    Write-Host 'Usage: new_technique.bat <Category> <Name>     e.g. new_technique.bat Shadows RayTracedShadows'
    Write-Host 'Categories in use:' ((Get-ChildItem -LiteralPath (Join-Path $Root 'Techniques') -Directory | Where-Object { $_.Name -ne '_Template' } | ForEach-Object { $_.Name }) -join ', ')
    exit 2
}
if ($Category -notmatch '^[A-Z][A-Za-z0-9]*$') { Stop-NewTechnique "category '$Category' must be PascalCase letters and digits, e.g. Shadows" }
if ($Name -notmatch '^[A-Z][A-Za-z0-9]*$') { Stop-NewTechnique "name '$Name' must be a PascalCase C++ class name, e.g. RayTracedShadows" }

# The technique's shader is Shaders/<Name>.slang; a ShaderLibrary module of the same name would be shadowed by it.
$libraryModules = @(Get-ChildItem -LiteralPath (Join-Path $Root 'ShaderLibrary') -Filter '*.slang' | ForEach-Object { $_.BaseName })
if ($libraryModules -contains $Name) { Stop-NewTechnique "'$Name' is a ShaderLibrary module name; pick another name" }

# Class names must be unique across all categories (they all link into one executable).
$existing = @(Get-ChildItem -LiteralPath (Join-Path $Root 'Techniques') -Recurse -File -Filter "$Name.cpp")
if ($existing.Count -gt 0) { Stop-NewTechnique "a technique named '$Name' already exists: $($existing[0].FullName.Substring($Root.Length + 1))" }

$template = Join-Path $Root 'Techniques\_Template'
$target = Join-Path $Root "Techniques\$Category\$Name"
if (Test-Path -LiteralPath $target) { Stop-NewTechnique "Techniques\$Category\$Name already exists" }

$utf8 = New-Object System.Text.UTF8Encoding($false)
foreach ($file in Get-ChildItem -LiteralPath $template -Recurse -File) {
    $relative = $file.FullName.Substring($template.Length + 1).Replace('TechniqueName', $Name)
    $destination = Join-Path $target $relative
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    $text = [System.IO.File]::ReadAllText($file.FullName).Replace('TechniqueName', $Name).Replace('TechniqueCategory', $Category)
    [System.IO.File]::WriteAllText($destination, $text, $utf8)
}

Write-Host "Created Techniques\$Category\$Name\" -ForegroundColor Green
Write-Host "  $Name.cpp             the technique: passes, params (starts as a simple tint effect)"
Write-Host "  Shaders\$Name.slang   its compute shader"
Write-Host "  README.md             the write-up template"
Write-Host 'Next: run.bat (the build finds the new folder), then enable it in the Techniques panel (F2).'
