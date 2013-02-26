/*
 * ircbot.cpp
 *
 *  Created on: 29 Jul 2011
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

#include <skivvy/Timers.h>

#include <sookee/bug.h>
#include <sookee/log.h>

#include <skivvy/types.h>
#include <skivvy/utils.h>

#include <random>
#include <functional>

namespace skivvy { namespace ircbot {

using namespace skivvy::types;
using namespace skivvy::utils;
using namespace sookee::bug;
using namespace sookee::log;

RandomTimer::RandomTimer(std::function<void(const void*)> cb)
: mindelay(1)
, maxdelay(60)
, cb(cb)
{
}

RandomTimer::RandomTimer(const RandomTimer& rt)
: mindelay(rt.mindelay)
, maxdelay(rt.maxdelay)
, cb(rt.cb)
{
}

RandomTimer::~RandomTimer()
{
	turn_off();
}

void RandomTimer::timer()
{
	while(!users.empty())
	{
		try
		{
			for(const void* user: users)
				cb(user);

			std::time_t end = std::time(0) + rand_int(mindelay, maxdelay);
			//bug("NEXT QUOTE: " << ctime(&end));
			while(!users.empty() && std::time(0) < end)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		catch(std::exception& e)
		{
			log("e.what(): " << e.what());
		}
	}
}

bool RandomTimer::turn_off()
{
	if(users.empty())
		return false;

	{
		lock_guard lock(mtx);
		if(users.empty())
			return false;
		users.clear();
	}

	if(fut.valid())
		if(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			fut.get();

	return true;
}

void RandomTimer::set_mindelay(size_t seconds) { mindelay = seconds; }
void RandomTimer::set_maxdelay(size_t seconds) { maxdelay = seconds; }

bool RandomTimer::on(const void* user)
{
	if(users.count(user))
		return false;

	lock_guard lock(mtx);
	if(users.count(user))
		return false;

	users.insert(user);
	if(users.size() == 1)
		fut = std::async(std::launch::async, [&]{ timer(); });

	return true;
}

bool RandomTimer::off(const void* user)
{
	if(!users.count(user))
		return false;

	{
		lock_guard lock(mtx);
		if(!users.count(user))
			return false;

		users.erase(user);
		if(!users.empty())
			return true;
	}

	if(fut.valid())
		if(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			fut.get();

	return true;
}

MessageTimer::MessageTimer(std::function<void(const message&)> cb)
: mindelay(1)
, maxdelay(60)
, cb(cb)
{
}

MessageTimer::MessageTimer(const MessageTimer& rt)
: mindelay(rt.mindelay)
, maxdelay(rt.maxdelay)
, cb(rt.cb)
{
}

MessageTimer::~MessageTimer()
{
	turn_off();
}

void MessageTimer::timer()
{
	while(!messages.empty())
	{
		for(const message& m: messages)
			cb(m);

		std::time_t end = std::time(0) + rand_int(mindelay, maxdelay);
		bug("NEXT CALL: " << ctime(&end));
		while(!messages.empty() && std::time(0) < end)
			std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

bool MessageTimer::turn_off()
{
	if(messages.empty())
		return false;

	{
		lock_guard lock(mtx);
		if(messages.empty())
			return false;
		messages.clear();
	}

	if(fut.valid())
		if(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			fut.get();

	return true;
}

void MessageTimer::set_mindelay(size_t seconds) { mindelay = seconds; }
void MessageTimer::set_maxdelay(size_t seconds) { maxdelay = seconds; }

bool MessageTimer::on(const message& m)
{
	if(messages.count(m))
		return false;

	lock_guard lock(mtx);
	if(messages.count(m))
		return false;

	messages.insert(m);
	if(messages.size() == 1)
		fut = std::async(std::launch::async, [&]{ timer(); });

	return true;
}

bool MessageTimer::off(const message& m)
{
	if(!messages.count(m))
		return false;

	bool end = false;
	{
		lock_guard lock(mtx);
		if(!messages.count(m))
			return false;

		messages.erase(m);
		if(messages.empty())
			end = true;
	}

	if(end)
		if(fut.valid())
			if(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
				fut.get();

	return true;
}

}} // skivvy::ircbot
