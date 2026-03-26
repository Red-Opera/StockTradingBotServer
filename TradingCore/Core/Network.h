#pragma once

#include <string>

class Network
{
public:
	static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);		// 쓰기 콜백 메소드
	static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);			// 헤더 콜백 메소드
};

