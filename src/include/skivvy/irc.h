#pragma once
#ifndef _SKIVVY_IRC_H_
#define _SKIVVY_IRC_H_

/*
 * irc.h
 *
 *  Created on: 14 Aug 2011
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
#include <hol/small_types.h>

namespace skivvy { namespace irc {

using namespace header_only_library::small_types::basic;

extern const str IRC_BOLD;
extern const str IRC_COLOR;
extern const str IRC_NORMAL;
extern const str IRC_UNDERLINE;

extern const str IRC_Black;
extern const str IRC_Navy_Blue;
extern const str IRC_Green;
extern const str IRC_Red;
extern const str IRC_Brown;
extern const str IRC_Purple;
extern const str IRC_Olive;
extern const str IRC_Yellow;
extern const str IRC_Lime_Green;
extern const str IRC_Teal;
extern const str IRC_Aqua_Light;
extern const str IRC_Royal_Blue;
extern const str IRC_Hot_Pink;
extern const str IRC_Dark_Gray;
extern const str IRC_Light_Gray;
extern const str IRC_White;

/*
00 0xFFFFFF white
01 0x000000 black
02 0x000080 blue
03 0x008000 green
04 0xFF0000 lightred
05 0x800040 brown
06 0x800080 purple
07 0xFF8040 orange
08 0xFFFF00 yellow
09 0x80FF00 lightgreen
10 0x008080 cyan
11 0x00FFFF lightcyan
12 0x0000FF lightblue
13 0xFF00FF pink
14 0x808080 grey
15 0xC0C0C0 lightgrey
*/

struct color
{
	size_t r, g, b;
};

extern std::array<color, 16> irc_colors;

// h² = a² + b² + c²
//  h = sqr(a² + b² + c²)

size_t distance(const color& c1, const color& c2);

size_t closest_color(const color& c);

str oa_to_IRC(const char* msg);
str oa_to_IRC(const str& msg);

}} // skivvy::irc

#endif /* _SKIVVY_IRC_H_ */
