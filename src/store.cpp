/*
 * store.cpp
 *
 *  Created on: 09 Jan 2013
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

#include <skivvy/store.h>

namespace skivvy { namespace utils {

sifs CacheStore::ifs;
sofs CacheStore::ofs;

void escape(str& s)
{
//	bug_func();
	replace(s, "\\", "\\0"); // escape delimiters
	replace(s, "{", "\\1");
	replace(s, "}", "\\2");
	replace(s, ",", "\\3");
}

void unescape(str& s)
{
//	bug_func();
	replace(s, "\\3", ",");
	replace(s, "\\2", "}");
	replace(s, "\\1", "{");
	replace(s, "\\0", "\\");
}

std::istream& getobject(std::istream& is, str& o)
{
	char c;
	if(is.peek() != '{') // protocol error
		is.clear(std::ios::failbit | is.rdstate());
	else
	{
		is.ignore(); // skip '{'
		siz d = 1;
		o.clear();
		while(d && is.get(c))
		{
			if(c == '{')
				++d;
			else if(c == '}')
				--d;
			if(d)
				o += c;
		}
	}
	return is;
}

std::istream& extract(std::istream& is, str& s)
{
	if(sgl(is, s))
		unescape(s);
	return is;
}

std::istream& extract(std::istream&& is, str& s)
{
	return extract(is, s);
}


}} // skivvy::utils
