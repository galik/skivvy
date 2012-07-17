#pragma once
#ifndef _SKIVVY_PLUGIN_EXAMPLE_H_
#define _SKIVVY_PLUGIN_EXAMPLE_H_
/*
 * plugin-example.h
 *
 *  Created on: 30 Jan 2012
 *      Author: oasookee@googlemail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2012 SooKee oasookee@googlemail.com               |
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

namespace myspace { namespace example {

using namespace skivvy::ircbot;

/**
 *
 */
class ExampleIrcBotPlugin
: public BasicIrcBotPlugin
 , public IrcBotMonitor // only if you want to monitor general chatter
{
private:

	/**
	 * One of the plugin's functions
	 */
	void do_stuff(const message& msg);

public:
	ExampleIrcBotPlugin(IrcBot& bot);
	virtual ~ExampleIrcBotPlugin();

	// INTERFACE: BasicIrcBotPlugin

	virtual bool initialize();

	// INTERFACE: IrcBotPlugin

	virtual std::string get_name() const;
	virtual std::string get_version() const;
	virtual void exit();

	// INTERFACE: IrcBotMonitor

	virtual void event(const message& msg); // only if you want have IrcBotMonitor
};

}} // myspace::example

#endif // _SKIVVY_PLUGIN_EXAMPLE_H_
