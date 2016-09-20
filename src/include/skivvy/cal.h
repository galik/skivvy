/*
 * cal.h
 *
 *  Created on: 29 Apr 2010
 *      Author: oaskivvy@gmail.com
 */

#ifndef SKIVVY_CAL_H
#define SKIVVY_CAL_H

#include <ctime>
#include <cassert>
#include <string>

namespace skivvy { namespace cal {

using year_t = size_t;
using month_t = size_t;
using week_t = size_t;
using day_t = size_t;
using hour_t = size_t;
using min_t = size_t;
using sec_t = size_t;

inline
year_t get_year(time_t u = time(0))
{
	return gmtime(&u)->tm_year + 1900;
}

inline
month_t get_month(time_t u = time(0))
{
	return gmtime(&u)->tm_mon;
}

inline
day_t get_date(time_t u = time(0))
{
	return gmtime(&u)->tm_mday;
}

static size_t dmap[] =
{
	31		// Jan
	, 28	// Feb
	, 31	// Mar
	, 30	// Apr
	, 31	// May
	, 30	// Jun
	, 31	// Jul
	, 31	// Aug
	, 30	// Sep
	, 31	// Oct
	, 30	// Nov
	, 31	// Dec
};

static std::string mon_str_map[] =
{
	"January"
	, "February"
	, "March"
	, "April"
	, "May"
	, "June"
	, "July"
	, "August"
	, "September"
	, "October"
	, "November"
	, "December"
};

inline std::string get_month_str(month_t m, bool full = false)
{
	assert(m < 12);
	return full ? mon_str_map[m] : mon_str_map[m].substr(0, 3);
}

struct date_diff_t
{
	year_t year;
	month_t month;
	week_t week;
	day_t day;
	hour_t hour;
	min_t min;
	sec_t sec;

	date_diff_t()
	: year(0), month(0), week(0), day(0), hour(0), min(0), sec(0) {}
};

inline
bool is_leap_year(year_t y)
{
	return ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0);
}

inline
size_t days_in_month(year_t y, month_t m)
{
	return dmap[m] + (m == 1 ? (cal::is_leap_year(y) ? 1 : 0) : 0);
}

inline
size_t days_in_month(month_t m)
{
	return dmap[m] + (m == 1 ? (cal::is_leap_year(cal::get_year()) ? 1 : 0) : 0);
}


class date_t
: public tm
{
public:

	/**
	 * 2010-01-10
	 */
	static const char* FORMAT_ISO_8601;
	/**
	 * 20100110
	 */
	static const char* FORMAT_ISO_8601_COMPACT;
	/**
	 * 2010-01
	 */
	static const char* FORMAT_ISO_8601_MONTH;
	/**
	 * 2010-W23
	 */
	static const char* FORMAT_ISO_8601_WEEK;
	/**
	 * 2010-013 (YYYY-DDD)
	 */
	static const char* FORMAT_ISO_8601_ORDINAL;
	/**
	 * 20100110 (YYYYMMDD)
	 */
	static const char* FORMAT_STAMP_DATE;
	/**
	 * 223407 (HHMMSS)
	 */
	static const char* FORMAT_STAMP_TIME;
	/**
	 * 20100110-223407
	 */
	static const char* FORMAT_STAMP;

	enum
	{
		JAN = 0, FEB, MAR,APR ,MAY ,JUN ,JUL ,AUG ,SEP ,OCT ,NOV, DEC
	};

	enum
	{
		SUN = 0, MON, TUE, WED, THU, FRI, SAT
	};

	explicit date_t();
	explicit date_t(const date_t* date);
	explicit date_t(const date_t& date);
	explicit date_t(const time_t* time);
	explicit date_t(time_t time);
	explicit date_t(year_t y, month_t m, day_t d);
	virtual ~date_t() {}

	time_t as_unix() const { tm t = (*this); return mktime(&t); }

	bool is_leap_year()
	{
		return cal::is_leap_year(tm_year);
	}

	size_t days_in_month()
	{
		return cal::days_in_month(tm_year, tm_mon);
	}

	void dec_year()
	{
		--tm_year;
	}

	void dec_month()
	{
		if(tm_mon == JAN)
		{
			dec_year();
		}
		--tm_mon;
	}

	void dec_day()
	{
		if(tm_mday == 1)
		{
			dec_month();
			tm_mday = days_in_month() + 1;
		}
		if(tm_yday == 0)
		{
			tm_yday = 365 + is_leap_year();
		}
		--tm_mday;
		--tm_yday;
	}

	date_t& operator=(const tm& d)
	{
		tm_sec = d.tm_sec;
		tm_min = d.tm_min;
		tm_hour = d.tm_hour;
		tm_mday = d.tm_mday;
		tm_mon = d.tm_mon;
		tm_year = d.tm_year;
		tm_wday = d.tm_wday;
		tm_yday = d.tm_yday;
		tm_isdst = d.tm_isdst;
		return *this;
	}

	/**
	 * %a	Abbreviated weekday name -(Thu)
	 * %A	Full weekday name -(Thursday)
	 * %b	Abbreviated month name -(Aug)
	 * %B	Full month name -(August)
	 * %c	Date and time representation -(Thu Aug 23 14:55:02 2001)
	 * %d	Day of the month (01-31) -(23)
	 * %H	Hour in 24h format (00-23) -(14)
	 * %I	Hour in 12h format (01-12) -(02)
	 * %j	Day of the year (001-366) -(235)
	 * %m	Month as a decimal number (01-12) -(08)
	 * %M	Minute (00-59) -(55)
	 * %p	AM or PM designation -(PM)
	 * %S	Second (00-61) -(02)
	 * %U	Week number with the first Sunday as the first day of week one (00-53) -(33)
	 * %w	Weekday as a decimal number with Sunday as 0 (0-6) -(4)
	 * %W	Week number with the first Monday as the first day of week one (00-53) -(34)
	 * %x	Date representation -(08/23/01)
	 * %X	Time representation -(14:55:02)
	 * %y	Year, last two digits (00-99) -(01)
	 * %Y	Year -(2001)
	 * %Z	Timezone name or abbreviation -(CDT)
	 * %%	A % sign -(%)
	 */
	std::string format(const char* fmt = 0);
};

//class Calendar
//{
//public:
//	Calendar();
//	virtual ~Calendar();
//
//	static void test(tst::context& ctx) throw(std::exception);
//};

}} // skivvy::cal

#endif // SKIVVY_CAL_H
