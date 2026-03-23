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
    long long lastEndCost = 0;   // 전일 종가 (정수, 원 단위)
    double dailyProfitRate = 0.0;   // 하루 수익률 (전일 종가 대비)
};

struct TradeRecord
{
    std::string tradeDate;              // 거래일자 (trde_dt)
    std::string tradeNo;                // 거래번호 (trde_no)
    std::string stockCode;              // 종목코드 (stk_cd)
    std::string stockName;              // 종목명 (stk_nm)
    std::string ioType;                 // 입출구분 (io_tp)
    std::string ioTypeName;             // 입출구분명 (io_tp_nm)
    std::string tradeQty;               // 거래수량 (trde_qty_jwa_cnt)
    std::string tradeAmt;               // 거래금액 (trde_amt)
    std::string exctAmt;                // 정산금액 (exct_amt)
    std::string commission;             // 수수료 (cmsn)
    std::string taxFee;                 // 세금수수료합 (tax_sum_cmsn)
    std::string tradeUnit;              // 거래단가 (trde_unit)
    std::string procTime;               // 처리시간 (proc_tm)
    std::string creditDealTypeName;     // 신용거래구분명 (crd_deal_tp_nm)
    std::string remarkName;             // 적요명 (rmrk_nm)
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

    // 거래 내역 관련 메소드
    static void RefreshTradeHistory(const std::string& startDate, const std::string& endDate);  // 거래 내역 조회
    static std::vector<TradeRecord> GetTradeHistorySnapshot();                                  // 거래 내역 스냅샷 반환

private:
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);     // 헤더 콜백 함수

    static void AppendHoldingsFromResponse(const nlohmann::json& data, std::map<std::string, Holding>& localHoldings);
	static double UpdateDailyProfitRate(long long currentPrice, long long lastEndCost);                                 // 현재 가격과 전일 종가를 기반으로 하루 수익률 계산

    static std::map<std::string, long long> GetLastEndPriceFromDatabase(const std::set<std::string>& codes);

    static long long GetLongLongField(const nlohmann::json& item, std::initializer_list<const char*> keys);

    static std::set<std::string> accounts;              // 모든 계좌번호를 저장하는 변수ㄴ
    static std::string currentAccountNumber;            // 현재 사용 중인 계좌번호를 저장하는 변수
    static std::map<std::string, Holding> holdings;     // 보유 종목 정보를 저장하는 변수
    static std::vector<TradeRecord> tradeHistory;       // 거래 내역을 저장하는 변수

    static std::mutex holdingsMutex;                    // holdings 접근을 보호하기 위한 뮤텍스
    static std::mutex tradeHistoryMutex;                // tradeHistory 접근을 보호하기 위한 뮤텍스
	static std::mutex cachedCloseMutex;                 // 직전 종가 캐시 접근을 보호하기 위한 뮤텍스

	static std::map<std::string, long long> lastEndPriceByCode;             // 종목코드별 직전 종가를 저장하는 맵 (정수, 원 단위)
	static std::map<std::string, long long> lastEndCostByCode;              // 종목코드별 직전 종가를 저장하는 맵 (정수, 원 단위)
    static constexpr const char* webServerUrl = "http://localhost:4500";
};