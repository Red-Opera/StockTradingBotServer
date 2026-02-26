#include "Login.h"

#include "Core/Config.h"
#include "Core/Log.h"

#include <iostream>
#include <string>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string Login::accessToken;

// 접근토큰 발급
std::string Login::GetAccessToken()
{
    Log& log = Log::GetInstance();

    std::string readBuffer;

	// 이미 토큰이 발급된 경우 재사용
    if (!accessToken.empty())
		return accessToken;

    // 1. 요청할 API URL
    std::string endpoint = "/oauth2/token";
    std::string url = Config::hostURL + endpoint;

    // 3. 요청 데이터 (JSON)
    json requestData =
    {
        { "grant_type", "client_credentials" },
        { "appkey", Config::appKey },
        { "secretkey", Config::appSecret }
    };

    std::string jsonStr = requestData.dump();

    CURL* curl = curl_easy_init();

    if (curl == nullptr)
    {
        log.Output(LogLevel::ERROR, "CURL 초기화 실패");

        return std::string();
    }

    // 2. header 설정
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=UTF-8");

    // curl 옵션 설정
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Login::WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    // 4. HTTP POST 요청 수행
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        std::string errorMessage = "HTTP 요청 실패 : " + std::string(curl_easy_strerror(res));
		log.Output(LogLevel::ERROR, errorMessage.c_str());
    }

    else
    {
        // 응답 상태 코드 가져오기
        long responseCode;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);

        // 응답 출력
        std::cout << "Code: " << responseCode << std::endl;

        // JSON 파싱 및 출력
        try
        {
            json responseJson = json::parse(readBuffer);
            std::cout << "Body: " << responseJson.dump(4) << std::endl;

            // 토큰 추출
            if (responseJson.contains("token"))
                accessToken = responseJson["token"].get<std::string>();
        }

        catch (json::parse_error& e)
        {
			std::string errorMessage = "JSON 파싱 오류 발생 : " + std::string(e.what());

			log.Output(LogLevel::ERROR, errorMessage.c_str());
        }
    }

    // 정리
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

	if (accessToken.empty())
		log.Output(LogLevel::ERROR, "토큰 발급 실패");

    else
		log.Output(LogLevel::INFO, "토큰 발급 성공");

    return accessToken;
}

void Login::ClearAccessToken()
{
    Log::GetInstance().Output(LogLevel::INFO, "토큰 초기화 - 재발급을 요청합니다.");

    accessToken.clear();
}

size_t Login::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp)
{
    userp->append((char*)contents, size * nmemb);

    return size * nmemb;
}