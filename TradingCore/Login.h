#pragma once

#include "Core/Log.h"

#include <string>

class Login
{
public:
	static std::string GetAccessToken();
	static void ClearAccessToken();				// 토큰 초기화 (만료 시 재발급용)
	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);

private:
	static std::string accessToken;     // 발급된 토큰 캐시
	Log& log = Log::GetInstance();
};