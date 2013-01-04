#pragma once
#ifndef _SKIVVY_IRCBOT_FACTOID_H_
#define _SKIVVY_IRCBOT_FACTOID_H_
/*
 * plugin-factoid.h
 *
 *  Created on: 04 Jan 2013
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oaskivvy@gmail.com               |
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

#include <skivvy/ircbot.h>

#include <deque>
#include <mutex>

namespace skivvy { namespace ircbot {

class FactoidIrcBotPlugin
: public BasicIrcBotPlugin
{
public:

private:

	std::mutex mtx;

	bool faq(const message& msg);
	bool give(const message& msg);

public:
	FactoidIrcBotPlugin(IrcBot& bot);
	virtual ~FactoidIrcBotPlugin();

	// INTERFACE: BasicIrcBotPlugin

	virtual bool initialize();

	// INTERFACE: IrcBotPlugin

	virtual std::string get_name() const;
	virtual std::string get_version() const;
	virtual void exit();
};

}} // sookee::ircbot

#endif // _SKIVVY_IRCBOT_FACTOID_H_
