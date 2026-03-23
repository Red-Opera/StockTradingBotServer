#pragma once

#include "DateTime.h"

#include <nlohmann/json.hpp>

#include <vector>
#include <string_view>

using json = nlohmann::json;

struct JsonData
{
	std::string key;
	std::string value;
};

class Convert
{
public:
	// ============================================================================
	// JSON
	// ============================================================================

	static double JsonToDouble(const json& j, const std::string& key);
	static long JsonToLong(const json& j, const std::string& key);

	static JsonData GetJsonData(const std::string_view& data);				// 키:값 쌍을 JsonData 구조체로 반환

	// ============================================================================
	// DateTime
	// ============================================================================

	static Week IntToWeek(int weekInt);				// 0~6의 정수를 Week 열거형으로 변환
	static std::vector<int> WeekToInt(Week week);	// Week 열거형을 0~6의 정수로 변환
};