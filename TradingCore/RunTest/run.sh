# This script runs the built C++ binary on the server. It assumes the binary is copied to this directory.

PROJECT_PATH=~/Trader/TradingCoreTest
BINARY_NAME=TradingCore.out
PORT=9000

cd "$PROJECT_PATH" || exit 1

# --- 기존 프로세스 종료 ---
PID=$(pgrep -f "$BINARY_NAME")

if [ -n "$PID" ]; then
    echo "[INFO] 기존 프로세스 종료 중: $PID ($BINARY_NAME)..."
    kill -9 $PID
    sleep 1
else
    echo "[INFO] 실행 중인 프로세스가 없습니다."
fi

# --- 포트 해제 대기 (최대 5초) ---
for i in $(seq 1 5); do
    if ! ss -tlnp | grep -q ":$PORT"; then
        break
    fi
    echo "[INFO] 포트 $PORT 해제 대기 중... ($i/5)"
    sleep 1
done

if ss -tlnp | grep -q ":$PORT"; then
    echo "[ERROR] 포트 $PORT 가 아직 사용 중입니다. 새 인스턴스를 시작할 수 없습니다."
    exit 1
fi

# --- 기존 로그 파일 삭제 ---
if [ -f app.log ]; then
    echo "[INFO] 기존 로그 파일을 삭제합니다..."
    rm -f app.log
fi

# --- 바이너리 실행 ---
if [ ! -f "$BINARY_NAME" ]; then
    echo "[ERROR] $PROJECT_PATH 에서 $BINARY_NAME 을 찾을 수 없습니다."
    exit 1
fi

chmod +x "$BINARY_NAME"

echo "[INFO] C++ 애플리케이션을 시작합니다..."
nohup ./$BINARY_NAME > app.log 2>&1 &

echo "[INFO] 배포 완료. 로그: $PROJECT_PATH/app.log"
exit 0
