param(
    [string]$BuildDir = "build-report03-fft-openmp",
    [string]$Config = "Release",
    [int[]]$Threads = @(1, 2, 4, 8),
    [switch]$RunPerformance,
    [switch]$VectorizationReport,
    [switch]$SkipConfigure,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$ResultDir = Join-Path $PSScriptRoot "report_03_results"
New-Item -ItemType Directory -Force -Path $ResultDir | Out-Null

$BuildPath = Join-Path $RepoRoot $BuildDir
$ConfigureLog = Join-Path $ResultDir "configure.log"
$BuildLog = Join-Path $ResultDir "build.log"
$CorrectnessLog = Join-Path $ResultDir "correctness.log"
$PerformanceLog = Join-Path $ResultDir "performance.log"

Write-Host "[Report03] Repository root: $RepoRoot"
Write-Host "[Report03] Build directory: $BuildPath"
Write-Host "[Report03] Result directory: $ResultDir"

$cmakeArgs = @(
    "-S", $RepoRoot,
    "-B", $BuildPath,
    "-DCMAKE_BUILD_TYPE=$Config"
)

if ($VectorizationReport) {
    $cxx = $env:CXX
    if ($cxx -match "clang") {
        $vecFlags = "-O3 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize"
    }
    else {
        $vecFlags = "-O3 -fopt-info-vec-optimized -fopt-info-vec-missed"
    }
    $configUpper = $Config.ToUpperInvariant()
    $cmakeArgs += "-DCMAKE_CXX_FLAGS_$configUpper=$vecFlags"
    Write-Host "[Report03] Vectorization report flags: $vecFlags"
}

if (-not $SkipConfigure) {
    Write-Host "[Report03] Configuring project..."
    & cmake @cmakeArgs 2>&1 | Tee-Object -FilePath $ConfigureLog
}
else {
    Write-Host "[Report03] Skip configure."
}

if (-not $SkipBuild) {
    Write-Host "[Report03] Building MODULE_PW_pw_test..."
    & cmake --build $BuildPath --target MODULE_PW_pw_test --config $Config 2>&1 | Tee-Object -FilePath $BuildLog
}
else {
    Write-Host "[Report03] Skip build."
}

$candidateExe = @(
    (Join-Path $BuildPath "source/source_basis/module_pw/test/MODULE_PW_pw_test.exe"),
    (Join-Path $BuildPath "source/source_basis/module_pw/test/$Config/MODULE_PW_pw_test.exe"),
    (Join-Path $BuildPath "source/source_basis/module_pw/test/MODULE_PW_pw_test"),
    (Join-Path $BuildPath "source/source_basis/module_pw/test/$Config/MODULE_PW_pw_test")
)

$TestExe = $candidateExe | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $TestExe) {
    throw "Cannot find MODULE_PW_pw_test executable under $BuildPath. Please check the build log."
}

Write-Host "[Report03] Test executable: $TestExe"

Write-Host "[Report03] Running correctness tests..."
$env:OMP_PROC_BIND = "close"
$env:OMP_PLACES = "cores"
$maxThread = ($Threads | Measure-Object -Maximum).Maximum
$env:OMP_NUM_THREADS = [string]$maxThread
& $TestExe --gtest_filter="PWTEST.transform_omp_threads_*" 2>&1 | Tee-Object -FilePath $CorrectnessLog

if ($RunPerformance) {
    Write-Host "[Report03] Running disabled performance helper for selected thread counts..."
    "# Report03 FFT OpenMP performance log" | Out-File -FilePath $PerformanceLog -Encoding UTF8
    foreach ($thread in $Threads) {
        Write-Host "[Report03] OMP_NUM_THREADS=$thread"
        $env:OMP_NUM_THREADS = [string]$thread
        "`n## OMP_NUM_THREADS=$thread" | Out-File -FilePath $PerformanceLog -Encoding UTF8 -Append
        & $TestExe --gtest_also_run_disabled_tests --gtest_filter="PWTEST.DISABLED_transform_omp_speedup_report" 2>&1 |
            Tee-Object -FilePath $PerformanceLog -Append
    }
}
else {
    Write-Host "[Report03] Skip performance helper. Add -RunPerformance to enable it."
}

Write-Host "[Report03] Done. Please fill report_03_fft_transform_openmp.md with logs/data from:"
Write-Host "  $ConfigureLog"
Write-Host "  $BuildLog"
Write-Host "  $CorrectnessLog"
if ($RunPerformance) {
    Write-Host "  $PerformanceLog"
}
