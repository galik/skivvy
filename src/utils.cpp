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

#include <skivvy/utils.h>

#include <skivvy/types.h>
#include <skivvy/logrep.h>

namespace skivvy { namespace utils {

using namespace skivvy::types;
//using namespace skivvy::irc;

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
			log("Bad range: " << range);
			continue;
		}

		siz l, u;

		if(!(siss(lb) >> l))
		{
			log("Bad range (unrecognized number): " << range);
			continue;
		}

		if(ub.empty() || !(siss(ub) >> u))
			u = l;

		if(u < l)
		{
			log("Bad range (higher to lower): " << range);
			continue;
		}

		for(siz i = l; i <= u; ++i)
			items.push_back(i);
	}
	return true;
}

}} // skivvy::cal
