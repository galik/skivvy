/*
 * logrep.cpp
 *
 *  Created on: 3 Aug 2011
 *      Author: oasookee@googlemail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oasookee@googlemail.com               |
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

#include <skivvy/ios.h>

#include <skivvy/types.h>

namespace skivvy { namespace ios {

using namespace skivvy::types;

std::istream& getstring(std::istream& is, str& s)
{
	char term = ' ';
	if((is >> std::ws).peek() == '"' || is.peek() == '\'')
		is.get(term);

	char c;
	str n;
	while(is.get(c) && c != term)
		n.append(1, c);

	if(n.empty())
		is.setstate(std::ios::failbit);
	else
	{
		s = n;
		is.clear();
	}

	return is;
}

}} // sookee::ios
