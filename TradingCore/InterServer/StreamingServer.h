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
    void RefrashLoop();                 // 주기적으로 REST API를 호출하여 보유 종목을 갱신하는 메소드

    std::thread acceptThread;
    std::thread pollingThread;          // 폴링 스레드
    std::atomic<bool> running;
    uint16_t listenPort;

    int listenSocket;                   // 플랫폼에 따라 소켓 핸들 타입이 다르므로 int로 통일 (Winsock에서는 SOCKET을 int로 캐스팅)

    static constexpr int pollingIntervalMs = 500;       // REST API 폴링 주기 (밀리초)
    static constexpr int clientSendIntervalMs = 500;    // 클라이언트 전송 주기 (밀리초)
};
