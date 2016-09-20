/*
 * irc.cpp
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

#include <hol/small_types.h>
#include <skivvy/irc.h>

#include <cmath>
#include <sstream>

namespace skivvy { namespace irc {

using namespace hol::small_types::basic;

const std::string IRC_REVERSE = "\u0016";
const std::string IRC_ITALICS = "\u0016";
const std::string IRC_BOLD = ""; // 0x02
const std::string IRC_COLOR = ""; // 0x03
const std::string IRC_NORMAL = ""; // 0x0F
const std::string IRC_UNDERLINE = ""; // 0x1F

const std::string IRC_White = "00";
const std::string IRC_Black = "01";
const std::string IRC_Navy_Blue = "02";
const std::string IRC_Green = "03";
const std::string IRC_Red = "04";
const std::string IRC_Brown = "05";
const std::string IRC_Purple = "06";
const std::string IRC_Olive = "07";
const std::string IRC_Yellow = "08";
const std::string IRC_Lime_Green = "09";
const std::string IRC_Teal = "10";
const std::string IRC_Aqua_Light = "11";
const std::string IRC_Royal_Blue = "12";
const std::string IRC_Hot_Pink = "13";
const std::string IRC_Dark_Gray = "14";
const std::string IRC_Light_Gray = "15";
//const std::string IRC_White = "16";

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

std::array<color, 16> irc_colors
{{
	color{ 0xFF, 0xFF, 0xFF }
	, color{ 0x00, 0x00, 0x00 }
	, color{ 0x00, 0x00, 0x80 }
	, color{ 0x00, 0x80, 0x00 }
	, color{ 0xFF, 0x00, 0x00 }
	, color{ 0x80, 0x00, 0x40 }
	, color{ 0x80, 0x00, 0x80 }
	, color{ 0xFF, 0x80, 0x40 }
	, color{ 0xFF, 0xFF, 0x00 }
	, color{ 0x80, 0xFF, 0x00 }
	, color{ 0x00, 0x80, 0x80 }
	, color{ 0x00, 0xFF, 0xFF }
	, color{ 0x00, 0x00, 0xFF }
	, color{ 0xFF, 0x00, 0xFF }
	, color{ 0x80, 0x80, 0x80 }
	, color{ 0xC0, 0xC0, 0xC0 }
}};

// h² = a² + b² + c²
//  h = sqr(a² + b² + c²)

size_t distance(const color& c1, const color& c2)
{
	double r = std::abs(c1.r - c2.r);
	double g = std::abs(c1.g - c2.g);
	double b = std::abs(c1.b - c2.b);
	double h = std::sqrt((r * r) + (g * g) + (b * b));
	return size_t(h);
}

size_t closest_color(const color& c)
{
	size_t index = 0;

	size_t min = std::sqrt((0xFF * 0xFF) + (0xFF * 0xFF) + (0xFF * 0xFF));
	for(size_t d, i = 0; i < irc_colors.size(); ++i)
		if((d = distance(irc_colors[i], c)) < min) { index = i; min = d; }

	return index;
}

#define ColorIndex(c)	( ( (c) - '0' ) & 7 )
#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && isalnum(*((p)+1)) ) // ^[0-9a-zA-Z]
#define Q_IsSpecialChar(c) ((c) && ((c) < 32))

const int oatoirctab[8] =
{
	1 // "black"
	, 5 // "red"
	, 3 // "lime"
	, 8 // "yellow"
	, 2 // "blue"
	, 12 // "cyan"
	, 6 // "magenta"
	, 1 // "white"
};


str oa_to_IRC(const char* msg)
{
	std::ostringstream oss;

	oss << IRC_BOLD;

	while(*msg)
	{
		if(Q_IsColorString(msg))
		{
			oss << IRC_NORMAL;
			siz code = (*(msg + 1)) % 8;
			oss << IRC_COLOR << (oatoirctab[code] < 10 ? "0" : "") << oatoirctab[code];
			msg += 2;
		}
		else if(Q_IsSpecialChar(*msg))
		{
			oss << "#";
			msg++;
		}
		else
		{

			oss << *msg;
			msg++;
		}
	}

	oss << IRC_NORMAL;

	return oss.str();
}

str oa_to_IRC(const str& msg)
{
	return oa_to_IRC(msg.c_str());
}

}} // sookee::irc

