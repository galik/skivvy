#pragma once
#ifndef _SKIVVY_TIMERS_H_
#define _SKIVVY_TIMERS_H_

/*
 * Timers.h
 *
 *  Created on: 05 Feb 2013
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

#include <skivvy/types.h>
#include <skivvy/message.h>

#include <future>

namespace skivvy { namespace ircbot {

using namespace skivvy::types;

// ==================================================
// Bot Utils
// ==================================================

/**
 * multi-user aware random callback timer
 */
class RandomTimer
{
private:
	typedef std::set<const void*> user_set;

	user_set users;
	std::mutex mtx;
	std::future<void> fut;
	siz mindelay; // = 1;
	siz maxdelay; // = 120;

	std::function<void(const void*)> cb;

	void timer();

public:

	RandomTimer(std::function<void(const void*)> cb);
	RandomTimer(const RandomTimer& rt);
	~RandomTimer();

	void set_mindelay(size_t seconds);
	void set_maxdelay(size_t seconds);

	/**
	 * @return false if already on for this user else true.
	 */
	bool on(const void* user);

	/**
	 * @return false if already off for this user else true.
	 */
	bool off(const void* user);

	/**
	 * @return false if already off for this user else true.
	 */
	bool turn_off();
};

/**
 * multi-user aware random callback timer
 */
class MessageTimer
{
private:
	struct comp
	{
	  bool operator()(const message& lhs, const message& rhs) const
	  {return lhs.reply_to_cp() < rhs.reply_to_cp();}
	};

	typedef std::set<message, comp> message_set;

	message_set messages;
	std::mutex mtx;
	std::future<void> fut;
	siz mindelay; // = 1;
	siz maxdelay; // = 120;

	std::function<void(const message&)> cb;

	void timer();

public:

	MessageTimer(std::function<void(const message&)> cb);
	MessageTimer(const MessageTimer& mt);
	~MessageTimer();

	void set_mindelay(size_t seconds);
	void set_maxdelay(size_t seconds);

	/**
	 * @return false if already on for this user else true.
	 */
	bool on(const message& m);

	/**
	 * @return false if already off for this user else true.
	 */
	bool off(const message& m);

	/**
	 * @return false if already off for this user else true.
	 */
	bool turn_off();
};

}} // skivvy::ircbot

#endif // _SKIVVY_TIMERS_H_
