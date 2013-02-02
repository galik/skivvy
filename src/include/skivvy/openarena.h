#pragma once
#ifndef _SKIVVY_OPENARENA_H_
#define _SKIVVY_OPENARENA_H_

/*
 * openarena.h
 *
 *  Created on: 27 Jan 2013
 *      Author: oaskivvy@gmail.com
 */
/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oaskivvy@gmail.com               |
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
#include <skivvy/types.h>
#include <skivvy/irc.h>
#include <skivvy/str.h>

namespace skivvy { namespace oa {

using namespace skivvy::irc;
using namespace skivvy::types;
using namespace skivvy::string;

inline
str oa_handle_to_irc(str oa)
{
	for(char& c: oa)
		if(!std::isprint(c))
			c = '#';
	static str BACK = "," + IRC_Black;
	static str_map subs =
	{
		{"^0", IRC_COLOR + IRC_Black + "," + IRC_Dark_Gray}
		, {"^1", IRC_COLOR + IRC_Red + BACK}
		, {"^2", IRC_COLOR + IRC_Lime_Green + BACK}
		, {"^3", IRC_COLOR + IRC_Yellow + BACK}
		, {"^4", IRC_COLOR + IRC_Navy_Blue + BACK}
		, {"^5", IRC_COLOR + IRC_Royal_Blue + BACK}
		, {"^6", IRC_COLOR + IRC_Purple + BACK}
		, {"^7", IRC_COLOR + IRC_White + BACK}
		, {"^8", IRC_COLOR + IRC_Brown + BACK}
	};

	if(oa.empty())
		return oa;

	// ensure color prefix
	if(oa[0] != '^')
		oa = "^7" + oa; // default to white

	// ensure color suffix
//	oa = oa + "^7"; // default to white

//	size_t pos = 0;
	for(str_pair& p: subs)
		replace(oa, p.first, p.second);
	oa += IRC_NORMAL;
	return oa;

}

//inline
//str oa_handle_to_irc(str oa)
//{
//	return oa;
//	static str BACK = "," + IRC_Black;
//	static str_map subs =
//	{
//		{"^0", IRC_COLOR + IRC_Black + "," + IRC_Dark_Gray}
//		, {"^1", IRC_COLOR + IRC_Red + BACK}
//		, {"^2", IRC_COLOR + IRC_Lime_Green + BACK}
//		, {"^3", IRC_COLOR + IRC_Yellow + BACK}
//		, {"^4", IRC_COLOR + IRC_Navy_Blue + BACK}
//		, {"^5", IRC_COLOR + IRC_Royal_Blue + BACK}
//		, {"^6", IRC_COLOR + IRC_Purple + BACK}
//		, {"^7", IRC_COLOR + IRC_White + BACK}
//		, {"^8", IRC_COLOR + IRC_Brown + BACK}
//	};
//
//	static str oa2irc[] =
//	{
//		IRC_COLOR + IRC_Black
//		, IRC_COLOR + IRC_Red // COLOR_RED
//		, IRC_COLOR + IRC_Green // COLOR_GREEN
//		, IRC_COLOR + IRC_Yellow // COLOR_YELLOW
//		, IRC_COLOR + IRC_Navy_Blue // COLOR_BLUE
//		, IRC_COLOR + IRC_Royal_Blue // COLOR_CYAN
//		, IRC_COLOR + IRC_Purple // COLOR_MAGENTA
//		, IRC_COLOR + IRC_White   // COLOR_WHITE
//		, IRC_COLOR + IRC_Brown   //
//		, "xxx"
//	};
//
//	if(oa.empty())
//		return oa;
//
//	for(char& c: oa)
//		if(!std::isprint(c))
//			c = '#';
//
//	// ensure color prefix
//	if(oa[0] != '^')
//		oa = "^7" + oa; // default to white
//
//	str irc;
//	siz i = 1;
//	for(; i < oa.size(); ++i)
//	{
//		bug_var(oa[i - 1]);
//		bug_var(oa[i]);
//		if(isdigit(oa[i] && oa[i - 1] == '^'))
//		{
//			bug_var(oa[i] - '0');
//			irc.append(oa2irc[oa[i] - '0']);
//			++i;
//		}
//		else
//			irc.append(1, oa[i - 1]);
//	}
//	irc.append(1, oa[i - 1]);
//
//	irc += IRC_NORMAL;
//	return irc;
//
//}

#define ColorIndex(c)	( ( (c) - '0' ) & 7 )
#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && isalnum(*((p)+1)) ) // ^[0-9a-zA-Z]
#define Q_IsSpecialChar(c) ((c) && ((c) < 32))

inline
str oatoirc(const str& s, const str& back = "")
{
	std::ostringstream oss;

	static str BACK = back.empty() ? back : "," + back;

	static str q3ToAnsi[] =
	{
		IRC_COLOR + IRC_Black
		, IRC_Red // COLOR_RED
		, IRC_Green // COLOR_GREEN
		, IRC_Yellow // COLOR_YELLOW
		, IRC_Navy_Blue // COLOR_BLUE
		, IRC_Royal_Blue // COLOR_CYAN
		, IRC_Purple // COLOR_MAGENTA
		, IRC_White   // COLOR_WHITE
		, IRC_Brown   //
	};

	const char* msg = s.c_str();

	while( *msg )
	{
		if( Q_IsColorString( msg ) || *msg == '\n' )
		{
			if( *msg == '\n' )
			{
				oss << IRC_NORMAL;
				msg++;
			}
			else
			{
				oss << IRC_COLOR << q3ToAnsi[ ColorIndex(*(msg + 1))] << back;
				msg += 2;
			}
		}
		else if(Q_IsSpecialChar(*msg))
		{
				// Print the special char
			oss << "#";
			msg++;
		}
		else
		{
			oss << *msg;
			msg++;
		}
	}
	return oss.str();
}

}} // skivvy::oa
#endif /* _SKIVVY_OPENARENA_H_ */
