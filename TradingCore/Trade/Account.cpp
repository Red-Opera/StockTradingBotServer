#include "Account.h"
#include "Login.h"

#include "Core/Config.h"
#include "Core/Log.h"
#include "Core/Network.h"
#include "Utility/Convert.h"
#include "Utility/DateTime.h"
#include "Utility/String.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>

using json = nlohmann::json;

std::set<std::string> Account::accounts;
std::string Account::currentAccountNumber;
std::map<std::string, Holding> Account::holdings;
std::mutex Account::holdingsMutex;

std::mutex Account::cachedCloseMutex;
std::map<std::string, long long> Account::lastKnownPriceByCode;
std::map<std::string, long long> Account::lastEndPriceByCode;

std::set<std::string>& Account::GetAllAccountNumbers()
{
    Log& log = Log::GetInstance();
    
	// 이미 계좌번호를 가져온 경우 재사용
    if (!accounts.empty())
		return accounts;

	// 키움 증권의 Access Token 가져오기
    std::string token = Login::GetAccessToken();

    if (token.empty())
    {
		log.Output(LogLevel::ERROR, "접근 토큰이 비어있습니다. 계좌 조회를 중단합니다.");

        return accounts;
    }

	const std::string endpoint = "/api/dostk/acnt";                     // 계좌번호 조회 API 엔드포인트
	const std::string url = std::string(Config::hostURL) + endpoint;    // 전체 URL

	std::string hasGetNextData = "N";   // 다음에 가져올 데이터가 있는지 저장하는 변수
	std::string nextKey = "";           // 다음 데이터를 가져오기 위한 키

	// 최대 50페이지까지 반복
    const int maxPages = 50;

    for (int page = 0; page < maxPages; page++)
    {
        CURL* curl = curl_easy_init();

        if (curl == nullptr)
        {
            log.Output(LogLevel::ERROR, "CURL 초기화 실패 (Account::GetAllAccountNumbers)");

            break;
        }

		// Request에 필요한 헤더 설정
        std::string readBuffer;
        std::string headerBuffer;

        struct curl_slist* headers = NULL;

        std::string hdrContentType = "Content-Type: application/json;charset=UTF-8";
        std::string hdrAuth = "authorization: Bearer " + token;
        std::string hdrCont = "cont-yn: " + hasGetNextData;
        std::string hdrNext = "next-key: " + nextKey;
        std::string hdrApiId = "api-id: ka00001";

        headers = curl_slist_append(headers, hdrContentType.c_str());
        headers = curl_slist_append(headers, hdrAuth.c_str());
        headers = curl_slist_append(headers, hdrCont.c_str());
        headers = curl_slist_append(headers, hdrNext.c_str());
        headers = curl_slist_append(headers, hdrApiId.c_str());

		// curl 옵션 설정
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{}");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Network::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, Network::HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headerBuffer);

		// HTTP POST 요청 수행
        CURLcode response = curl_easy_perform(curl);

        if (response != CURLE_OK)
        {
            std::string errorMessage = std::string("HTTP 요청 실패 : ") + curl_easy_strerror(response);
            log.Output(LogLevel::ERROR, errorMessage.c_str());

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            break;
        }

		// 응답 상태 코드 가져오기
        long responseCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

		// 비정상 응답이 들어올 경우 처리
        if (responseCode != 200)
        {
            std::string errorMessage = "계좌 번호 호출 비정상 응답 코드 수신 : " + std::to_string(responseCode);

            log.Output(LogLevel::ERROR, errorMessage.c_str());

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            break;
		}

		// 가져온 헤더를 가지고 contYn, nextKey 파싱
        std::istringstream hstream(headerBuffer);
        std::string line;
		std::string hasThisNextData = "N";      // 이번 응답에서 다음 데이터가 있는지 여부
        std::string newNext = "";

		// 데이터를 한 줄씩 읽으면서 파싱
        while (std::getline(hstream, line))
        {
			JsonData data = Convert::GetJsonData(line);

            if (data.key.empty())
				continue;

			// 소문자로 변환하여 키 비교
            std::string lowerKey = data.key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

            if (lowerKey == "cont-yn")
                hasThisNextData = data.value;

            else if (lowerKey == "next-key")
                newNext = data.value;
        }

		// JSON 파싱 및 계좌번호 추출
        try
        {
            if (!readBuffer.empty())
            {
                json data = json::parse(readBuffer);

                // 만약 응답에 acctNo 키가 있고, 그 값이 문자열일 경우
                if (data.contains("acctNo") && data["acctNo"].is_string())
                {
                    std::string account = data["acctNo"].get<std::string>();

                    if (!account.empty())
                        accounts.insert(account);
                }

                // 만약 응답에 acctNo 키가 있고, 그 값이 배열일 경우
                else if (data.contains("acctNo") && data["acctNo"].is_array())
                {
                    for (auto& accountElement : data["acctNo"])
                    {
                        if (accountElement.is_string())
                            accounts.insert(accountElement.get<std::string>());
                    }
                }

                // 그 외에 데이터 구조에 대해서 보수적으로 스캔
                else
                {
                    // 모든 키-값 쌍을 순회
                    for (auto iter = data.begin(); iter != data.end(); ++iter)
                    {
                        std::string key = iter.key();   // 키 가져오기
                        std::string lk = key;           // 소문자로 변환

                        std::transform(lk.begin(), lk.end(), lk.begin(), ::tolower);

                        // 계좌 관련 키인지 확인
                        if (lk.find("acct") != std::string::npos || lk.find("account") != std::string::npos)
                        {
                            if (iter.value().is_string())
                                accounts.insert(iter.value().get<std::string>());

                            else if (iter.value().is_array())
                            {
                                for (auto& accountElement : iter.value())
                                {
                                    if (accountElement.is_string())
                                        accounts.insert(accountElement.get<std::string>());
                                }
                            }
                        }
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

		// hasGetNextData, nextKey 업데이트
        hasGetNextData = hasThisNextData;
        nextKey = newNext;

		// 더 이상 데이터가 없으면 종료
        if (hasGetNextData != "Y")
            break;
    }

    return accounts;
}

std::string& Account::GetCurrentAccountNumber()
{
    return currentAccountNumber;
}

bool Account::HasAccount(const std::string& accountNumber)
{
    if (accounts.empty())
        GetAllAccountNumbers();

    return accounts.find(accountNumber) != accounts.end();
}

void Account::SetUseAccount()
{
    Log& log = Log::GetInstance();

	// 설정한 계좌번호가 있으면 해당 계좌번호로 설정
    if (HasAccount(Config::accountNum))
    {
		currentAccountNumber = Config::accountNum;

		log.Output(LogLevel::INFO, ("지정된 계좌번호 " + std::string(Config::accountNum) + "(으)로 설정합니다.").c_str());

        return;
    }

	log.Output(LogLevel::WARNING, "지정된 계좌번호를 찾을 수 없습니다. 기본 계좌번호로 설정합니다.");

    // 계좌번호가 없으면 첫 번째 계좌번호를 사용하도록 설정
	std::set<std::string> allAccounts = GetAllAccountNumbers();

    if (!allAccounts.empty())
		currentAccountNumber = *allAccounts.begin();

	log.Output(LogLevel::INFO, ("기본 계좌번호 " + currentAccountNumber + "(으)로 설정합니다.").c_str());
}

void Account::RefreshCurrentHoldings()
{
    Log& log = Log::GetInstance();
    std::string token = Login::GetAccessToken();

    if (token.empty())
    {
        log.Output(LogLevel::ERROR, "접근 토큰이 비어있습니다. 보유 종목 조회를 중단합니다.");

        return;
    }

    std::string endpoint = "/api/dostk/acnt";
    std::string url = std::string(Config::hostURL) + endpoint;
    std::string hasGetNextData = "N";
    std::string nextKey = "";

    const int maxPages = 50;

	// 홀딩 정보를 임시로 저장할 로컬 맵 생성 (락 지속 시간 최소화)
    std::map<std::string, Holding> localHoldings;

    log.Output(LogLevel::INFO, "현재 보유 종목 조회를 시작합니다...");

    for (int page = 0; page < maxPages; page++)
    {
        CURL* curl = curl_easy_init();

        if (curl == nullptr)
        {
            log.Output(LogLevel::ERROR, "CURL 초기화 실패 (Account::FetchCurrentHoldings)");

            break;
        }

        std::string readBuffer;
        std::string headerBuffer;

        struct curl_slist* headers = NULL;

        std::string hdrContentType = "Content-Type: application/json;charset=UTF-8";
        std::string hdrAuth = "authorization: Bearer " + token;
        std::string hdrCont = "cont-yn: " + hasGetNextData;
        std::string hdrNext = "next-key: " + nextKey;
        std::string hdrApiId = "api-id: kt00018";

        headers = curl_slist_append(headers, hdrContentType.c_str());
        headers = curl_slist_append(headers, hdrAuth.c_str());
        headers = curl_slist_append(headers, hdrCont.c_str());
        headers = curl_slist_append(headers, hdrNext.c_str());
        headers = curl_slist_append(headers, hdrApiId.c_str());

        // Request body
        json requestBody =
        {
            { "qry_tp", "2" },              // 2 : 개별
            { "dmst_stex_tp", "KRX" }       // KRX : 한국거래소
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
            std::string errorMessage = "잔고 조회 비정상 응답 코드 수신 : " + std::to_string(responseCode);

            log.Output(LogLevel::ERROR, errorMessage.c_str());

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            return; // holdings 갱신 없이 기존 데이터 유지
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
                    oss << "잔고 조회 실패: [" << returnCode << "] " << returnMsg;

                    log.Output(LogLevel::ERROR, oss.str().c_str());

                    // 토큰 만료 오류(return_code == 3)인 경우 토큰 초기화하여 다음 호출에서 재발급
                    if (returnCode == 3)
                        Login::ClearAccessToken();

                    curl_slist_free_all(headers);
                    curl_easy_cleanup(curl);

                    return; // holdings 갱신 없이 기존 데이터 유지
                }

                AppendHoldingsFromResponse(data, localHoldings);
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

    RefreshDailyProfitRates(localHoldings);

	// 락을 최소화하기 위해 로컬 맵에 데이터를 모두 채운 후 한 번에 스왑
    {
        std::lock_guard<std::mutex> lock(holdingsMutex);
        holdings.swap(localHoldings);
    }

    std::ostringstream summary;
    summary << "보유 종목 조회 완료. 총 " << holdings.size() << "개 종목";

    log.Output(LogLevel::INFO, summary.str().c_str());
}

std::map<std::string, Holding> Account::GetHoldingsSnapshot()
{
    std::lock_guard<std::mutex> lock(holdingsMutex);

    return holdings;
}

void Account::ShowHoldings()
{
    Log& log = Log::GetInstance();

    auto currentHoldings = GetHoldingsSnapshot();

    if (currentHoldings.empty())
    {
		log.Output(LogLevel::INFO, "보유 종목이 없습니다.", LogTarget::CONSOLE);

        return;
    }

    std::ostringstream oss;
    oss << "\n========== 현재 보유 종목 ==========";
	log.Output(LogLevel::INFO, oss.str().c_str(), LogTarget::CONSOLE);

    double totalValue = 0.0;
    double totalProfitLoss = 0.0;

    for (const std::pair<std::string, Holding>& currentPair : currentHoldings)
    {
        const Holding& holding = currentPair.second;

        std::ostringstream line;

        line << holding.code << " (" << holding.name << ")\n"
             << "  보유수량 : " << holding.quantity << "주\n"
             << "  매입가 : " << holding.purchasePrice << "원\n"
             << "  현재가 : " << holding.price << "원\n"
             << "  평가금액 : " << holding.value << "원\n"
             << "  평가손익 : " << holding.profitLoss << "원 (" << holding.profitRate << "%)";

		log.Output(LogLevel::INFO, line.str().c_str(), LogTarget::CONSOLE);

        totalValue += static_cast<double>(holding.value);
        totalProfitLoss += static_cast<double>(holding.profitLoss);
    }

    std::ostringstream totalLine;
    totalLine << "===================================\n"
              << "총 평가금액 : " << static_cast<long long>(totalValue) << "원\n"
              << "총 평가손익 : " << static_cast<long long>(totalProfitLoss) << "원";

	log.Output(LogLevel::INFO, totalLine.str().c_str(), LogTarget::CONSOLE);
}

void Account::AppendHoldingsFromResponse(const json& data, std::map<std::string, Holding>& localHoldings)
{
    if (!data.contains("acnt_evlt_remn_indv_tot") || !data["acnt_evlt_remn_indv_tot"].is_array())
        return;

    Log& log = Log::GetInstance();

    for (const json& item : data["acnt_evlt_remn_indv_tot"])
    {
        Holding holding;
        holding.account = currentAccountNumber;
        holding.code = item.value("stk_cd", "");
        holding.name = item.value("stk_nm", "");

        std::string inputQuantity = String::GetSignDigit(item.value("rmnd_qty", "0"));
        std::string inputPrice = String::GetSignDigit(item.value("cur_prc", "0"));
        std::string inputValue = String::GetSignDigit(item.value("evlt_amt", "0"));
        std::string inputPurchase = String::GetSignDigit(item.value("pur_pric", "0"));
        std::string inputProfitLoss = String::GetSignDigit(item.value("evltv_prft", "0"));
        std::string inputRate = item.value("prft_rt", "0.0");

        holding.quantity = !inputQuantity.empty() ? std::stol(inputQuantity) : 0;
        holding.price = !inputPrice.empty() ? std::stoll(inputPrice) : 0;
        holding.value = !inputValue.empty() ? std::stoll(inputValue) : 0;
        holding.purchasePrice = !inputPurchase.empty() ? std::stoll(inputPurchase) : 0;
        holding.profitLoss = !inputProfitLoss.empty() ? std::stoll(inputProfitLoss) : 0;

        std::string rate = String::GetSignDigit(inputRate);

        if (!rate.empty())
        {
            size_t dotPos = inputRate.find('.');
            holding.profitRate = (dotPos != std::string::npos) ? std::stod(inputRate) : std::stod(rate);
        }

        std::string key = holding.account + ":" + holding.code;
        localHoldings[key] = holding;

        std::ostringstream oss;
        oss << "종목 추가 : " << holding.code << " (" << holding.name << ") "
            << "수량 = " << holding.quantity << " "
            << "현재가 = " << holding.price << " "
            << "평가금액 = " << holding.value << " "
            << "손익 = " << holding.profitLoss << " "
            << "수익률 = " << holding.profitRate << "%";

        log.Output(LogLevel::INFO, oss.str().c_str());
    }
}

void Account::RefreshDailyProfitRates(std::map<std::string, Holding>& localHoldings)
{
    if (localHoldings.empty())
        return;

    std::set<std::string> stockCodes;

    for (const auto& pair : localHoldings)
    {
        const Holding& holding = pair.second;

        if (!holding.code.empty())
            stockCodes.insert(holding.code);
    }

    std::map<std::string, long long> lastEndPriceFromDatabase = GetLastEndPriceFromDatabase(stockCodes);
    const Week weekday = DateTime::GetCurrentWeekday();

    std::lock_guard<std::mutex> lock(cachedCloseMutex);

    for (std::pair<const std::string, Holding>& pair : localHoldings)
    {
        Holding& holding = pair.second;

        auto lastEndPriceIndex = lastEndPriceFromDatabase.find(holding.code);

		// 데이터베이스에서 직전 종가를 가져왔고, 0보다 큰 경우 해당 값을 사용
        if (lastEndPriceIndex != lastEndPriceFromDatabase.end() && lastEndPriceIndex->second > 0)
            holding.lastEndPrice = lastEndPriceIndex->second;

        else
        {
			// 만약 데이터베이스에서 직전 종가를 가져오지 못했거나 0 이하인 경우, 요일에 따라 다른 전략으로 직전 종가 결정
            if (weekday == Week::Saturday || weekday == Week::Sunday)
            {
                std::map<std::string, long long>::iterator lastCostIndex = lastEndPriceByCode.find(holding.code);

                if (lastCostIndex != lastEndPriceByCode.end())
                    holding.lastEndPrice = lastCostIndex->second;
            }

			// 그 외의 경우에는 최근에 알려진 가격을 사용
            else
            {
                auto priceIter = lastKnownPriceByCode.find(holding.code);

                if (priceIter != lastKnownPriceByCode.end())
                    holding.lastEndPrice = priceIter->second;
            }

            if (holding.lastEndPrice < 0)
                holding.lastEndPrice = 0;
        }

        if (holding.lastEndPrice <= 0)
        {
            holding.lastEndPrice = holding.price;
            holding.dailyProfitRate = 0.0;
        }

        else
            holding.dailyProfitRate = CalculateDailyProfitRate(holding.price, holding.lastEndPrice);

        lastKnownPriceByCode[holding.code] = holding.price;
        lastEndPriceByCode[holding.code] = holding.lastEndPrice;
    }
}

double Account::CalculateDailyProfitRate(long long currentPrice, long long lastEndPrice)
{
    if (lastEndPrice <= 0)
		return 0.0;

	double priceDifference = static_cast<double>(currentPrice - lastEndPrice);

    return priceDifference / static_cast<double>(lastEndPrice) * 100.0;
}

std::map<std::string, long long> Account::GetLastEndPriceFromDatabase(const std::set<std::string>& codes)
{
    std::map<std::string, long long> result;

    if (codes.empty())
        return result;

    CURL* curl = curl_easy_init();

    if (curl == nullptr)
        return result;

    std::ostringstream csv;
    bool first = true;

    for (const auto& code : codes)
    {
        if (code.empty())
            continue;

        if (!first)
            csv << ",";

        csv << code;
        first = false;
    }

    if (first)
    {
        curl_easy_cleanup(curl);
        return result;
    }

    char* encodedCodes = curl_easy_escape(curl, csv.str().c_str(), 0);

    if (encodedCodes == nullptr)
    {
        curl_easy_cleanup(curl);
        return result;
    }

	// Spring Boot + MySQL에서 직전 거래일 종가를 조회하는 API 엔드포인트로 요청
    const std::string url = std::string(webServerUrl) + "/stream/holdings/prev-close?codes=" + encodedCodes;
    curl_free(encodedCodes);

    std::string readBuffer;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Network::WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L);

    CURLcode response = curl_easy_perform(curl);

	// 요청 실패 시 빈 맵 반환
    if (response != CURLE_OK)
    {
        curl_easy_cleanup(curl);

        return result;
    }

    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    curl_easy_cleanup(curl);

	// 응답이 200이 아니거나 응답 본문이 비어있으면 빈 맵 반환
    if (responseCode != 200 || readBuffer.empty())
        return result;

    try
    {
        json data = json::parse(readBuffer);

        if (!data.is_object())
            return result;

        for (auto iter = data.begin(); iter != data.end(); ++iter)
        {
			// 키는 종목 코드, 값은 직전 거래일 종가라고 가정하고 파싱
            const std::string& code = iter.key();
            const json& value = iter.value();

            if (code.empty() || value.is_null())
                continue;

            long long prevClose = 0;

            if (value.is_number_integer())
                prevClose = value.get<long long>();

            else if (value.is_number_float())
                prevClose = static_cast<long long>(value.get<double>());

            else if (value.is_string())
            {
                std::string digits = String::GetSignDigit(value.get<std::string>());

                if (!digits.empty())
                    prevClose = std::stoll(digits);
            }

            if (prevClose > 0)
                result[code] = prevClose;
        }
    }

    catch (...)
    {
        return std::map<std::string, long long>();
    }

    return result;
}