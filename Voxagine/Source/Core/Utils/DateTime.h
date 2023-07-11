#pragma once

#include "pch.h"

class DateTime
{
public:
	static DateTime Parse(const std::string& time)
	{
		const time_t timestamp = std::stoll(time);
		if(timestamp != 0)
		{
			return DateTime(timestamp);
		}

		return DateTime();

	}

	DateTime()
	{
		// TODO log the local date and time

		Initialize();
	}

	DateTime(time_t timestamp)
	{
		m_CurrentTime = timestamp;
	}

	std::string ToString() const
	{
		return std::to_string(m_CurrentTime);
	}

	std::string ToUTC() const
	{
		char buff[70];
		strftime(buff, sizeof(buff), "%A %c", m_UTCTime);
		return std::string(buff);
	}

	time_t GetTime() const
	{
		return m_CurrentTime;
	}

	DateTime& operator=(const DateTime& dateTime)
	{
		// 
		m_CurrentTime = dateTime.m_CurrentTime;

		// convert now to tm struct for UTC
		m_UTCTime = dateTime.m_UTCTime;

		seconds	= dateTime.m_UTCTime->tm_sec;
		minutes	= dateTime.m_UTCTime->tm_min;
		hours	= dateTime.m_UTCTime->tm_hour;
		day		= dateTime.m_UTCTime->tm_mday;
		month	= dateTime.m_UTCTime->tm_mon;
		year	= dateTime.m_UTCTime->tm_year;
		weekday	= dateTime.m_UTCTime->tm_wday;
		yearday	= dateTime.m_UTCTime->tm_yday;
		isdst	= dateTime.m_UTCTime->tm_isdst;

		return *this;
	}
	
	int seconds = 0;	// seconds of minutes from 0 to 61
	int minutes = 0;	// minutes of hour from 0 to 59
	int hours = 0;		// hours of day from 0 to 24
	int day = 0;		// day of month from 1 to 31
	int month = 0;		// month of year from 0 to 11
	int year = 0;		// year since 1900
	int weekday = 0;	// days since sunday
	int yearday = 0;	// days since January 1st
	int isdst = 0;		// hours of daylight savings time

private:
	void Initialize()
	{
		// convert now to tm struct for UTC
		_gmtime64_s(m_UTCTime, &m_CurrentTime);

		seconds = m_UTCTime->tm_sec;
		minutes = m_UTCTime->tm_min;
		hours = m_UTCTime->tm_hour;
		day = m_UTCTime->tm_mday;
		month = m_UTCTime->tm_mon;
		year = m_UTCTime->tm_year;
		weekday = m_UTCTime->tm_wday;
		yearday = m_UTCTime->tm_yday;
		isdst = m_UTCTime->tm_isdst;
	}

	// Current date/time based on current system
	time_t m_CurrentTime = std::time(nullptr);

	// Current UTC time
	tm* m_UTCTime = nullptr;
};
