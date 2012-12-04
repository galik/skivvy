#pragma once
#ifndef _SOOKEE_IRCBOT_COMPILE_H_
#define _SOOKEE_IRCBOT_COMPILE_H_
/*
 * ircbot-compile.h
 *
 *  Created on: 30 Jan 2012
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

#include <skivvy/ircbot.h>

#include <deque>
#include <mutex>

namespace skivvy { namespace ircbot {

class CompileIrcBotPlugin
: public BasicIrcBotPlugin
 , public IrcBotMonitor
{
public:

private:

	std::mutex mtx;
	void cpp(const message& msg, bool cpp11);

public:
	CompileIrcBotPlugin(IrcBot& bot);
	virtual ~CompileIrcBotPlugin();

	// INTERFACE: BasicIrcBotPlugin

	virtual bool initialize();

	// INTERFACE: IrcBotPlugin

	virtual std::string get_name() const;
	virtual std::string get_version() const;
	virtual void exit();

	// INTERFACE: IrcBotMonitor

	virtual void event(const message& msg);
};

}} // sookee::ircbot

#endif // _SOOKEE_IRCBOT_COMPILE_H_
