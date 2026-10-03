param(
    [ValidateRange(1, 65535)][int]$Port = 8080,
    [switch]$NoBrowser
)
$ErrorActionPreference = 'Stop'
$dsaProjectRoot = $PSScriptRoot
Set-Location -LiteralPath $dsaProjectRoot
try {
    $dsaCompiler = Get-Command g++.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($dsaCompiler) { $dsaCompilerPath = $dsaCompiler.Source }
    else {
        $dsaCompilerPath = @('C:\msys64\ucrt64\bin\g++.exe', 'C:\msys64\mingw64\bin\g++.exe') |
            Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    }
    if (!$dsaCompilerPath) { throw 'Can cai g++ C++17 (MinGW/MSYS2) va them vao PATH truoc khi chay.' }
    $env:PATH = (Split-Path -Parent $dsaCompilerPath) + ';' + $env:PATH
    $dsaBuildRoot = Join-Path $dsaProjectRoot 'build'
    $dsaExecutable = Join-Path $dsaBuildRoot 'dsa_server.exe'
    New-Item -ItemType Directory -Force $dsaBuildRoot | Out-Null
    $dsaSourceFiles = @((Get-ChildItem -LiteralPath $dsaProjectRoot -File | Where-Object { $_.Extension -in '.cpp', '.h' })) +
        @(Get-ChildItem -LiteralPath (Join-Path $dsaProjectRoot 'third_party') -File | Where-Object { $_.Extension -in '.h', '.hpp' })
    $dsaNeedsBuild = !(Test-Path -LiteralPath $dsaExecutable)
    if (!$dsaNeedsBuild) {
        $dsaBuildTime = (Get-Item -LiteralPath $dsaExecutable).LastWriteTimeUtc
        $dsaNeedsBuild = @($dsaSourceFiles | Where-Object { $_.LastWriteTimeUtc -gt $dsaBuildTime }).Count -gt 0
    }
    $dsaUrl = "http://127.0.0.1:$Port"
    $dsaListener = Get-NetTCPConnection -LocalPort $Port -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($dsaListener) {
        $dsaOwner = Get-CimInstance Win32_Process -Filter "ProcessId = $($dsaListener.OwningProcess)" -ErrorAction SilentlyContinue
        if (!$dsaOwner -or $dsaOwner.ExecutablePath -ne $dsaExecutable) {
            throw "Port $Port dang duoc ung dung khac su dung. Thu: run.cmd -Port 8081"
        }
        if ($dsaNeedsBuild) {
            throw "Can build code moi. Dung server cu bang Stop-Process -Id $($dsaListener.OwningProcess), roi chay lai run.cmd."
        }
        $dsaExisting = Invoke-RestMethod "$dsaUrl/api/data" -TimeoutSec 3
        if ($null -eq $dsaExisting.products -or $null -eq $dsaExisting.orders) { throw 'Server dang chay khong tra ve du lieu hop le.' }
        Write-Host "Server da chay: $dsaUrl"
        if (!$NoBrowser) { Start-Process $dsaUrl }
        exit 0
    }
    if ($dsaNeedsBuild) {
        Write-Host 'Dang build server C++... Lan dau co the mat khoang mot phut.'
        $dsaTemporaryExe = Join-Path $dsaBuildRoot 'dsa_server.new.exe'
        & $dsaCompilerPath -std=c++17 -O1 -Wall -Wextra -Wpedantic server.cpp -o $dsaTemporaryExe -lws2_32 -pthread
        if ($LASTEXITCODE -ne 0) { throw 'Build that bai. Kiem tra thong bao compiler o tren.' }
        Move-Item -LiteralPath $dsaTemporaryExe -Destination $dsaExecutable -Force
    }
    $dsaDataRoot = Join-Path $dsaProjectRoot 'data'
    $dsaDataPath = Join-Path $dsaDataRoot 'shop.csv'
    New-Item -ItemType Directory -Force $dsaDataRoot | Out-Null
    if (!(Test-Path -LiteralPath $dsaDataPath)) {
        Copy-Item -LiteralPath (Join-Path $dsaProjectRoot 'sample-data\shop.csv') -Destination $dsaDataPath
    }
    $dsaRunning = $false
    if (!$dsaRunning) {
        $dsaServerProcess = Start-Process -FilePath $dsaExecutable -ArgumentList @('--port', "$Port") -WorkingDirectory $dsaProjectRoot -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $dsaBuildRoot 'server.log') -RedirectStandardError (Join-Path $dsaBuildRoot 'server-error.log')
        for ($dsaAttempt = 0; $dsaAttempt -lt 20; $dsaAttempt++) {
            if ($dsaServerProcess.HasExited) { throw "Server khong khoi dong duoc. Xem build\server-error.log; thu port khac neu port $Port dang ban." }
            try {
                $dsaReady = Invoke-RestMethod "$dsaUrl/api/data" -TimeoutSec 1
                if ($null -ne $dsaReady.products -and $null -ne $dsaReady.orders) { $dsaRunning = $true; break }
            } catch { Start-Sleep -Milliseconds 250 }
        }
        if (!$dsaRunning) { Stop-Process -Id $dsaServerProcess.Id -ErrorAction SilentlyContinue; throw 'Server khong phan hoi. Xem log trong build.' }
        Write-Host "Server PID: $($dsaServerProcess.Id). Dung bang: Stop-Process -Id $($dsaServerProcess.Id)"
    }
    Write-Host "Mo web: $dsaUrl"
    Write-Host 'Du lieu luu tai data\shop.csv. Khong chay console cung ghi CSV khi server dang chay.'
    if (!$NoBrowser) { Start-Process $dsaUrl }
    exit 0
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}
