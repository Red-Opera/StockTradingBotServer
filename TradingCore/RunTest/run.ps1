# === 설정 ===

$ServerUser = ""
$ServerIP = ""
$ServerProjectDir = "/home/ubuntu/Trader/TradingCoreTest"                      # 서버 절대 경로
$SshKeyPath = ""
$ProjectRoot = "D:\\Development\\Trader\\StockTradingBotServer\\TradingCore"    # 프로젝트 루트 (C++ 프로젝트)
$LocalBinaryRelative = "bin\\x64\\Debug\\TradingCore.out"  # 로컬에서 복사할 실행파일 경로 (ProjectRoot 기준)
$RemoteBinaryName = "TradingCore.out"                      # 서버에 복사될 파일명
$RemoteNewSuffix = ".new"                                  # 업로드 시 임시 이름

# 서버로 복사할 RunTest 폴더 내 셸 스크립트 목록 (run.ps1, stop.ps1 제외)
$RunTestScripts = @("run.sh", "stop.sh")

# === 콘솔 출력 인코딩을 UTF-8로 설정 ===
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

# === 0. 로컬에 빌드 산출물이 있는지 확인 ===
Write-Host "[INFO] 로컬 빌드 결과물을 확인하는 중..."
$localBinary = Join-Path $ProjectRoot $LocalBinaryRelative
if (-not (Test-Path $localBinary)) 
{
    Write-Host "[ERROR] 로컬 바이너리를 찾을 수 없습니다: $localBinary"
    Write-Host "[ERROR] 배포 전에 로컬에서 빌드하여 $LocalBinaryRelative 파일을 생성하세요. 종료합니다."
    exit 1
}

# 절대 경로로 변환 (scp 사용을 위해)
$localBinaryFull = (Get-Item $localBinary).FullName

# 서버 경로
$remoteUploadPath = "$ServerProjectDir/$($RemoteBinaryName)$RemoteNewSuffix"
$remoteFinalPath = "$ServerProjectDir/$RemoteBinaryName"

# === 1. 서버의 프로젝트 디렉토리 생성 보장 ===
Write-Host "[INFO] 서버 프로젝트 디렉토리 확인 중..."
ssh -i "$SshKeyPath" "${ServerUser}@${ServerIP}" "mkdir -p $ServerProjectDir"
if ($LASTEXITCODE -ne 0)
{
    Write-Host "[ERROR] 서버 디렉토리 생성에 실패했습니다. 종료합니다."
    exit 1
}

# === 헬퍼: 로컬 셸 스크립트 정리 (BOM 및 CRLF 제거) ===
function Sanitize-LocalScript([string]$path, [string]$tempPath)
{
    if (-not (Test-Path $path)) { return $false }
    $raw = Get-Content -Raw -LiteralPath $path -ErrorAction Stop
    # BOM 제거
    $clean = $raw.TrimStart([char]0xFEFF)
    # 줄 끝 문자를 LF로 정규화
    $clean = $clean -replace "`r`n", "`n"
    $clean = $clean -replace "`r", ""
    # UTF-8 BOM 없이 저장
    $enc = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($tempPath, $clean, $enc)
    return $true
}

# === 2. 로컬 바이너리 전송 (TradingCore.out 단독 복사) ===
Write-Host "[INFO] 바이너리 업로드 중: $localBinaryFull -> ${ServerUser}@${ServerIP}:$remoteUploadPath"
scp -i "$SshKeyPath" "$localBinaryFull" "${ServerUser}@${ServerIP}:$remoteUploadPath"
if ($LASTEXITCODE -ne 0) 
{
    Write-Host "[ERROR] 바이너리 복사에 실패했습니다. 종료합니다."
    exit 1
}

# === 3. RunTest 폴더의 셸 스크립트만 전송 (run.sh, stop.sh) ===
foreach ($script in $RunTestScripts) 
{
    $localScript = Join-Path $ProjectRoot "RunTest\$script"
    $tempScript = [System.IO.Path]::Combine($env:TEMP, "$script.deploy")

    if (Test-Path $localScript) 
    {
        Write-Host "[INFO] $script 업로드 중..."

        if (Sanitize-LocalScript $localScript $tempScript) 
        {
            scp -i "$SshKeyPath" $tempScript "${ServerUser}@${ServerIP}:$ServerProjectDir/$script"
            Remove-Item -Force $tempScript -ErrorAction SilentlyContinue

            if ($LASTEXITCODE -ne 0) 
            {
                Write-Host "[ERROR] $script 복사에 실패했습니다. 종료합니다."
                exit 1
            }
        }
    } 

    else 
    {
        Write-Host "[WARN] 로컬에서 $script 를 찾을 수 없습니다 ($localScript). 건너뜁니다."
    }
}

# === 4. 서버에서 .new -> 최종 파일 교체, 권한 부여 및 run.sh 실행 ===
Write-Host "[INFO] 서버에서 바이너리 교체 및 run.sh 실행 중..."
$remoteCmd = "export LANG=ko_KR.UTF-8; export LC_ALL=ko_KR.UTF-8; if [ -f $remoteUploadPath ]; then mv -f $remoteUploadPath $remoteFinalPath; chmod +x $remoteFinalPath; fi; chmod +x $ServerProjectDir/run.sh 2>/dev/null; chmod +x $ServerProjectDir/stop.sh 2>/dev/null; if [ ! -f $ServerProjectDir/run.sh ]; then echo '[ERROR] 서버에서 run.sh 를 찾을 수 없습니다'; exit 1; fi; bash $ServerProjectDir/run.sh; exit 0"

Write-Host "[INFO] ===== 서버 출력 시작 ====="
$sshOutput = ssh -i "$SshKeyPath" "${ServerUser}@${ServerIP}" "$remoteCmd" 2>&1
$sshOutput | ForEach-Object {
    Write-Host "[SERVER] $_"
}
Write-Host "[INFO] ===== 서버 출력 끝 ====="

# 서버에서 run.sh 가 정상 실행된 경우 SSH 종료코드와 무관하게 배포 성공으로 처리
Write-Host "[INFO] 배포 완료. 서버 로그: $ServerProjectDir/app.log"