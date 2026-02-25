# === 설정 ===
$ServerUser = ""
$ServerIP = ""
$ServerProjectDir = "/home/ubuntu/Trader/WebTest"                           # 서버 절대 경로
$SshKeyPath = ""
$ProjectRoot = "D:\Development\Trader\StockTradingBotServer\TradingBotWeb"  # 프로젝트 루트

# === 0. 로컬에서 프로젝트 빌드 ===
Write-Host "[INFO] Building project locally..."
Set-Location $ProjectRoot
./gradlew build

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Local build failed. Exiting."
    exit 1
}

# === 1. 서버의 기존 프로젝트 디렉토리 완전 삭제 및 재생성 ===
Write-Host "[INFO] Removing old project directory on server..."
ssh -i "$SshKeyPath" "${ServerUser}@${ServerIP}" "rm -rf $ServerProjectDir && mkdir -p $ServerProjectDir"

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Failed to clean server directory. Exiting."
    exit 1
}

# === 2. 전체 프로젝트를 서버로 전송 ===
Write-Host "[INFO] Copying entire project to server..."
scp -i "$SshKeyPath" -r "$ProjectRoot/*" "${ServerUser}@${ServerIP}:${ServerProjectDir}/"

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Failed to copy project. Exiting."
    exit 1
}

# === 3. 필수 파일들 개별 복사 (덮어쓰기 보장) ===
Write-Host "[INFO] Ensuring critical files are copied..."
scp -i "$SshKeyPath" "$ProjectRoot/ProjectTest/run.sh" "${ServerUser}@${ServerIP}:${ServerProjectDir}/"
scp -i "$SshKeyPath" "$ProjectRoot/ProjectTest/stop.sh" "${ServerUser}@${ServerIP}:${ServerProjectDir}/"
scp -i "$SshKeyPath" "$ProjectRoot/build/libs/*.jar" "${ServerUser}@${ServerIP}:${ServerProjectDir}/"

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Failed to copy critical files. Exiting."
    exit 1
}

# === 4. 서버에서 실행 권한 부여 및 run.sh 실행 ===
Write-Host "[INFO] Setting execute permissions and running run.sh on server..."
ssh -i "$SshKeyPath" "${ServerUser}@${ServerIP}" "chmod +x $ServerProjectDir/run.sh $ServerProjectDir/stop.sh && bash $ServerProjectDir/run.sh"

Write-Host "[INFO] Deployment completed. Check logs at $ServerProjectDir/app.log on server."