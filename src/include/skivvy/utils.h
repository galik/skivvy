/*
 * utils.h
 *
 *  Created on: 13 Feb 2013
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oaskivvy@gmail.com                     |
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

#ifndef _SKIVVY_UTILS_H_
#define _SKIVVY_UTILS_H_

#include <skivvy/types.h>

namespace skivvy { namespace utils {

using namespace skivvy::types;

// "1-4", "7", "9-11 ", " 15 - 16" ... -> siz_vec{1,2,3,4,7,9,10,11,15,16}
bool parse_rangelist(const str& rangelist, siz_vec& items);

template<typename Rep, typename Period>
void print_duration(std::chrono::duration<Rep, Period> t, std::ostream& os)
{
	typedef std::chrono::duration<int, std::ratio<60 * 60 * 24>> days;

	auto d = std::chrono::duration_cast < days > (t);
	auto h = std::chrono::duration_cast < std::chrono::hours > (t - d);
	auto m = std::chrono::duration_cast < std::chrono::minutes > (t - d - h);
	auto s = std::chrono::duration_cast < std::chrono::seconds > (t - d - h - m);
	if(t >= days(1))
		os << d.count() << "d ";
	if(t >= std::chrono::hours(1))
		os << h.count() << "h ";
	if(t >= std::chrono::minutes(1))
		os << m.count() << "m ";
	os << s.count() << "s";
}

str prompt_color(const str& seed);
#define REPLY_PROMPT skivvy::utils::prompt_color(__func__)

}} // skivvy::utils

#endif // _SKIVVY_UTILS_H_
