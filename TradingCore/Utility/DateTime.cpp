#include "DateTime.h"
#include "Convert.h"

#include <ctime>

Date::Date() : year(0), month(0), day(0), weekday(Week::Sunday)
{

}

Date::Date(unsigned char year, unsigned char month, unsigned char day) : year(year), month(month), day(day), weekday(Week::Sunday)
{

}

Date::Date(unsigned char year, unsigned char month, unsigned char day, Week weekday)
	: year(year), month(month), day(day), weekday(weekday)
{

}

Time::Time() : hour(0), minute(0), second(0), millisecond(0)
{

}

Time::Time(unsigned char hour, unsigned char minute, unsigned char second) : hour(hour), minute(minute), second(second), millisecond(0)
{

}

Time::Time(unsigned char hour, unsigned char minute, unsigned char second, unsigned char millisecond)
	: hour(hour), minute(minute), second(second), millisecond(millisecond)
{

}

DateTime::DateTime(const Date& date, const Time& time) : date(date), time(time)
{

}

DateTime::DateTime(const Date& date, unsigned char hour, unsigned char minute, unsigned char second)
	: date(date), time(hour, minute, second)
{

}

DateTime::DateTime(unsigned char year, unsigned char month, unsigned char day, const Time& time)
	: date(year, month, day), time(time)
{

}

DateTime::DateTime(unsigned char year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second)
	: date(year, month, day), time(hour, minute, second)
{

}

Week DateTime::GetCurrentWeekday()
{
    std::time_t now = std::time(nullptr);
    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    return Convert::IntToWeek(localTime.tm_wday);
}
