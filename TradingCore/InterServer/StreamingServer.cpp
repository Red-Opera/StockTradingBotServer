#include "StreamingServer.h"
#include "../Trade/Account.h"
#include "Core/Log.h"

#include <thread>
#include <chrono>
#include <vector>
#include <sstream>
#include <ctime>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <cstring>
#endif

using namespace std::chrono_literals;

StreamingServer::StreamingServer() : running(false), listenPort(0), listenSocket(-1)
{
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#else
	// SIGPIPE를 무시하여 클라이언트가 연결을 끊었을 때 서버가 종료되지 않도록 함
    signal(SIGPIPE, SIG_IGN);
#endif
}

StreamingServer::~StreamingServer()
{
    Stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool StreamingServer::Start(uint16_t port)
{
    if (running)
        return false;

    listenPort = port;

    // create socket
    listenSocket = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket < 0)
    {
        Log::GetInstance().Output(LogLevel::ERROR, "소켓 생성 실패 (StreamingServer::Start)");
        return false;
    }

	// 소켓 옵션 설정: SO_REUSEADDR을 사용하여 서버가 재시작될 때 "Address already in use" 오류 방지
    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

	// 주소와 포트 바인딩
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(listenPort);

	// 소켓을 주소에 바인딩
    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        Log::GetInstance().Output(LogLevel::ERROR, "바인드 실패 (StreamingServer::Start)");

#ifdef _WIN32
        closesocket(listenSocket);
#else
        close(listenSocket);
#endif

        listenSocket = -1;
        return false;
    }

	// 수신 대기 시작
    if (listen(listenSocket, 4) < 0)
    {
        Log::GetInstance().Output(LogLevel::ERROR, "listen 실패 (StreamingServer::Start)");

#ifdef _WIN32
        closesocket(listenSocket);
#else
        close(listenSocket);
#endif
        listenSocket = -1;

        return false;
    }

    running = true;
    acceptThread = std::thread(&StreamingServer::AcceptLoop, this);
    pollingThread = std::thread(&StreamingServer::RefrashLoop, this);
    tradePollingThread = std::thread(&StreamingServer::TradeHistoryPollingLoop, this);

    std::ostringstream oss;
    oss << "StreamingServer 시작 포트=" << listenPort;

    Log::GetInstance().Output(LogLevel::INFO, oss.str().c_str());

    return true;
}

void StreamingServer::Stop()
{
    if (!running)
        return;

    running = false;

	// accept가 블로킹이므로, 루프를 빠져나오기 위해서 listenSocket을 닫아야 함
    if (listenSocket >= 0)
    {
#ifdef _WIN32
        closesocket(listenSocket);
#else
        close(listenSocket);
#endif
        listenSocket = -1;
    }

    if (acceptThread.joinable())
        acceptThread.join();

    if (pollingThread.joinable())
        pollingThread.join();

    if (tradePollingThread.joinable())
        tradePollingThread.join();

    Log::GetInstance().Output(LogLevel::INFO, "StreamingServer 중지됨");
}

void StreamingServer::AcceptLoop()
{
    while (running)
    {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        // 클라이언트 연결 수락
        int clientSock = (int)accept(listenSocket, (sockaddr*)&clientAddr, &clientLen);

        if (clientSock < 0)
        {
			// 클라이언트 연결 수락 실패; 서버가 중지된 경우 루프 종료, 그렇지 않으면 잠시 대기 후 재시도
            if (!running)
                break;

            Log::GetInstance().Output(LogLevel::WARNING, "accept 실패 (StreamingServer::AcceptLoop)");
            std::this_thread::sleep_for(100ms);

            continue;
        }

        std::ostringstream oss;
        oss << "클라이언트 연결: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port);

        Log::GetInstance().Output(LogLevel::INFO, oss.str().c_str());

		// 각 클라이언트마다 보유 종목을 스트리밍하는 스레드 생성
        std::thread clientThread(&StreamingServer::ClientLoop, this, clientSock);
        clientThread.detach();
    }
}

void StreamingServer::ClientLoop(int clientSocket)
{
    Log& log = Log::GetInstance();

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

    // 각 메시지는 하나의 줄로 구성된 JSON 객체로 보유 종목을 나타냄
    while (running)
    {
		// 현재 보유 종목 스냅샷 가져오기
        auto currentHoldings = Account::GetHoldingsSnapshot();

        // 보유 종목 전송 (type: holding)
        for (const auto& nameHoldingPair : currentHoldings)
        {
            const Holding& holding = nameHoldingPair.second;

			// 클라이언트에게 보낼 JSON 객체 생성 (type 필드 추가)
            std::ostringstream result;
            result  << "{"
                    << "\"type\":\"holding\",";
            result  << "\"account\":\"" << holding.account << "\",";
            result  << "\"code\":\"" << holding.code << "\",";
            result  << "\"name\":\"" << holding.name << "\",";
            result  << "\"quantity\":" << holding.quantity << ",";
            result  << "\"price\":" << holding.price << ",";
            result  << "\"value\":" << holding.value << ",";
            result  << "\"purchasePrice\":" << holding.purchasePrice << ",";
            result  << "\"profitLoss\":" << holding.profitLoss << ",";
            result  << "\"profitRate\":" << holding.profitRate;
            result  << "}\n";

            std::string line = result.str();

#ifdef _WIN32
            int sent = send(clientSocket, line.c_str(), (int)line.size(), 0);

            if (sent <= 0)
            {
                Log::GetInstance().Output(LogLevel::INFO, "클라이언트 연결 끊김 (StreamingServer::ClientLoop)");
                closesocket(clientSocket);
                return;
            }
#else
            ssize_t sent = send(clientSocket, line.c_str(), line.size(), MSG_NOSIGNAL);

            if (sent <= 0)
            {
                int err = errno;
                std::ostringstream ess;
                ess << "send 실패 (StreamingServer::ClientLoop), errno = " << err << ", msg = " << std::strerror(err);
                Log::GetInstance().Output(LogLevel::INFO, ess.str().c_str());
                close(clientSocket);
                return;
            }
#endif
        }

        // 거래 내역 전송 (type: trade)
        auto trades = Account::GetTradeHistorySnapshot();

        for (const auto& trade : trades)
        {
            std::ostringstream result;
            result  << "{"
                    << "\"type\":\"trade\","
                    << "\"tradeDate\":\"" << trade.tradeDate << "\","
                    << "\"tradeNo\":\"" << trade.tradeNo << "\","
                    << "\"stockCode\":\"" << trade.stockCode << "\","
                    << "\"stockName\":\"" << trade.stockName << "\","
                    << "\"ioType\":\"" << trade.ioType << "\","
                    << "\"ioTypeName\":\"" << trade.ioTypeName << "\","
                    << "\"tradeQty\":\"" << trade.tradeQty << "\","
                    << "\"tradeAmt\":\"" << trade.tradeAmt << "\","
                    << "\"exctAmt\":\"" << trade.exctAmt << "\","
                    << "\"commission\":\"" << trade.commission << "\","
                    << "\"taxFee\":\"" << trade.taxFee << "\","
                    << "\"tradeUnit\":\"" << trade.tradeUnit << "\","
                    << "\"procTime\":\"" << trade.procTime << "\","
                    << "\"creditDealTypeName\":\"" << trade.creditDealTypeName << "\","
                    << "\"remarkName\":\"" << trade.remarkName << "\"";
            result  << "}\n";

            std::string line = result.str();

#ifdef _WIN32
            int sent = send(clientSocket, line.c_str(), (int)line.size(), 0);

            if (sent <= 0)
            {
                Log::GetInstance().Output(LogLevel::INFO, "클라이언트 연결 끊김 (StreamingServer::ClientLoop - trade)");
                closesocket(clientSocket);
                return;
            }
#else
            ssize_t sent = send(clientSocket, line.c_str(), line.size(), MSG_NOSIGNAL);

            if (sent <= 0)
            {
                int err = errno;
                std::ostringstream ess;
                ess << "send 실패 (StreamingServer::ClientLoop - trade), errno = " << err << ", msg = " << std::strerror(err);
                Log::GetInstance().Output(LogLevel::INFO, ess.str().c_str());
                close(clientSocket);
                return;
            }
#endif
        }

		// 다음 스냅샷 전까지 잠시 대기; 이 값으로 스트림 속도 조절
        std::this_thread::sleep_for(std::chrono::milliseconds(clientSendIntervalMs));
    }

#ifdef _WIN32
    closesocket(clientSocket);
#else
    close(clientSocket);
#endif
}

void StreamingServer::RefrashLoop()
{
    Log& log = Log::GetInstance();

    log.Output(LogLevel::INFO, "보유 종목 폴링 시작");

    while (running)
    {
        // 현재 시각(로컬 기준) 확인
        std::time_t now = std::time(nullptr);
        std::tm localTime = {};

#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif

        int hour = localTime.tm_hour;

        // 22시 ~ 07시는 장 운영 시간 외 → API 호출 생략, 기존 데이터 유지
        bool isOffHours = (hour >= 22 || hour < 7);

        if (isOffHours)
            log.Output(LogLevel::INFO, "장 운영 시간 외 (22:00~07:00) - API 호출 생략");

        else
            Account::RefreshCurrentHoldings();

        // pollingIntervalMs 동안 10ms 단위로 대기하여 빠르게 종료 신호를 감지
        for (int elapsed = 0; elapsed < pollingIntervalMs && running; elapsed += 10)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    log.Output(LogLevel::INFO, "보유 종목 폴링 종료");
}

void StreamingServer::TradeHistoryPollingLoop()
{
    Log& log = Log::GetInstance();

    log.Output(LogLevel::INFO, "거래 내역 폴링 시작");

    while (running)
    {
        // 현재 시각(로컬 기준) 확인
        std::time_t now = std::time(nullptr);
        std::tm localTime = {};

#ifdef _WIN32
        localtime_s(&localTime, &now);
#else
        localtime_r(&now, &localTime);
#endif

        // 오늘 날짜를 YYYYMMDD 형식으로 생성
        char dateBuf[16];
        std::strftime(dateBuf, sizeof(dateBuf), "%Y%m%d", &localTime);
        std::string today(dateBuf);

        // 1년 전 날짜를 시작일로 설정하여 전체 거래 내역 조회
        std::tm startTime = localTime;
        startTime.tm_year -= 1;
        startTime.tm_mday += 1;
        std::mktime(&startTime);

        char startBuf[16];
        std::strftime(startBuf, sizeof(startBuf), "%Y%m%d", &startTime);
        std::string startDate(startBuf);

        Account::RefreshTradeHistory(startDate, today);

        // tradeHistoryPollingIntervalMs 동안 10ms 단위로 대기하여 빠르게 종료 신호를 감지
        for (int elapsed = 0; elapsed < tradeHistoryPollingIntervalMs && running; elapsed += 10)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    log.Output(LogLevel::INFO, "거래 내역 폴링 종료");
}