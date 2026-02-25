# === 설정 ===

$ServerUser = ""
$ServerIP = ""
$ServerProjectDir = "/home/ubuntu/Trader/TradingCoreTest"                       # 서버 절대 경로
$SshKeyPath = ""
$ProjectRoot = "D:\\Development\\Trader\\StockTradingBotServer\\TradingCore"    # 프로젝트 루트 (C++ 프로젝트)
$RemoteBinaryName = "TradingCore.out"

# === helper: sanitize local shell script (remove BOM and CRLF) ===
function Sanitize-LocalScript([string]$path, [string]$tempPath) 
{
    if (-not (Test-Path $path)) { return $false }
    $raw = Get-Content -Raw -LiteralPath $path -ErrorAction Stop
    $clean = $raw.TrimStart([char]0xFEFF)
    $clean = $clean -replace "`r`n", "`n"
    $clean = $clean -replace "`r", ""
    $enc = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($tempPath, $clean, $enc)
    return $true
}

# === 1. stop.sh 복사 및 실행 ===
$localStopSh = Join-Path $ProjectRoot "RunTest/stop.sh"
$tempStop = [System.IO.Path]::Combine($env:TEMP, "stop.sh.deploy")

if (-not (Test-Path $localStopSh))
{
    Write-Host "[ERROR] Local stop.sh not found at: $localStopSh. Exiting."
    exit 1
}

Write-Host "[INFO] Uploading stop.sh to server..."
if (Sanitize-LocalScript $localStopSh $tempStop)
{
    scp -i "$SshKeyPath" $tempStop "${ServerUser}@${ServerIP}:$ServerProjectDir/stop.sh"
    Remove-Item -Force $tempStop -ErrorAction SilentlyContinue
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Failed to copy stop.sh. Exiting."
        exit 1
    }
}

Write-Host "[INFO] Executing stop.sh on server..."
ssh -i "$SshKeyPath" "${ServerUser}@${ServerIP}" "chmod +x $ServerProjectDir/stop.sh; bash $ServerProjectDir/stop.sh; true"
if ($LASTEXITCODE -ne 0)
{
    Write-Host "[ERROR] Remote stop.sh failed."
    exit 1
}

Write-Host "[INFO] Remote process stopped successfully."
