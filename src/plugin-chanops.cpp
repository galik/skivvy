/*
 * ircbot-chanops.cpp
 *
 *  Created on: 03 Aug 2011
 *      Author: oasookee@googlemail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oasookee@googlemail.com               |
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

static const str BAN_FILE = "chanops.ban_file";
static const str BAN_FILE_DEFAULT = "chanops-bans.txt";
static const str USER_FILE = "chanops.user_file";
static const str USER_FILE_DEFAULT = "chanops-users.txt";
const str GREET_JOINERS = "chanops.greet_joiners";
const bool GREET_JOINERS_DEFAULT = false;
const str UNGREET_FILE = "chanops.ungreet_file";
const str UNGREET_FILE_DEFAULT = "chanops-ungreets.txt";
const str GREETINGS_VEC = "chanops.greet";
const str GREET_MIN_DELAY = "chanops.greet_min_delay";
const siz GREET_MIN_DELAY_DEFAULT = 1;
const str GREET_MAX_DELAY = "chanops.greet_max_delay";
const siz GREET_MAX_DELAY_DEFAULT = 6;

#define CHANOPS_PERM(name,ord) \
static const ChanopsIrcBotPlugin::perm_set P_##name(~(-1U << ord)); \
static const ChanopsIrcBotPlugin::perm_set R_##name((1U << ord) >> 1)

CHANOPS_PERM(NONE, 0);
CHANOPS_PERM(USER, 1);
CHANOPS_PERM(OPER, 2);
CHANOPS_PERM(SUPR, 3);
CHANOPS_PERM(ROOT, 4);

static uint32_t checksum(const std::string& pass)
{
	if(pass.size() < sizeof(uint32_t)) return 0;
	uint32_t sum = 0xAA55AA55; // salt
	for(size_t i = 0; i < pass.size() - sizeof(uint32_t); ++i)
		sum *= *reinterpret_cast<const uint32_t*>(&pass[i]);
	return sum;
}

std::istream& operator>>(std::istream& is, ChanopsIrcBotPlugin::user_rec& ur)
{
	// user:00000000:pass
	// SooKee:00000000:8860884
//	is >> std::ws;
//	std::getline(is, ur.user, ':');
//	is >> ur.perms;
//	is.ignore();
//	is >> std::hex >> ur.sum;
//	bug("is: " << bool(is));
	char c;
	std::getline(is, ur.user, ':');
	is >> ur.perms;
	is >> c >> std::hex >> ur.sum;
	std::getline(is, ur.acts);
	return is;
}

std::ostream& operator<<(std::ostream& os, const ChanopsIrcBotPlugin::user_rec& ur)
{
	os << ':' << ur.user;
	os << ':' << ur.perms;
	os << ':' << std::hex << ur.sum;
	os << ':' << ur.acts;
	return os;
}

ChanopsIrcBotPlugin::ChanopsIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
{
}

ChanopsIrcBotPlugin::~ChanopsIrcBotPlugin() {}

bool ChanopsIrcBotPlugin::verify(const message& msg, const perm_set& perms)
{
	BUG_COMMAND(msg);

	if(perms == R_NONE) return true;

	bool in = false;
	for(const user_t& u: users)
		if(u.prefix == msg.from && (in = true) && (u.perms.to_ulong() & perms.to_ulong()))
			return true;
	if(in)
		bot.fc_reply_pm(msg, "ERROR: You do not have sufficient access.");
	else
		bot.fc_reply_pm(msg, "You are not logged in.");

	return false;
}

void ChanopsIrcBotPlugin::signup(const message& msg)
{
	BUG_COMMAND(msg);

	std::string user, pass, pass2;
	if(!extract_params(msg, {&user, &pass, &pass2}))
		return;

	bug("user: " << user);
	bug("pass: " << pass);
	bug("pass2: " << pass2);

	if(pass.size() < sizeof(uint32_t))
	{
		bot.fc_reply_pm(msg, "ERROR: Password must be at least 4 characters long.");
		return;
	}

	if(pass != pass2)
	{
		bot.fc_reply_pm(msg, "ERROR: Passwords don't match.");
		return;
	}

	user_rec ur;
	std::string line;
	std::ifstream ifs(bot.getf(USER_FILE, USER_FILE_DEFAULT));
	while(std::getline(ifs, line))
	{
		trim(line);
		if(line.empty())
			continue;
		if(line[0] == '#')
			continue;
		bug("line:" << line);
		if(std::istringstream(line) >> ur)
		{
			bug("ur:\n" << ur);
			if(ur.user == user)
			{
				bot.fc_reply_pm(msg, "ERROR: User already registered.");
				return;
			}
		}
	}

	ur.user = user;
	ur.perms = P_USER;
	ur.sum = checksum(pass);
	std::ofstream ofs(bot.getf(USER_FILE, USER_FILE_DEFAULT), std::ios::app);
	ofs << ur << '\n';
	bot.fc_reply_pm(msg, "Successfully registered.");
}

bool ChanopsIrcBotPlugin::extract_params(const message& msg, std::initializer_list<str*> args)
{
	std::istringstream iss(msg.get_user_params());
	for(str* arg: args)
		if(!(ios::getstring(iss, *arg)))
		{
			bot.fc_reply_pm(msg, help(msg.get_user_cmd()));
			return false;
		}
	return true;
}

void ChanopsIrcBotPlugin::login(const message& msg)
{
	BUG_COMMAND(msg);

	std::string user;
	std::string pass;
	if(!extract_params(msg, {&user, &pass}))
		return;

	bug("user: " << user);
	bug("pass: " << pass);
	uint32_t sum = checksum(pass);
	bug("sum: " << std::hex << sum);

	user_rec ur;
	std::string line;
	std::ifstream ifs(bot.getf(USER_FILE, USER_FILE_DEFAULT));
	while(std::getline(ifs, line))
	{
		trim(line);
		if(line.empty())
			continue;
		if(line[0] == '#')
			continue;

		bug("line: " << line);

		if(!(std::istringstream(line) >> ur))
			continue;
		bug("ur.user: " << ur.user);
		bug("ur.sum: " << std::hex << ur.sum);

		if(ur.user == user)
		{
			bug("user match!");
			if(ur.sum == sum)
			{
				user_t u(msg.from, ur.user, ur.perms);
				lock_guard lock(users_mtx);
				if(users.count(u))
				{
					bot.fc_reply_pm(msg, "You are already logged in to " + bot.nick);
					return;
				}
				u.nick = msg.get_sender();
				users.insert(u);
				bot.fc_reply_pm(msg, "You are now logged in to " + bot.nick);
				//irc->mode(u.flags);
				return;
			}
			bot.fc_reply_pm(msg, "ERROR: Bad password");
			return;
		}
	}
	bot.fc_reply_pm(msg, "ERROR: Username not found.");
}

void ChanopsIrcBotPlugin::list_users(const message& msg)
{
	BUG_COMMAND(msg);

	if(!verify(msg, R_USER))
		return;

	bot.fc_reply_pm(msg, "Logged in users:");

	lock_guard lock(users_mtx);

	for(const user_t& u: users)
		bot.fc_reply_pm(msg, u.user + ": " + u.prefix);
}

void ChanopsIrcBotPlugin::ban(const message& msg)
{
	BUG_COMMAND(msg);

	// MODE #skivvy +b *!*Skivvy@*.users.quakenet.org
	// MODE #skivvy -b *!*Skivvy@*.users.quakenet.org
	if(!verify(msg, R_OPER))
		return;

	std::string channel;
	std::string nick;
	if(!extract_params(msg, {&channel, &nick}))
		return;

	bug("nick: " << nick);

	irc->mode(channel, "+b *!*" + nick + "@*");

	bot.fc_reply_pm(msg, "Logged in users:");
	users_mtx.lock();
	for(const user_t& u: users)
		bot.fc_reply_pm(msg, u.prefix);
	users_mtx.unlock();
}

void ChanopsIrcBotPlugin::reclaim(const message& msg)
{
	BUG_COMMAND(msg);

	std::string nick;
	if(!extract_params(msg, {&nick}))
		return;

	bug("nick: " << nick);

	bot.fc_reply_pm(msg, "Attempting to claim nick: " + nick);
	users_mtx.lock();
	for(const user_t& u: users)
		bot.fc_reply_pm(msg, u.prefix);
	users_mtx.unlock();
}

// INTERFACE: BasicIrcBotPlugin

bool ChanopsIrcBotPlugin::initialize()
{
	std::ifstream ifs(bot.getf(BAN_FILE, BAN_FILE_DEFAULT));

	add
	({
		"register" // no ! indicated PM only command
		, "register <nick> <password> <password>"
		, [&](const message& msg){ signup(msg); }
	});
	add
	({
		"login" // no ! indicated PM only command
		, "login <nick> <password>"
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
		"!claim"
		, "!claim <nick> Keep trying to change nick to one already in use."
		, [&](const message& msg){ reclaim(msg); }
	});
	add
	({
		"!reclaim"
		, "!claim <nick> Keep trying to change nick to one already in use."
		, [&](const message& msg){ reclaim(msg); }
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
	if(msg.cmd == "JOIN" && bot.get(GREET_JOINERS, GREET_JOINERS_DEFAULT))
		join_event(msg);
}

void ChanopsIrcBotPlugin::join_event(const message& msg)
{
	BUG_COMMAND(msg);

	str_vec ungreets;
	std::ifstream ifs(bot.getf(UNGREET_FILE, UNGREET_FILE_DEFAULT));

	str ungreet;
	while(ifs >> ungreet)
		ungreets.push_back(ungreet);

	if((ungreets.empty()
	|| stl::find(ungreets, msg.get_sender()) == ungreets.end())
	&& msg.get_sender() != bot.nick)
	{
		str_vec greets = bot.get_vec(GREETINGS_VEC);
		if(!greets.empty())
		{
			siz min_delay = bot.get(GREET_MIN_DELAY, GREET_MIN_DELAY_DEFAULT);
			siz max_delay = bot.get(GREET_MAX_DELAY, GREET_MAX_DELAY_DEFAULT);
			str greet = greets[rand_int(0, greets.size() - 1)];

			for(siz pos = 0; (pos = greet.find("*")) != str::npos;)
				greet.replace(pos, 1, msg.get_sender());

			// greet only once
			ungreets.push_back(msg.get_sender());

			std::async(std::launch::async, [&,msg,greet,min_delay,max_delay]
			{
				std::this_thread::sleep_for(std::chrono::seconds(rand_int(min_delay, max_delay)));
				bot.fc_reply(msg, greet); // irc->say(msg.to) ?
			});
		}
	}
}

}} // sookee::ircbot

