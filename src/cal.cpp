/*
 * cal.cpp
 *
 *  Created on: 29 Apr 2010
 *      Author: oaskivvy@gmail.com
 */

#include <skivvy/cal.h>

namespace skivvy { namespace cal {

date_t::date_t()
{
	time_t t = 0;
	*this = *gmtime(&t);
}

date_t::date_t(const date_t* date)
{
	*this = *date;
}

date_t::date_t(const date_t& date)
{
	*this = date;
}

date_t::date_t(const time_t* t)
{
	time_t _t = t ? *t : time(0);
	*this = *gmtime(&_t);
}

date_t::date_t(time_t t)
{
	*this = *gmtime(&t);
}

date_t::date_t(year_t y, month_t m, day_t d)
{
	time_t t = 0;
	*this = *gmtime(&t);
	tm_year = y - 1900;
	tm_mon = m;
	tm_mday = d;
	tm_isdst = false;
}

const char* date_t::FORMAT_ISO_8601 = "%Y-%m-%d";
const char* date_t::FORMAT_ISO_8601_COMPACT = "%Y%m%d";
const char* date_t::FORMAT_ISO_8601_MONTH = "%Y%m";
const char* date_t::FORMAT_ISO_8601_WEEK = "%Y-W%U";
const char* date_t::FORMAT_ISO_8601_ORDINAL = "%Y-%j";
/**
 * 20100110 (YYYYMMDD)
 */
const char* date_t::FORMAT_STAMP_DATE = "%Y%m%d";
/**
 * 223407 (HHMMSS)
 */
const char* date_t::FORMAT_STAMP_TIME = "%H%M%S";
/**
 * 20100110-223407
 */
const char* date_t::FORMAT_STAMP = "%Y%m%d-%H%M%S";

std::string date_t::format(const char* fmt)
{
	static const char* default_format = "%Y%m%d";
	const char* f = fmt ? fmt : default_format;
	char str[1024];
	size_t len = strftime(str, 1024, f, this);
	return std::string(str, len);
}

//Calendar::Calendar()
//{
//}
//
//Calendar::~Calendar()
//{
//}
//
//void Calendar::test(tst::context& ctx) throw(std::exception)
//{
//	gman_test(ctx);
//	date_t d;
//
//	ctx.out << d.format() << std::endl;
//}

}} // skivvy::cal
