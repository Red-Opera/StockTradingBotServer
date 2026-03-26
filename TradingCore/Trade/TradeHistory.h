#pragma once

#include <vector>
#include <string>
#include <mutex>

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

class TradeHistory
{
public:
    static void RefreshTradeHistory(const std::string& startDate, const std::string& endDate);  // 거래 내역 조회
    static std::vector<TradeRecord> GetTradeHistorySnapshot();                                  // 거래 내역 스냅샷 반환

private:
    static std::vector<TradeRecord> tradeHistory;       // 거래 내역을 저장하는 변수

    static std::mutex tradeHistoryMutex;                // tradeHistory 접근을 보호하기 위한 뮤텍스
};

