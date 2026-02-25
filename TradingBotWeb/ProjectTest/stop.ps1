# === 설정 ===
$ServerUser = ""
$ServerIP = ""
$ServerProjectDir = "/home/ubuntu/Trader/WebTest"   # 서버 절대 경로
$SshKeyPath = ""

# === Spring Boot 앱 종료 ===
Write-Host "[INFO] Stopping Spring Boot application on server..."

ssh -i "$SshKeyPath" "${ServerUser}@${ServerIP}" "bash $ServerProjectDir/stop.sh"

if ($LASTEXITCODE -eq 0) {
    Write-Host "[INFO] Shutdown completed successfully."
} else {
    Write-Host "[ERROR] Failed to stop application."
    exit 1
}