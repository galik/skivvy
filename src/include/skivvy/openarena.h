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

namespace skivvy { namespace oa {

using namespace skivvy::irc;
using namespace skivvy::types;

inline
str oa_handle_to_irc(str oa)
{
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

	// ensure color prefix
	if(oa.empty())
		return oa;

	if(oa[0] != '^')
		oa = "^7" + oa; // default to white

	size_t pos = 0;
	for(str_pair& p: subs)
		while((pos = oa.find(p.first)) != str::npos)
			oa.replace(pos, p.first.size(), p.second);
	oa += IRC_NORMAL;
	return oa;

}

}} // skivvy::oa
#endif /* _SKIVVY_OPENARENA_H_ */
