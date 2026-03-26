#include "TradeHistory.h"

#include "Account.h"
#include "Login.h"

#include "Core/Log.h"
#include "Core/Config.h"
#include "Core/Network.h"
#include "Utility/Convert.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

using json = nlohmann::json;

std::vector<TradeRecord> TradeHistory::tradeHistory;       // 거래 내역을 저장하는 변수
std::mutex TradeHistory::tradeHistoryMutex;                // tradeHistory 접근을 보호하기 위한 뮤텍스

void TradeHistory::RefreshTradeHistory(const std::string& startDate, const std::string& endDate)
{
    Log& log = Log::GetInstance();
	std::string currentAccountNumber = Account::GetCurrentAccountNumber();

    // 현재 계좌번호 확인
    if (currentAccountNumber.empty())
    {
        log.Output(LogLevel::ERROR, "현재 사용 중인 계좌번호가 설정되지 않았습니다.");

        return;
    }

    // 액세스 토큰 가져오기
    std::string token = Login::GetAccessToken();

    if (token.empty())
    {
        log.Output(LogLevel::ERROR, "접근 토큰이 비어있습니다. 거래 내역 조회를 중단합니다.");

        return;
    }

    // API 엔드포인트 및 URL 설정
    const std::string endpoint = "/api/dostk/acnt";
    const std::string url = std::string(Config::hostURL) + endpoint;

    std::string hasGetNextData = "N";
    std::string nextKey = "";

    const int maxPages = 100;

    // 거래 내역을 임시로 저장할 로컬 벡터 생성
    std::vector<TradeRecord> localTrades;

    std::ostringstream startMsg;
    startMsg << "거래 내역 조회를 시작합니다... (" << startDate << " ~ " << endDate << ")";
    log.Output(LogLevel::INFO, startMsg.str().c_str());

    for (int page = 0; page < maxPages; page++)
    {
        CURL* curl = curl_easy_init();

        if (curl == nullptr)
        {
            log.Output(LogLevel::ERROR, "CURL 초기화 실패 (Account::FetchTradeHistory)");

            break;
        }

        std::string readBuffer;
        std::string headerBuffer;

        struct curl_slist* headers = NULL;

        std::string hdrContentType = "Content-Type: application/json;charset=UTF-8";
        std::string hdrAuth = "authorization: Bearer " + token;
        std::string hdrCont = "cont-yn: " + hasGetNextData;
        std::string hdrNext = "next-key: " + nextKey;
        std::string hdrApiId = "api-id: kt00015";

        headers = curl_slist_append(headers, hdrContentType.c_str());
        headers = curl_slist_append(headers, hdrAuth.c_str());
        headers = curl_slist_append(headers, hdrCont.c_str());
        headers = curl_slist_append(headers, hdrNext.c_str());
        headers = curl_slist_append(headers, hdrApiId.c_str());

        // Request body
        json requestBody =
        {
            { "strt_dt", startDate },
            { "end_dt", endDate },
            { "tp", "0" },
            { "stk_cd", "" },
            { "crnc_cd", "" },
            { "gds_tp", "0" },
            { "frgn_stex_code", "" },
            { "dmst_stex_tp", "%" }
        };

        std::string jsonStr = requestBody.dump();

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Network::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Network::HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerBuffer);

        CURLcode response = curl_easy_perform(curl);

        if (response != CURLE_OK)
        {
            std::string errorMessage = std::string("HTTP 요청 실패 : ") + curl_easy_strerror(response);
            log.Output(LogLevel::ERROR, errorMessage.c_str());

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            break;
        }

        long responseCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

        if (responseCode != 200)
        {
            std::string errorMessage = "거래 내역 조회 비정상 응답 코드 수신 : " + std::to_string(responseCode);
            log.Output(LogLevel::ERROR, errorMessage.c_str());

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            return;
        }

        // 헤더 파싱
        std::istringstream hstream(headerBuffer);
        std::string line;
        std::string hasThisNextData = "N";
        std::string newNext = "";

        while (std::getline(hstream, line))
        {
            JsonData data = Convert::GetJsonData(line);

            if (data.key.empty())
                continue;

            std::string lowerKey = data.key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

            if (lowerKey == "cont-yn")
                hasThisNextData = data.value;

            else if (lowerKey == "next-key")
                newNext = data.value;
        }

        // JSON 파싱
        try
        {
            if (!readBuffer.empty())
            {
                json data = json::parse(readBuffer);

                int returnCode = data.value("return_code", -1);
                std::string returnMsg = data.value("return_msg", "");

                if (returnCode != 0)
                {
                    std::ostringstream oss;
                    oss << "거래 내역 조회 실패: [" << returnCode << "] " << returnMsg;
                    log.Output(LogLevel::ERROR, oss.str().c_str());

                    if (returnCode == 3)
                        Login::ClearAccessToken();

                    curl_slist_free_all(headers);
                    curl_easy_cleanup(curl);
                    return;
                }

                // trst_ovrl_trde_prps_array 배열 파싱
                if (data.contains("trst_ovrl_trde_prps_array") && data["trst_ovrl_trde_prps_array"].is_array())
                {
                    for (auto& item : data["trst_ovrl_trde_prps_array"])
                    {
                        TradeRecord record;
                        record.tradeDate = item.value("trde_dt", "");
                        record.tradeNo = item.value("trde_no", "");
                        record.stockCode = item.value("stk_cd", "");
                        record.stockName = item.value("stk_nm", "");
                        record.ioType = item.value("io_tp", "");
                        record.ioTypeName = item.value("io_tp_nm", "");
                        record.tradeQty = item.value("trde_qty_jwa_cnt", "");
                        record.tradeAmt = item.value("trde_amt", "");
                        record.exctAmt = item.value("exct_amt", "");
                        record.commission = item.value("cmsn", "");
                        record.taxFee = item.value("tax_sum_cmsn", "");
                        record.tradeUnit = item.value("trde_unit", "");

                        // procTime을 HH:MM:SS 형식으로 포맷팅
                        {
                            std::string rawProcTime = item.value("proc_tm", "");
                            std::string digits;
                            for (char c : rawProcTime) {
                                if (std::isdigit(c)) {
                                    digits += c;
                                }
                            }

                            if (digits.length() > 0) {
                                // 6자리 미만이면 앞에 0을 패딩
                                if (digits.length() < 6) {
                                    digits = std::string(6 - digits.length(), '0') + digits;
                                }
                                // HH:MM:SS 형식으로 변환
                                record.procTime = digits.substr(0, 2) + ":" + digits.substr(2, 2) + ":" + digits.substr(4, 2);
                            }
                            else {
                                record.procTime = rawProcTime;
                            }
                        }

                        record.creditDealTypeName = item.value("crd_deal_tp_nm", "");
                        record.remarkName = item.value("rmrk_nm", "");

                        localTrades.push_back(record);

                        std::ostringstream oss;

                        oss << "거래 내역 : " << record.tradeDate << " "
                            << record.stockCode << " (" << record.stockName << ") "
                            << record.ioTypeName << " "
                            << record.tradeQty << "주 "
                            << record.tradeAmt << "원";

                        log.Output(LogLevel::INFO, oss.str().c_str());
                    }
                }
            }
        }

        catch (json::parse_error& e)
        {
            std::string err = std::string("JSON 파싱 오류: ") + e.what();
            log.Output(LogLevel::ERROR, err.c_str());
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        hasGetNextData = hasThisNextData;
        nextKey = newNext;

        if (hasGetNextData != "Y")
            break;
    }

    // 락을 최소화하기 위해 로컬 벡터에 데이터를 모두 채운 후 한 번에 스왑
    {
        std::lock_guard<std::mutex> lock(tradeHistoryMutex);
        tradeHistory.swap(localTrades);
    }

    std::ostringstream summary;
    summary << "거래 내역 조회 완료. 총 " << tradeHistory.size() << "건";

    log.Output(LogLevel::INFO, summary.str().c_str());
}

std::vector<TradeRecord> TradeHistory::GetTradeHistorySnapshot()
{
    std::lock_guard<std::mutex> lock(tradeHistoryMutex);
    return tradeHistory;
}