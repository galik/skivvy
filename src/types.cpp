/*
 * types.cpp
 *
 *  Created on: 21 May 2012
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2012 SooKee oaskivvy@gmail.com               |
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

#include <skivvy/types.h>
#include <skivvy/logrep.h>

#include <sstream>

namespace skivvy { namespace types {

using namespace skivvy::utils;

std::istream& delay::parse(std::istream& is)
{
	char f = 's'; // factor s = sec, m = min, h = hour, d = day, w = week
	is >> d;
	if(is >> f)
	{
//		if((is.peek() == EOF || std::isspace(is.peek())))
//		{
			switch(f)
			{
				case 's': d *= 1; break;
				case 'm': d *= 60; break;
				case 'h': d *= 60 * 60; break;
				case 'd': d *= 60 * 60 * 24; break;
				case 'w': d *= 60 * 60 * 24 * 7; break;
				default:
				log("UNKNOWN DELAY FACTOR: " << f);
				is.clear(is.rdstate() | std::ios::failbit);
			}
//		}
//		else
//		{
//			str rest;
//			is >> rest;
//			log("BAD DELAY FACTOR: " << f << rest);
//			is.clear(is.rdstate() | std::ios::failbit);
//		}
	}
	else
	{
		is.clear(is.rdstate() & ~std::ios::failbit);
	}
	return is;
}

bool delay::parse(const str& s)
{
	std::istringstream iss(s);
	return parse(iss);
}

}} // sookee::types
