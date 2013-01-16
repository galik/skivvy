/*
 * logrep.cpp
 *
 *  Created on: 3 Aug 2011
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

#include <skivvy/logrep.h>
#include <skivvy/str.h>
#include <skivvy/ansi.h>

#include <cctype>
#include <sstream>
#include <algorithm>
#include <thread>
#include <numeric>

namespace skivvy { namespace utils {

using namespace skivvy::ansi;
using namespace skivvy::types;
using namespace skivvy::string;

size_t __scope__bomb__::indent = 0;

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
//	return "{" + std::to_string(obj_map[id]) + "}";
	std::ostringstream oss;
	oss << std::hex << id;
	return "{" + std::to_string(obj_map[id]) + ": " + oss.str() + "}";
}

const str COL = "\\[\\033[1;34m\\]";
const str OFF = "\\[\\033[0m\\]";

std::ostream& botbug(std::ostream* os)
{
	static std::ostream* osp = 0;
	static ansistream as(std::cout);

	if(!osp) // initialize
		if(!os)
			osp = &std::cout;
//			osp = &as;
	if(os) // change
		osp = os;
	return *osp;
}

str notbug()
{
	return ansi_esc({NORM});
}

str embellish(siz indent)
{
	return std::string(indent, '-');
}

const int_vec col =
{
	FG_RED
	, FG_GREEN
	, FG_YELLOW
	, FG_BLUE
	, FG_MAGENTA
	, FG_CYAN
};

str get_col(const str& name, int seed = 0)
{
	static str_map m;

	if(m.find(name) == m.end())
		m[name] = ansi_esc({col[std::accumulate(name.begin(), name.end(), seed) % col.size()]});// ;

	return m[name];
}

__scope__bomb__::__scope__bomb__(const char* name): name(name)
{
	++indent;
	bug(get_col(name) << str(indent, '-') + "> " << name << ' ' << get_col(THREAD, 1) << THREAD << get_col(OBJECT, 2) << OBJECT << ansi_esc({NORM}));
}
__scope__bomb__::~__scope__bomb__()
{
	bug(get_col(name) << "<" << str(indent, '-') << name << ' ' << get_col(THREAD, 1) << THREAD << get_col(OBJECT, 2) << OBJECT << ansi_esc({NORM}));
	--indent;
}

int rand_int(int low, int high)
{
//	static /*std::default_random_engine*/ std::mt19937 re {};
//	static std::uniform_int_distribution<int> uid {};
//	return uid(re, std::uniform_int_distribution<int>::param_type {low, high});
	static std::minstd_rand g(std::time(0));
	std::uniform_int_distribution<> d(low, high);
	return d(g);
}

}} // skivvy::utils
