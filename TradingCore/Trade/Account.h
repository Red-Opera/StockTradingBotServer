#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>
#include <set>
#include <map>
#include <mutex>

struct Holding
{
    std::string account;            // 계좌번호
    std::string code;               // 종목코드
    std::string name;               // 종목명
    long quantity = 0;              // 보유수량
    long long price = 0;            // 현재가 (정수, 원 단위)
    long long value = 0;            // 평가금액 (정수, 원 단위)
    long long purchasePrice = 0;    // 매입가 (정수, 원 단위)
    long long profitLoss = 0;       // 평가손익 (정수, 원 단위)
    double profitRate = 0.0;        // 수익률 (매입가 대비)
    long long lastEndPrice = 0;     // 전일 종가 (정수, 원 단위)
    double dailyProfitRate = 0.0;   // 하루 수익률 (전일 종가 대비)
};

class Account
{
public:
    static std::set<std::string>& GetAllAccountNumbers();           // 모든 계좌번호를 반환하는 정적 메소드
    static std::string& GetCurrentAccountNumber();                  // 현재 사용 중인 계좌번호 반환

    static bool HasAccount(const std::string& accountNumber);       // 특정 계좌번호가 존재하는지 확인하는 정적 메소드
    static void SetUseAccount();                                    // 실제로 사용할 수 있는 계좌번호 설정

    static void RefreshCurrentHoldings();                           // 현재 보유 종목 정보를 새로고침
    static void ShowHoldings();                                     // 보유 종목 정보를 출력

    // 보유 종목의 스냅샷을 가져오는 메소드
    static std::map<std::string, Holding> GetHoldingsSnapshot();

private:
	static void AppendHoldingsFromResponse(const nlohmann::json& data, std::map<std::string, Holding>& localHoldings);  // API 응답에서 보유 종목 정보를 추출하여 localHoldings에 추가하는 메소드
	static void RefreshDailyProfitRates(std::map<std::string, Holding>& localHoldings);             // 보유 종목들의 하루 수익률을 새로고침하는 메소드
	static double CalculateDailyProfitRate(long long currentPrice, long long lastEndPrice);         // 현재 가격과 전일 종가를 기반으로 하루 수익률 계산

	// 데이터베이스에서 종목코드 목록에 대한 직전 종가를 가져오는 메소드 (다중 코드로 조회하여 효율성 향상)
    static std::map<std::string, long long> GetLastEndPriceFromDatabase(const std::set<std::string>& codes);

    static std::set<std::string> accounts;              // 모든 계좌번호를 저장하는 변수
    static std::string currentAccountNumber;            // 현재 사용 중인 계좌번호를 저장하는 변수
    static std::map<std::string, Holding> holdings;     // 보유 종목 정보를 저장하는 변수

    static std::mutex holdingsMutex;                    // holdings 접근을 보호하기 위한 뮤텍스
	static std::mutex cachedCloseMutex;                 // 직전 종가 캐시 접근을 보호하기 위한 뮤텍스

	static std::map<std::string, long long> lastKnownPriceByCode;            // 종목코드별 최근에 알려진 가격을 저장하는 맵
	static std::map<std::string, long long> lastEndPriceByCode;              // 종목코드별 직전 종가를 저장하는 맵 (정수, 원 단위)
    static constexpr const char* webServerUrl = "http://localhost:4500";
};