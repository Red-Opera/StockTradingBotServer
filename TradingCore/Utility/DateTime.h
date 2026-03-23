#pragma once

enum class Week : unsigned char { Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday };

class Date
{
public:
	Date();
	Date(unsigned char year, unsigned char month, unsigned char day);
	Date(unsigned char year, unsigned char month, unsigned char day, Week weekday);

	unsigned char year, month, day;
	Week weekday;
};

class Time
{
public:
	Time();
	Time(unsigned char hour, unsigned char minute, unsigned char second);
	Time(unsigned char hour, unsigned char minute, unsigned char second, unsigned char millisecond);

	unsigned char hour, minute, second, millisecond;
};

class DateTime
{
public:
	DateTime() = default;
	DateTime(const Date& date, const Time& time);
	DateTime(const Date& date, unsigned char hour, unsigned char minute, unsigned char second);
	DateTime(unsigned char year, unsigned char month, unsigned char day, const Time& time);
	DateTime(unsigned char year, unsigned char month, unsigned char day,
			unsigned char hour, unsigned char minute, unsigned char second);

	static Week GetCurrentWeekday();		// 현재 시스템 날짜의 요일을 반환하는 정적 메소드

private:
	Date date;
	Time time;
};