/*
 * FloodController.cpp
 *
 *  Created on: 13 Jul 2012
 *      Author: oasookee@gmail.com
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

#include <skivvy/FloodController.h>
//#include <skivvy/ios.h>
#include <skivvy/stl.h>
//#include <skivvy/str.h>
//#include <skivvy/logrep.h>
//#include <skivvy/irc-constants.h>

#include <hol/small_types.h>
#include <hol/bug.h>
#include <hol/simple_logger.h>

#include <random>
#include <functional>

namespace skivvy { namespace ircbot {

using namespace skivvy;
//using namespace hol::small_types::ios;
//using namespace hol::small_types::ios::functions;
using namespace hol::small_types::basic;
using namespace hol::small_types::string_containers;
using namespace hol::simple_logger;

using lock_guard = std::lock_guard<std::mutex>;

FloodController::FloodController()
: idx(0)
{
}

FloodController::~FloodController() {}

void FloodController::dispatcher()
{
	while(dispatching)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(time_between_checks));
		{
			lock_guard lock(mtx);

			if(++idx >= keys.size())
				idx = 0;

			const siz end = idx;

			for(; idx < keys.size(); ++idx)
				if(!m[keys[idx]].empty())
					break;

			if(idx == keys.size())
			{
				for(idx = 0; idx < end; ++idx)
					if(!m[keys[idx]].empty())
						break;

				if(idx == end)
					continue;
			}

			m[keys[idx]].front()();
			m[keys[idx]].pop();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(time_between_events - time_between_checks));
	}

	LOG::I << "Dispatcher ended.";
}

bool FloodController::send(const str& channel, std::function<bool()> func)
{
	lock_guard lock(mtx);

	m[channel].push(func);

	if(stl::find(keys, channel) == keys.end())
		keys.push_back(channel);

	return true;
}

void FloodController::clear()
{
	lock_guard lock(mtx);
	m.clear();
	keys.clear();
}

void FloodController::clear(const str& channel)
{
	lock_guard lock(mtx);
	auto found = stl::find(keys, channel);
	if(found != keys.end())
	{
		m.erase(channel);
		keys.erase(found);
	}
}

void FloodController::start()
{
	LOG::I << "Starting up dispatcher:";

	if(!dispatching)
	{
		dispatching = true;
		fut = std::async(std::launch::async, [&]{ dispatcher(); });
	}
}

void FloodController::stop()
{
	LOG::I << "Closing down dispatcher:";
	if(!dispatching)
		return;
	dispatching = false;
	if(fut.valid())
		if(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			fut.get();
}

}} // sookee::ircbot
