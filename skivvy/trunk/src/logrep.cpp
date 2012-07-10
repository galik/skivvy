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

#include <skivvy/logrep.h>
#include <skivvy/str.h>

#include <cctype>
#include <sstream>
#include <algorithm>
#include <thread>

namespace skivvy { namespace utils {

using namespace skivvy::types;
using namespace skivvy::string;

void wait_on(std::function<bool()> test, time_t timeout, time_t rate)
{
	time_t now = std::time(0);
	while(!test() && (!timeout || time(0) - now < timeout))
		std::this_thread::sleep_for(std::chrono::seconds(rate));
}

str thread_name()
{
	std::thread::id id = std::this_thread::get_id();
	static std::map<std::thread::id, siz> thread_map;
	static size_t thread_count(0);

	if(thread_map.find(id) == thread_map.end())
	{
		thread_map[id] = thread_count++;
	}
	return "[" + std::to_string(thread_map[id]) + "]";
}

str obj_name(void* id)
{
	static std::map<void*, siz> obj_map;
	static siz obj_count(0);

	if(obj_map.find(id) == obj_map.end())
	{
		obj_map[id] = obj_count++;
	}
	return "{" + std::to_string(obj_map[id]) + "}";
}

}} // sookee::utils
