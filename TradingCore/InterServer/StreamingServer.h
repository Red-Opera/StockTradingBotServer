#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <cstdint>

class StreamingServer
{
public:
    StreamingServer();
    ~StreamingServer();

	// TCP 서버를 시작하여 지정된 포트에서 클라이언트 연결을 수락 (스트리밍 전용)
    bool Start(uint16_t port);

	// 서버를 중지하고 스레드를 조인
    void Stop();

private:
    void AcceptLoop();
    void ClientLoop(int clientSocket);  // JSON 객체를 줄바꿈으로 구분하여 지속적으로 전송하는 메소드

    std::thread acceptThread;
    std::atomic<bool> running;
    uint16_t listenPort;

    int listenSocket; // 플랫폼에 따라 소켓 핸들 타입이 다르므로 int로 통일 (Winsock에서는 SOCKET을 int로 캐스팅)
};
