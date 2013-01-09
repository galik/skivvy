/*
 * plugin-chanops.cpp
 *
 *  Created on: 03 Aug 2011
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oaskivvy@gmail.com                     |
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

#include <skivvy/plugin-chanops.h>

#include <fstream>
#include <sstream>

#include <skivvy/logrep.h>
#include <skivvy/stl.h>
#include <skivvy/str.h>
#include <skivvy/ios.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(ChanopsIrcBotPlugin);
PLUGIN_INFO("Channel Operations", "0.1");

using namespace skivvy;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

const str STORE_FILE = "chanops.store.file";
const str STORE_FILE_DEFAULT = "chanops-store.txt";

static const str BAN_FILE = "chanops.ban.file";
static const str BAN_FILE_DEFAULT = "chanops-bans.txt";
static const str USER_FILE = "chanops.user.file";
static const str USER_FILE_DEFAULT = "chanops-users.txt";
const str GREET_JOINERS = "chanops.greet.active";
const bool GREET_JOINERS_DEFAULT = false;
const str UNGREET_FILE = "chanops.ungreet.file";
const str UNGREET_FILE_DEFAULT = "chanops-ungreets.txt";
const str GREETINGS_VEC = "chanops.greet";
const str GREET_MIN_DELAY = "chanops.greet.delay.min";
const siz GREET_MIN_DELAY_DEFAULT = 1;
const str GREET_MAX_DELAY = "chanops.greet.delay.max";
const siz GREET_MAX_DELAY_DEFAULT = 6;

static uint32_t checksum(const std::string& pass)
{
	if(pass.size() < sizeof(uint32_t))
		return 0;
	uint32_t sum = 0xAA55AA55; // salt
	for(size_t i = 0; i < pass.size() - sizeof(uint32_t); ++i)
		sum *= *reinterpret_cast<const uint32_t*>(&pass[i]);
	return sum;
}


std::ostream& operator<<(std::ostream& os, const ChanopsIrcBotPlugin::user_r& ur)
{
	bug_func();

	// <user>:<sum>:group1,group2

	os << ur.user <<  ':' << std::hex << ur.sum << ':';
	str sep;
	for(const str& g: ur.groups)
		{ os << sep << g; sep = ","; }

	return os;
}
std::istream& operator>>(std::istream& is, ChanopsIrcBotPlugin::user_r& ur)
{
	bug_func();

	// <user>:<sum>:group1,group2

	std::getline(is, ur.user, ':');
	(is >> std::hex >> ur.sum).ignore();

	str group;
	while(std::getline(is, group, ','))
		ur.groups.insert(group);
//	siz s;
//	str line, g;
//	std::getline(is, ur.user);
//	std::getline(is, line);
//	std::istringstream(line) >> std::hex >> ur.sum;
//	std::getline(is, line);
//	std::istringstream(line) >> s;
//	for(siz i = 0; i < s; ++i)
//		if(std::getline(is, g))
//			ur.groups.insert(g);
	return is;
}

ChanopsIrcBotPlugin::ChanopsIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
, store(bot.getf(STORE_FILE, STORE_FILE_DEFAULT))
{
}

ChanopsIrcBotPlugin::~ChanopsIrcBotPlugin() {}

bool ChanopsIrcBotPlugin::permit(const message& msg)
{
	BUG_COMMAND(msg);

	const str& group = perms[msg.get_user_cmd()];

	// not accessible via exec(msg);
	if(group.empty())
		return false;

	// anyone can call
	if(group == "*")
		return true;

	bool in = false;
	for(const user_t& u: users)
	{
		if(u.prefix != msg.from)
			continue;
		in = true;
		for(const str& g: u.groups)
			if(g == group)
				return true;
		break;
	}

	if(in)
		bot.fc_reply_pm(msg, "ERROR: You do not have sufficient access.");
	else
		bot.fc_reply_pm(msg, "You are not logged in.");

	return false;
}

bool ChanopsIrcBotPlugin::signup(const message& msg)
{
	BUG_COMMAND(msg);

	std::string user, pass, pass2;
	if(!bot.extract_params(msg, {&user, &pass, &pass2}))
		return false;

	bug("user: " << user);
	bug("pass: " << pass);
	bug("pass2: " << pass2);

	if(pass.size() < sizeof(uint32_t))
		return bot.cmd_error_pm(msg, "ERROR: Password must be at least 4 characters long.");

	if(pass != pass2)
		return bot.cmd_error_pm(msg, "ERROR: Passwords don't match.");

	user_r ur;
	if(store.has("user." + user))
		return bot.cmd_error_pm(msg, "ERROR: User already registered.");

	ur.user = user;
	ur.groups.insert(G_USER);
	ur.sum = checksum(pass);

	store.set("user." + user, ur);

	bot.fc_reply_pm(msg, "Successfully registered.");
	return true;
}

bool ChanopsIrcBotPlugin::login(const message& msg)
{
	BUG_COMMAND(msg);

	std::string user;
	std::string pass;
	if(!bot.extract_params(msg, {&user, &pass}))
		return false;

	bug("user: " << user);
	bug("pass: " << pass);
	uint32_t sum = checksum(pass);
	bug("sum: " << std::hex << sum);

	if(!store.has("user." + user))
		return bot.cmd_error_pm(msg, "ERROR: Username not found.");

	user_r ur;
	ur = store.get("user." + user, ur);

	if(ur.groups.count(G_BANNED))
		return bot.cmd_error_pm(msg, "ERROR: Banned");

	if(ur.sum != sum)
		return bot.cmd_error_pm(msg, "ERROR: Bad password");

	user_t u(msg, ur);

	lock_guard lock(users_mtx);
	if(users.count(u))
		return bot.cmd_error_pm(msg, "You are already logged in to " + bot.nick);

	users.insert(u);

	bot.fc_reply_pm(msg, "You are now logged in to " + bot.nick);

	apply_acts(u);

	return true;
}

void ChanopsIrcBotPlugin::apply_acts(const user_t& u)
{
	if(u.groups.count(G_BANNED))
		irc->mode(u.nick, "+b");
	else if(u.groups.count(G_OPPED))
		irc->mode(u.nick, "+o");
	else if(u.groups.count(G_VOICED))
		irc->mode(u.nick, "+v");
}

bool ChanopsIrcBotPlugin::list_users(const message& msg)
{
	BUG_COMMAND(msg);

	if(!permit(msg))
		return false;

	bot.fc_reply_pm(msg, "Logged in users:");

	lock_guard lock(users_mtx);

	for(const user_t& u: users)
		bot.fc_reply_pm(msg, u.user + ": " + u.prefix);
	return true;
}

bool ChanopsIrcBotPlugin::ban(const message& msg)
{
	BUG_COMMAND(msg);

	// MODE #skivvy +b *!*Skivvy@*.users.quakenet.org
	// MODE #skivvy -b *!*Skivvy@*.users.quakenet.org
	if(!permit(msg))
		return false;

	if(!msg.from_channel())
		return bot.cmd_error_pm(msg, "ERROR: !ban can only be used from a channel.");

	str channel = msg.to;

	// <nick>|<regex>
	if(bot.nicks.find(msg.get_nick()) != bot.nicks.end()) // ban by nick
	{
		store.set("ban.nick." + channel, msg.get_nick());
		irc->mode(msg.to, "+b *!*" + msg.get_nick() + "@*");
		irc->kick({msg.to}, {msg.get_user()}, "Bye bye.");
	}
	else // ban by regex on userhost
	{
		store.set("ban.preg." + channel, msg.get_user_params());
		irc->mode(msg.to, "+b *!*" + msg.get_userhost());
		irc->kick({msg.to}, {msg.get_user()}, "Bye bye.");
	}
	return true;
}

bool ChanopsIrcBotPlugin::reclaim(const message& msg)
{
	BUG_COMMAND(msg);

	str nick;
	if(!bot.extract_params(msg, {&nick}))
		return false;

	bug("nick: " << nick);

	return true;
}

// every function belongs to a group

// INTERFACE: BasicIrcBotPlugin

bool ChanopsIrcBotPlugin::initialize()
{
//	std::ifstream ifs(bot.getf(BAN_FILE, BAN_FILE_DEFAULT));
	perms =
	{
		{"!users", G_USER}
		, {"!reclaim", G_USER}
		, {"!ban", G_OPER}
	};

	add
	({
		"register" // no ! indicated PM only command
		, "register <username> <password> <password>"
		, [&](const message& msg){ signup(msg); }
	});
	add
	({
		"login" // no ! indicated PM only command
		, "login <username> <password>"
		, [&](const message& msg){ login(msg); }
	});
	add
	({
		"!users"
		, "!users List logged in users."
		, [&](const message& msg){ list_users(msg); }
	});
	add
	({
		"!ban"
		, "!ban <nick>|<regex> - ban either a registered user OR a regex match on userhost."
		, [&](const message& msg){ ban(msg); }
	});
	bot.add_monitor(*this);
	return true;
}

// INTERFACE: IrcBotPlugin

std::string ChanopsIrcBotPlugin::get_name() const { return NAME; }
std::string ChanopsIrcBotPlugin::get_version() const { return VERSION; }

void ChanopsIrcBotPlugin::exit()
{
//	bug_func();
}

// INTERFACE: IrcBotMonitor

void ChanopsIrcBotPlugin::event(const message& msg)
{
	if(msg.cmd == "JOIN")// && bot.get(GREET_JOINERS, GREET_JOINERS_DEFAULT))
		join_event(msg);
}

bool ChanopsIrcBotPlugin::join_event(const message& msg)
{
	BUG_COMMAND(msg);

	bug_var(bot.get(GREET_JOINERS, GREET_JOINERS_DEFAULT));

	// bans
	str_vec nicks = store.get_vec("ban.nick." + msg.to);
	for(const str& nick: nicks)
		if(nick == msg.get_nick())
		{
			bot.fc_reply(msg, "NICK: " + nick + " is banned from this channel.");
			irc->mode(msg.to, "+b *!*" + nick + "@*");
			irc->kick({msg.to}, {msg.get_user()}, "Bye bye.");
			return true;
		}

	str_vec pregs = store.get_vec("ban.preg." + msg.to);
	for(const str& preg: pregs)
		if(bot.preg_match(msg.get_userhost(), preg))
		{
			bot.fc_reply(msg, "USERHOST: " + msg.get_userhost() + " is banned from this channel.");
			irc->mode(msg.to, "+b *!*" + msg.get_userhost());
			irc->kick({msg.to}, {msg.get_user()}, "Bye bye.");
			return true;
		}

	if(bot.get(GREET_JOINERS, GREET_JOINERS_DEFAULT))
	{
		str_vec ungreets;
		std::ifstream ifs(bot.getf(UNGREET_FILE, UNGREET_FILE_DEFAULT));

		str ungreet;
		while(ifs >> ungreet)
			ungreets.push_back(ungreet);

		if((ungreets.empty()
		|| stl::find(ungreets, msg.get_nick()) == ungreets.end())
		&& msg.get_nick() != bot.nick)
		{
			str_vec greets = bot.get_vec(GREETINGS_VEC);
			if(!greets.empty())
			{
				siz min_delay = bot.get(GREET_MIN_DELAY, GREET_MIN_DELAY_DEFAULT);
				siz max_delay = bot.get(GREET_MAX_DELAY, GREET_MAX_DELAY_DEFAULT);
				str greet = greets[rand_int(0, greets.size() - 1)];

				for(siz pos = 0; (pos = greet.find("*")) != str::npos;)
					greet.replace(pos, 1, msg.get_nick());

				// greet only once
				ungreets.push_back(msg.get_nick());

				std::async(std::launch::async, [&,msg,greet,min_delay,max_delay]
				{
					std::this_thread::sleep_for(std::chrono::seconds(rand_int(min_delay, max_delay)));
					bot.fc_reply(msg, greet); // irc->say(msg.to) ?
				});
			}
		}
	}

	// Auto OP/VOICE/MODE

	str who, chan;
	str_vec v = bot.get_vec("chanops.op");
	bug_var(v.size());
	for(const str& s: v)
	{
		bug_var(s);
		bug_var(msg.from);
		std::istringstream(s) >> chan >> who;
		bug_var(chan);
		bug_var(who);
		if(bot.preg_match(msg.params, chan, true) && bot.preg_match(msg.from, who))
//		if((chan == "#*" || msg.params == chan) && bot.preg_match(msg.from, who))
		{
			bug("match:");
			irc->mode(msg.params, "+o" , msg.get_nick());
		}
	}

	v = bot.get_vec("chanops.voice");
	for(const str& s: v)
	{
		std::istringstream(s) >> chan >> who;
		if(bot.preg_match(msg.params, chan, true) && bot.preg_match(msg.from, who))
//		if((chan == "#*" || msg.params == chan) && bot.preg_match(msg.from, who))
			irc->mode(msg.params, "+v", msg.get_nick());
	}

	v = bot.get_vec("chanops.kick");
	for(const str& s: v)
	{
		str why;
		std::getline(std::istringstream(s) >> chan >> who, why);
		if(bot.preg_match(msg.params, chan, true) && bot.preg_match(msg.from, who))
			irc->kick({msg.params}, {msg.get_nick()}, why);
	}

	v = bot.get_vec("chanops.mode");
	for(const str& s: v)
	{
		str mode;
		std::istringstream(s) >> chan >> who >> mode;
		if(bot.preg_match(msg.params, chan, true) && bot.preg_match(msg.from, who))
//		if((chan == "#*" || msg.params == chan) && bot.preg_match(msg.from, who))
			irc->mode(msg.params, mode , msg.get_nick());
	}

	return true;
}

}} // sookee::ircbot

