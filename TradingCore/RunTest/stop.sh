#!/bin/bash

BINARY_NAME=TradingCore.out
PORT=9000

# --- 기존 프로세스 종료 ---
PID=$(pgrep -f "$BINARY_NAME")

if [ -n "$PID" ]; then
    echo "[INFO] 프로세스 종료 중: $PID ($BINARY_NAME)..."
    kill -9 $PID
    sleep 1
    echo "[INFO] 프로세스가 종료되었습니다."
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
    echo "[WARN] 대기 후에도 포트 $PORT 가 아직 사용 중입니다."
else
    echo "[INFO] 포트 $PORT 가 해제되었습니다."
fi

exit 0
