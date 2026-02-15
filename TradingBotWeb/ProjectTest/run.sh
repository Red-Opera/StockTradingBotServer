#!/bin/bash

PROJECT_PATH=/home/ubuntu/Trader/WebTest
JAR_NAME=TradingBotWeb-0.0.1-SNAPSHOT.jar

cd $PROJECT_PATH

# --- 기존 jar 종료 (만약 실행 중이라면) ---
PID=$(pgrep -f $JAR_NAME)
if [ -n "$PID" ]; then
    echo "[INFO] Stopping existing process: $PID"
    kill -9 $PID
    sleep 2
else
    echo "[INFO] No running process found."
fi

# --- 기존 로그 파일 삭제 ---
if [ -f app.log ]; then
    echo "[INFO] Removing old log file..."
    rm -f app.log
fi

# --- Spring Boot 실행 ---
echo "[INFO] Starting Spring Boot application..."
nohup java -jar $JAR_NAME > app.log 2>&1 &
echo "[INFO] Deployment completed. Logs: $PROJECT_PATH/app.log"