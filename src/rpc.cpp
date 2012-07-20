/*
 * rpc.cpp
 *
 *  Created on: 10 Aug 2011
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

#include <skivvy/rpc.h>

namespace skivvy { namespace rpc {

template<>
std::string call::get_param<std::string>()
{
	std::string t;
	std::getline(i, t);
	return t;
}

template<>
std::string call::get_return_value<std::string>()
{
	std::string t;
	std::getline(o, t);
	return t;
}

template<>
void call::get_return_value<void>()
{
	std::string t;
	std::getline(o, t);
}

template<>
void call::get_return_param<std::string>(std::string& t)
{
	std::getline(o, t);
}

//template<>
//std::istream& operator>><std::string>(std::istream& is, std::vector<std::string>& v)
//{
//	size_t sz;
//	is >> sz;
//	std::string line;
//	std::getline(is, line);
//	for(size_t i = 0; i < sz; ++i)
//	{
//		std::string t;
//		if(std::getline(is, t)) v.push_back(t);
//	}
//	return is;
//}

//template<>
//std::istream& operator>><std::string, std::string>(std::istream& is, std::pair<std::string, std::string>& p)
//{
//	std::getline(is, p.first);
//	std::getline(is, p.second);
//	return is;
//}

template<>
std::istream& inputter<std::string>(std::istream& is, std::string& t)
{
//	bug_func();
	return std::getline(is, t);
}

}} // sookee::rpc
