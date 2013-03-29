#pragma once
#ifndef _SOOKEE_IRCBOT_FLOODCONTROLLER_H_
#define _SOOKEE_IRCBOT_FLOODCONTROLLER_H_

/*
 * FloodController.h
 *
 *  Created on: 13 Jul 2012
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2012 SooKee oasookee@gmail.com               |
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

#include <mutex>
#include <future>
#include <queue>

#include <sookee/bug.h>

namespace skivvy { namespace ircbot {

using namespace skivvy::types;
using namespace sookee::bug;

/**
 */
class FloodController
{
private:
	typedef std::queue<std::function<bool()>> dispatch_que;
	typedef std::map<str, dispatch_que> dispatch_map;

	std::mutex mtx;
	dispatch_map m; // channel -> dispatch_que
	str_vec keys;
	siz idx;

	siz time_between_checks = 500; // miliseconds
	siz time_between_events = 1800; // miliseconds

	std::future<void> fut;
	bool dispatching = false;

	void dispatcher();

public:
	FloodController();
	~FloodController();

	bool send(const str& channel, std::function<bool()> func);
	void clear();
	void clear(const str& channel);
	void start();
	void stop();

	void set_time_between_checks(siz milliseconds)
	{
		time_between_checks = milliseconds;
		bug_var(time_between_checks);
	}
	void set_time_between_events(siz milliseconds)
	{
		time_between_events = milliseconds;
		bug_var(time_between_events);
	}
};

}} // namespace skivvy { namespace ircbot {

#endif // _SOOKEE_IRCBOT_FLOODCONTROLLER_H_
