#!/bin/bash

PROJECT_PATH=/home/ubuntu/Trader/WebTest
JAR_NAME=TradingBotWeb-0.0.1-SNAPSHOT.jar

cd $PROJECT_PATH

# --- 실행 중인 jar 종료 ---
PID=$(pgrep -f $JAR_NAME)
if [ -n "$PID" ]; then
    echo "[INFO] Stopping process: $PID"
    kill -9 $PID
    sleep 1
    echo "[INFO] Application stopped successfully."
else
    echo "[INFO] No running application found."
fi