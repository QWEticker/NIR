<#
.SYNOPSIS
  Build the project, run tests, produce JUnit XML, HTML and CSV reports.

.DESCRIPTION
  Usage (from repository root):
    pwsh .\tools\run_tests_and_report.ps1
  Optional parameters:
    -BuildDir   (default: build)
    -Config     (default: Debug)
    -OutDir     (default: build/test_reports)
    -CleanBuild (switch) remove previous build dir before configure

  Requirements:
    - cmake in PATH
    - a C++ compiler / generator for CMake
    - python (py or python) for junit2html and CSV script

#>

param(
    [string]$BuildDir = "build",
    [string]$Config = "Debug",
    [string]$OutDir = "build/test_reports",
    [switch]$CleanBuild
)

$ErrorActionPreference = "Stop"

function Find-Python {
    # Prefer 'py' launcher, then 'python'
    if (Get-Command py -ErrorAction SilentlyContinue) {
        return @{ Cmd='py'; Args='-3' }
    } elseif (Get-Command python -ErrorAction SilentlyContinue) {
        return @{ Cmd='python'; Args='' }
    } else {
        throw "Python not found in PATH. Install Python 3 and retry."
    }
}

function Invoke-ProcessChecked([string]$exe, [string[]]$arguments) {
    Write-Host "Running: $exe $($arguments -join ' ')"
    & $exe @arguments
    $rc = $LASTEXITCODE
    if ($rc -ne 0) {
        throw "Command failed with exit code ${rc}: ${exe} $($arguments -join ' ')"
    }
    return $rc
}

$RepoRoot = (Get-Location).Path
$BuildPath = Join-Path $RepoRoot $BuildDir
$OutPath   = Join-Path $RepoRoot $OutDir

Write-Host "Repository root: $RepoRoot"
Write-Host "Build dir: $BuildPath"
Write-Host "Output reports dir: $OutPath"
Write-Host "Config: $Config"

if ($CleanBuild -and (Test-Path $BuildPath)) {
    Write-Host "Removing existing build directory..."
    Remove-Item -Recurse -Force $BuildPath
}

if (-not (Test-Path $BuildPath)) {
    New-Item -ItemType Directory -Path $BuildPath | Out-Null
}

# 1) Configure with CMake
Push-Location $BuildPath
try {
    $cmakeCmd = "cmake"
    $cmakeArgs = @("-DCVQKD_BUILD_TESTS=ON", "..")
    Invoke-ProcessChecked $cmakeCmd $cmakeArgs} catch {
    Pop-Location
    throw
}

# 2) Build test target and simulator
try {
    Write-Host "Building cvqkd_tests..."
    Invoke-ProcessChecked "cmake" @("--build", ".", "--config", $Config, "--target", "cvqkd_tests")    Write-Host "Building cvqkd_sim..."
    Invoke-ProcessChecked $cmakeCmd @("--build", ".", "--config", $Config, "--target", "cvqkd_sim")
} catch {
    # try building ALL_BUILD as fallback
    Write-Warning "Targeted build failed; attempting full build..."
    Invoke-ProcessChecked $cmakeCmd @("--build", ".", "--config", $Config, "--target", "ALL_BUILD")
}

# 3) Locate cvqkd_tests executable
Write-Host "Searching for cvqkd_tests executable..."
$testExe = Get-ChildItem -Path $BuildPath -Recurse -File -Include "cvqkd_tests.exe","cvqkd_tests" -ErrorAction SilentlyContinue | Select-Object -First 1

if (-not $testExe) {
    Pop-Location
    throw "cvqkd_tests executable not found under $BuildPath. Check build logs."
}
$testExePath = $testExe.FullName
Write-Host "Found test executable: $testExePath"

# ensure output dir exists
Pop-Location
if (-not (Test-Path $OutPath)) { New-Item -ItemType Directory -Path $OutPath | Out-Null }

# 4) Run tests and write GoogleTest JUnit XML
$xmlPath = Join-Path $OutPath "test_results.xml"
Write-Host "Running tests and writing XML to $xmlPath"

# Some GTest builds require passing --gtest_output=xml:<file>
# Use & to execute the test binary
# NOTE: tests may fail (exit code 1), but we still want to generate reports
Write-Host "Running: $testExePath --gtest_output=xml:$xmlPath"
& $testExePath "--gtest_output=xml:$xmlPath"
$testExitCode = $LASTEXITCODE
if ($testExitCode -ne 0) {
    Write-Warning "Tests finished with exit code $testExitCode (some tests may have failed). Reports will still be generated."
}

# 5) Convert XML -> HTML using junit2html (python module)
$py = Find-Python
$pyCmd = $py.Cmd
$pyArgs = $py.Args

# Install junit2html if missing
Write-Host "Ensuring junit2html is installed..."
try {
    & $pyCmd $pyArgs -m pip show junit2html > $null 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "junit2html not found; installing..."
        & $pyCmd $pyArgs -m pip install --user junit2html
    } else {
        Write-Host "junit2html found."
    }
} catch {
    Write-Warning "pip install failed; continuing, but HTML gen may fail."
}

$htmlPath = Join-Path $OutPath "test_results.html"
Write-Host "Generating HTML report: $htmlPath"
try {
    & $pyCmd $pyArgs -m junit2html $xmlPath $htmlPath
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "junit2html failed. Install with: py -m pip install junit2html"
    } else {
        Write-Host "HTML report generated: $htmlPath"
    }
} catch {
    Write-Warning "junit2html error: $_"
    Write-Warning "Install with: py -m pip install junit2html"
}

# 6) Convert XML -> CSV using tools/junit2csv.py (included separately)
$csvPath = Join-Path $OutPath "test_results.csv"
$csvScript = Join-Path (Join-Path $RepoRoot "tools") "junit2csv.py"
if (-not (Test-Path $csvScript)) {
    Write-Warning "CSV converter script not found at $csvScript. Skipping CSV generation."
} else {
    Write-Host "Generating CSV report: $csvPath"
    Invoke-ProcessChecked $pyCmd @($pyArgs, $csvScript, $xmlPath, $csvPath)
}

# 7) Print summary and open HTML
Write-Host ""
Write-Host "Reports generated:"
Write-Host " - XML:  $xmlPath"
Write-Host " - HTML: $htmlPath"
if (Test-Path $csvPath) { Write-Host " - CSV:  $csvPath" }

# open HTML in default browser (best-effort)
if (Test-Path $htmlPath) {
    Write-Host "Opening HTML report..."
    Start-Process $htmlPath
}

Write-Host "`nDone."