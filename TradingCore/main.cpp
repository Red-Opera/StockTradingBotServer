#include "Login.h"

#include "Core/Application.h"
#include "Trade/Account.h"
#include "Core/Config.h"
#include "InterServer/StreamingServer.h"

#include <atomic>
#include <csignal>
#include <thread>
#include <chrono>

// 프로세스 종료 요청 플래그
static std::atomic<bool> gTerminate(false);

static void SignalHandler(int)
{
    gTerminate.store(true);
}

int main()
{
    Application& app = Application::GetInstance();
    app.Initialize();

    Account::SetUseAccount();           // 계좌 설정
    Account::RefreshCurrentHoldings();  // 잔고 조회
    Account::ShowHoldings();            // 출력

    // 스트리밍 서버 시작 (포트는 필요에 따라 변경)
    StreamingServer server;
    server.Start(9000);

    // 시그널 핸들러 등록
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // 종료 요청이 올 때까지 대기 (안전한 방식)
    while (!gTerminate.load())
        std::this_thread::sleep_for(std::chrono::seconds(1));

    // 종료 처리
    server.Stop();

    return 0;
}