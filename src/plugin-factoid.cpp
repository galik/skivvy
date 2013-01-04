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
#include <skivvy/irc.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(FactoidIrcBotPlugin);
PLUGIN_INFO("Factoid", "0.1");

using namespace skivvy;
using namespace skivvy::irc;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

const str STORE_FILE = "factoid.store.file";
const str STORE_FILE_DEFAULT = "factoid-store.txt";
const str FACT_USER = "factoid.fact.user";

FactoidIrcBotPlugin::FactoidIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
, store(bot.getf(STORE_FILE, STORE_FILE_DEFAULT))
{
}

FactoidIrcBotPlugin::~FactoidIrcBotPlugin() {}

bool FactoidIrcBotPlugin::is_user_valid(const message& msg, const str& svar)
{
	bug_func();
	for(const str& r: bot.get_vec(svar))
		if(bot.preg_match(msg.get_userhost(), r))
			return true;
	return false;
}

// IRC_BOLD + IRC_COLOR + IRC_Green + "addfact: " + IRC_NORMAL

str get_prefix(const message& msg, const str& color)
{
	str cmd = msg.get_user_cmd();
	if(!cmd.empty())
		cmd.erase(0, 1); // loose the '!'

	return IRC_BOLD + IRC_COLOR + color + cmd + ": " + IRC_NORMAL;
}

bool FactoidIrcBotPlugin::reloadfacts(const message& msg)
{
	BUG_COMMAND(msg);

	if(!is_user_valid(msg, FACT_USER))
		return bot.cmd_error(msg, msg.get_nick() + " is not authorised to reload facts.");

	store.load();
	bot.fc_reply(msg, get_prefix(msg, IRC_Red) + " Fact database reloaded.");

	return true;
}

bool FactoidIrcBotPlugin::addfact(const message& msg)
{
	BUG_COMMAND(msg);

	if(!is_user_valid(msg, FACT_USER))
		return bot.cmd_error(msg, msg.get_nick() + " is not authorised to add facts.");

	// !addfact <key> "<fact>"
	str key, fact;
	if(!bot.extract_params(msg, {&key, &fact}))
		return false;

	store.add(lowercase(key), fact);
	bot.fc_reply(msg, get_prefix(msg, IRC_Green) + " Fact added to database.");

	return true;
}

bool FactoidIrcBotPlugin::findfact(const message& msg)
{
	BUG_COMMAND(msg);

	// !findfact <regex>"
	str_vec keys = store.find(lowercase(msg.get_user_params()));

	if(keys.size() > 20)
	{
		bot.fc_reply(msg, get_prefix(msg, IRC_Dark_Gray) + "Too many results, printing the first 20.");
		keys.resize(10);
	}

	str line, sep;
	for(const str& key: keys) // TODO: filter out aliases here ?
		{ line += sep + key; sep = ", "; }
	bot.fc_reply(msg, get_prefix(msg, IRC_Dark_Gray) + line);

	return true;
}

bool FactoidIrcBotPlugin::fact(const message& msg, const str& key, const str& prefix)
{
	BUG_COMMAND(msg);

	// !fact <key>
	const str_vec facts = store.get_vec(lowercase(key));
	for(const str& fact: facts)
	{
		if(!fact.empty() && fact[0] == '=')
		{
			// follow a fact alias link: <fact2>: = <fact1>
			str key;
			std::getline(std::istringstream(fact).ignore() >> std::ws, key);
			this->fact(msg, key, prefix);
		}
		else if(!fact.empty() && fact[0] == '#')
		{
			// maps: # [map] // enumerate keys with subject marker [map]
			str topic;
			std::istringstream iss(fact);
			if(iss.ignore() >> std::ws
			&& std::getline(iss, topic, '[')
			&& std::getline(iss, topic, ']'))
				topic = "[" + topic + "]";

			if(topic.empty())
			{
				log("factoid: ERROR: bad topic in key enumeration: " << fact);
				return false;
			}

			str line, sep;
			str_vec lines;
			str_vec keys = store.find(".*");
			for(const str& key: keys)
			{
				if(!store.get(key).find(topic))
				{
					line += sep + key;
					sep = ", ";
					if(line.size() > 300)
					{
						lines.push_back(line);
						line.clear();
						sep.clear();
					}
				}
			}
			if(!line.empty())
				lines.push_back(line);

			for(const str& line: lines)
//				bot.fc_reply(msg, get_prefix(msg, IRC_Dark_Gray) + line);
				bot.fc_reply(msg, prefix + IRC_BOLD + IRC_COLOR + IRC_Dark_Gray + line);
		}
		else
//			bot.fc_reply(msg, get_prefix(msg, IRC_Navy_Blue) + fact);
			bot.fc_reply(msg, prefix + IRC_BOLD + IRC_COLOR + IRC_Navy_Blue + fact);
	}

	return true;
}

bool FactoidIrcBotPlugin::fact(const message& msg)
{
	BUG_COMMAND(msg);

	// !fact <key>
	str key = msg.get_user_params();
	if(trim(key).empty())
		return bot.cmd_error(msg, "Expected: !fact <key>.");
	fact(msg, key);

	return true;
}

bool FactoidIrcBotPlugin::give(const message& msg)
{
	BUG_COMMAND(msg);

	// !give <nick> <key>
	str nick, key;
	if(!(std::getline(std::istringstream(msg.get_user_params()) >> nick >> std::ws, key)))
		return bot.cmd_error(msg, "Expected: !give <nick> <key>.");

	fact(msg, key, nick + ": " + irc::IRC_BOLD + "(" + key + ") - " + irc::IRC_NORMAL);

	return true;
}

// INTERFACE: BasicIrcBotPlugin

bool FactoidIrcBotPlugin::initialize()
{
	add
	({
		"!addfact"
		, "!addfact <key> \"<fact>\" - Display key fact."
		, [&](const message& msg){ addfact(msg); }
	});
	add
	({
		"!findfact"
		, "!findfact <regex> - Get a list of matching fact keys."
		, [&](const message& msg){ findfact(msg); }
	});
	add
	({
		"!fact"
		, "!fact <key> - Display key fact."
		, [&](const message& msg){ fact(msg); }
	});
	add
	({
		"!give"
		, "!give <nick> <key> - Display fact highlighting <nick>."
		, [&](const message& msg){ give(msg); }
	});
	add
	({
		"!reloadfacts"
		, "!reloadfacts - Reload fact database."
		, [&](const message& msg){ reloadfacts(msg); }
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
