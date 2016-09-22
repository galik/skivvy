/*
 * utils.cpp
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

#include <random>

#include <skivvy/utils.h>

#include <hol/simple_logger.h>
#include <hol/small_types.h>
#include <hol/random_utils.h>
#include <skivvy/irc.h>

namespace skivvy { namespace utils {

using namespace hol::random_utils;
using namespace hol::small_types::ios;
using namespace hol::small_types::ios::functions;
using namespace hol::small_types::basic;
using namespace skivvy::irc;
using namespace hol::simple_logger;
//using namespace sookee::ios;

bool parse_rangelist(const str& rangelist, siz_vec& items)
{
	str range;
	siss iss(rangelist);
	while(sgl(iss, range, ','))
	{
		// "1-4", "7", "9-11 ", " 15 - 16" ...
		siss iss(range);

		str lb, ub;
		sgl(sgl(iss, lb, '-'), ub);
		if(lb.empty())
		{
			LOG::E << "Bad range: " << range;
			continue;
		}

		siz l, u;

		if(!(siss(lb) >> l))
		{
			LOG::E << "Bad range (unrecognized number): " << range;
			continue;
		}

		if(ub.empty() || !(siss(ub) >> u))
			u = l;

		if(u < l)
		{
			LOG::E << "Bad range (higher to lower): " << range;
			continue;
		}

		for(siz i = l; i <= u; ++i)
			items.push_back(i);
	}
	return true;
}

static const str_map bg =
{
	{IRC_White, IRC_Black}
	, {IRC_Black, IRC_Light_Gray}
	, {IRC_Navy_Blue, IRC_Light_Gray}
	, {IRC_Green, IRC_Light_Gray}
	, {IRC_Red, IRC_Light_Gray}
	, {IRC_Brown, IRC_Light_Gray}
	, {IRC_Purple, IRC_Light_Gray}
	, {IRC_Olive, IRC_Light_Gray}
	, {IRC_Yellow, IRC_Black}
	, {IRC_Lime_Green, IRC_Black}
	, {IRC_Teal, IRC_Black}
	, {IRC_Aqua_Light, IRC_Black}
	, {IRC_Royal_Blue, IRC_Black}
	, {IRC_Hot_Pink, IRC_Black}
	, {IRC_Dark_Gray, IRC_Light_Gray}
	, {IRC_Light_Gray, IRC_Black}
};

str prompt_color(const str& seed)
{
	str name = seed;
	if(!seed.find("do_") && seed.size() > 3)
		name = seed.substr(3);
	siz idx = 0;
	for(const char c: name)
		idx += siz(c);

	idx = (idx % 16);
//	str col = (idx<10?"0":"") + std::to_string(idx);
	str col = std::to_string(idx);
	if(col.size() < 2)
		col = "0" + col;
	soss oss;

	str back = IRC_Black;
	if(bg.count(col))
		back = bg.at(col);

	oss << IRC_BOLD << IRC_COLOR << col << "," << back
		<< name << ":" << IRC_NORMAL << " ";
	return oss.str();
}


//int rand_int(int low, int high)
//{
//	thread_local static std::mt19937 g(std::random_device{}());
//	std::uniform_int_distribution<> d(low, high);
//	return d(g);
//}

str wild_replace(const str wild, const str& replacement)
{
	return wild_replace(wild, str_vec{replacement});
}

str wild_replace(const str wild, const str_vec& replacements)
{
	if(replacements.empty())
		return wild;
	str fixed;
	bool esc = false;
	for(const char c: wild)
	{
		if(esc && !(esc = false))
			fixed += c;
		else if(c == '\\')
			esc = true;
		else if(c == '*')
			fixed += rnd::random_element(replacements);
		else
			fixed += c;
	}
	return fixed;
}

// Missing from removal of libsookee

str::size_type extract_delimited_text(const str& in, const str& d1, const str& d2, str& out, size_t pos)
{
//	if(pos == str::npos)
//		return pos;

	auto end = pos;

	if((pos = in.find(d1, pos)) != str::npos)
		if((end = in.find(d2, (pos = pos + d1.size()))) != str::npos)
		{
			out = in.substr(pos, end - pos);
			return end + d2.size();
		}
	return str::npos;
}

}} // skivvy::cal
