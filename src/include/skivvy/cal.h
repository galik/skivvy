#pragma once
#ifndef _SOOKEE_CAL_H_
#define _SOOKEE_CAL_H_

/*
 * cal.h
 *
 *  Created on: 6 Dec 2011
 *      Author: oaskivvy@gmail.com
 */
/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oaskivvy@gmail.com               |
'------------------------------------------------------------------'

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

http://www.gnu.org/licenses/gpl-2.0.html

'-----------------------------------------------------------------*/

#include <array>
#include <string>
#include <cassert>

namespace skivvy { namespace cal {

typedef size_t year_t;
typedef size_t month_t;
typedef size_t week_t;
typedef size_t day_t;
typedef size_t hour_t;
typedef size_t min_t;
typedef size_t sec_t;

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

}} // sookee::cal

#endif /* _SOOKEE_CAL_H_ */
