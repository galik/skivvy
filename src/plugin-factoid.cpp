/*
 * ircbot-grabber.cpp
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

#include <skivvy/plugin-factoid.h>

#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>

#include <skivvy/logrep.h>
#include <skivvy/str.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(FactoidIrcBotPlugin);
PLUGIN_INFO("Factoid", "0.1");

using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

FactoidIrcBotPlugin::FactoidIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
{
}

FactoidIrcBotPlugin::~FactoidIrcBotPlugin() {}

bool FactoidIrcBotPlugin::faq(const message& msg)
{
	BUG_COMMAND(msg);
	return true;
}

bool FactoidIrcBotPlugin::give(const message& msg)
{
	BUG_COMMAND(msg);
	return true;
}

// INTERFACE: BasicIrcBotPlugin

bool FactoidIrcBotPlugin::initialize()
{
	add
	({
		"!faq"
		, "!faq <key> - Display key fact."
		, [&](const message& msg){ faq(msg); }
	});
	add
	({
		"!give"
		, "!give <nick> <key> - Display fact highlighting <nick>."
		, [&](const message& msg){ give(msg); }
	});
//	bot.add_monitor(*this);
	return true;
}

// INTERFACE: IrcBotPlugin

str FactoidIrcBotPlugin::get_name() const { return NAME; }
str FactoidIrcBotPlugin::get_version() const { return VERSION; }

void FactoidIrcBotPlugin::exit()
{
//	bug_func();
}

// INTERFACE: IrcBotMonitor

}} // skivvy::ircbot
