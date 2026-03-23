#include "Convert.h"

#include <vector>

double Convert::JsonToDouble(const json& j, const std::string& key)
{
    try
    {
        if (!j.contains(key))
			return 0.0;

        if (j[key].is_string())
        {
			// JSON 값을 문자열로 가져옴
            std::string s = j[key].get<std::string>();

			// 빈 문자열인 경우 0.0 반환
            if (s.empty())
                return 0.0;

            // string 타입을 double로 변환
            return std::stod(s);
        }

        else if (j[key].is_number())
            return j[key].get<double>();
    }

    catch (...) {}

    return 0.0;
}

long Convert::JsonToLong(const json& j, const std::string& key)
{
    try
    {
        if (!j.contains(key))
            return 0;

        if (j[key].is_string())
        {
            std::string s = j[key].get<std::string>();

            if (s.empty()) 
                return 0;

			// string 타입을 long로 변환
            return std::stol(s);
        }

        else if (j[key].is_number_integer())
            return j[key].get<long>();
    }

    catch (...) {}

    return 0;
}

JsonData Convert::GetJsonData(const std::string_view& data)
{
    auto pos = data.find(':');

    if (pos == std::string::npos)
		return JsonData{ "", "" };

    JsonData result;

    result.key = std::string(data.substr(0, pos));
    result.value = std::string(data.substr(pos + 1));

	return result;
}

Week Convert::IntToWeek(int weekInt)
{
	return static_cast<Week>(1 << (weekInt % 7));
}

std::vector<int> Convert::WeekToInt(Week week)
{
    std::vector<int> result;
	int intWeek = static_cast<int>(week);

    for (int i = 0; i < 7; ++i)
    {
        if ((intWeek & (1 << i)) != 0)
            result.push_back(i);
    }

	return result;
}