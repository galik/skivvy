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
#include <skivvy/irc-constants.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(ChanopsIrcBotPlugin);
PLUGIN_INFO("Channel Operations", "0.1");

using namespace skivvy;
using namespace skivvy::irc;
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

const str CHANOPS_TAKEOVER_DEOP = "chanops.takeover.deop";
const str CHANOPS_TAKEOVER_BAN = "chanops.takeover.ban";
const str CHANOPS_TAKEOVER_KEY = "chanops.takeover.key";


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
		irc->mode(msg.to, " +b *!*" + msg.get_nick() + "@*");
		irc->kick({msg.to}, {msg.get_user()}, "Bye bye.");
	}
	else // ban by regex on userhost
	{
		store.set("ban.preg." + channel, msg.get_user_params());
		irc->mode(msg.to, " +b *!*" + msg.get_userhost());
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
//	BUG_COMMAND(msg);
	if(msg.cmd == RPL_NAMREPLY)
		name_event(msg);
	else if(msg.cmd == NICK)
		nick_event(msg);
	else if(msg.cmd == RPL_WHOISUSER)
		whoisuser_event(msg);
	else if(msg.cmd == "JOIN")
		join_event(msg);
	else if(msg.cmd == "MODE")
		mode_event(msg);
}


bool ChanopsIrcBotPlugin::nick_event(const message& msg)
{
	BUG_COMMAND(msg);
	return true;
}

bool ChanopsIrcBotPlugin::whoisuser_event(const message& msg)
{
	BUG_COMMAND(msg);
	// -----------------------------------------------------
	//                  from: dreamhack.se.quakenet.org
	//                   cmd: 311
	//                params: Skivvy00 SooKee ~SooKee SooKee.users.quakenet.org *
	//                    to: Skivvy00
	//                  text: SooKee
	// msg.from_channel()   : false
	// msg.get_nick()       : dreamhack.se.quakenet.org
	// msg.get_user()       : dreamhack.se.quakenet.org
	// msg.get_host()       : dreamhack.se.quakenet.org
	// msg.get_userhost()   : dreamhack.se.quakenet.org
	// msg.get_user_cmd()   : SooKee
	// msg.get_user_params():
	// msg.reply_to()       : dreamhack.se.quakenet.org
	// -----------------------------------------------------

	str skip, nick, userhost;

	lock_guard lock(nicks_mtx);
	if(siss(msg.params) >> skip >> nick >> userhost)
		nicks[nick] = userhost;

	return true;
}

bool ChanopsIrcBotPlugin::name_event(const message& msg)
{
	BUG_COMMAND(msg);
	// :dreamhack.se.quakenet.org 353 Skivvy = #openarena :Skivvy +SooKee +I4C @OAbot +Light3r +pet

	str skip, chan;
	siss iss(msg.params);
	sgl(iss, skip, '#') >> chan;
	chan.insert(0, "#");
	bug_var(chan);

	str tb_chan = bot.get("chanops.takover.chan");

	// get nicks of everyome in the take-back channel.
//	if(chan == tb_chan) // take back channel?
	{
		str nick;

		iss.clear();
		iss.str(msg.text);
		iss >> nick; // ignogre my nick

		bug_var(nick);

		while(iss >> nick)
		{
			if(!nick.empty())
			{
				lock_guard lock(nicks_mtx);
				if(nick[0] == '@' && nick != "Q" && nick != "@" + bot.nick)
					tb_ops.insert(nick.substr(1));

				irc->whois(nick); // initiate request, see whois_event() for response
				nicks[nick[0] == '+' || nick[0] == '@' ? nick.substr(1) : nick];
			}
		}
	}

	// WHOIS
	// RPL_WHOISUSER     :quakenet.org 311 Skivvy SooKee ~SooKee SooKee.users.quakenet.org * :SooKee
	// RPL_WHOISCHANNELS :quakenet.org 319 Skivvy SooKee :@#skivvy @#openarenahelp +#openarena @#omfg
	// RPL_WHOISSERVER   :quakenet.org 312 Skivvy SooKee *.quakenet.org :QuakeNet IRC Server
	// UD                :quakenet.org 330 Skivvy SooKee SooKee :is authed as
	// RPL_ENDOFWHOIS    :quakenet.org 318 Skivvy SooKee :End of /WHOIS list.


	return true;
}

bool ChanopsIrcBotPlugin::mode_event(const message& msg)
{
	BUG_COMMAND(msg);
	// -----------------------------------------------------
	//                  from: SooKee!~SooKee@SooKee.users.quakenet.org
	//                   cmd: MODE
	//                params: #skivvy-test +b *!*Skivvy0x@*.users.quakenet.org
	//                    to: #skivvy-test
	//                  text: End of /NAMES list.
	// msg.from_channel()   : true
	// msg.get_nick()       : SooKee
	// msg.get_user()       : ~SooKee
	// msg.get_host()       : SooKee.users.quakenet.org
	// msg.get_userhost()   : ~SooKee@SooKee.users.quakenet.org
	// msg.get_user_cmd()   : End
	// msg.get_user_params(): of /NAMES list.
	// msg.reply_to()       : #skivvy-test
	// -----------------------------------------------------

	// :SooKee!~SooKee@SooKee.users.quakenet.org MODE #skivvy-test +b *!*Skivvy@*.users.quakenet.org

	str chan, flag, user;

	if(!(siss(msg.params) >> chan >> flag >> user))
	{
		log("BAD message");
		return false;
	}

	//     from: Q!TheQBot@CServe.quakenet.org
	//      cmd: MODE
	//   params: #skivvy-admin +o Skivvy00
	//       to: #skivvy-admin
	//     text: End of /NAMES list.
	// msg.from_channel()   : true
	// msg.get_nick()       : Q
	// msg.get_user()       : TheQBot
	// msg.get_host()       : CServe.quakenet.org
	// msg.get_userhost()   : TheQBot@CServe.quakenet.org
	// msg.get_user_cmd()   : End
	// msg.get_user_params(): of /NAMES list.
	// msg.reply_to()       : #skivvy-admin
	// -----------------------------------------------------

	str tb_chan = bot.get("chanops.takover.chan");

	if(chan == tb_chan) // take back channel?
	{
		// did I just get ops?
		if(msg.from == "Q!TheQBot@CServe.quakenet.org" && msg.params == tb_chan + " +o " + bot.nick)
		{
			lock_guard lock(nicks_mtx);

			// kick all ops
			str sep, kick_nicks;
			for(const str& nick: tb_ops)
				{ kick_nicks += sep + nick; sep = ","; }

			const str TAKEOVER_KICK_MSG;
			const str TAKEOVER_KICK_MSG_DEFAULT = "Reclaiming channel.";
			if(!kick_nicks.empty())
				irc->kick({chan}, {kick_nicks}, bot.get(TAKEOVER_KICK_MSG, TAKEOVER_KICK_MSG_DEFAULT));

			// ban who you need to ban
			for(const str& mask: bot.get_vec(CHANOPS_TAKEOVER_BAN))
				irc->mode(chan, " +b " + mask);

			// ban all ops nicks
			for(const str& nick: tb_ops)
				irc->mode(chan, " +b " + nick + "!*@*");

			// open channel
			irc->mode(chan, " -i");
			irc->mode(chan, " -p");

			if(bot.has(CHANOPS_TAKEOVER_KEY))
				irc->mode(chan, " -k " + bot.get(CHANOPS_TAKEOVER_KEY));
		}
		// Now set up a thread banning ops more thoroughly as their
		// whois data arrives

		std::async(std::launch::async, [&]()
		{
			time_t start = std::time(0);
			while(std::time(0) - start < 10 && !tb_ops.empty())
			{
				lock_guard lock(nicks_mtx);
				for(str_set_citer oi = tb_ops.begin(); oi != tb_ops.end();)
				{
					if(!nicks[*oi].empty())
					{
						bug_var(nicks[*oi]);
						str mask = "*!*@*";
						siz pos = nicks[*oi].rfind(".");
						bug_var(pos);
						pos = nicks[*oi].rfind(".", --pos);
						bug_var(pos);
						mask += nicks[*oi].substr(0, pos);
						bug_var(mask);
						irc->mode(chan, " +b " + mask);
						oi = tb_ops.erase(oi);
					}
					else
						++oi;
				}

			}
		});
	}

	bug_var(chan);
	bug_var(flag);
	bug_var(user);

	str_vec v = bot.get_vec("chanops.protect");
	bug_var(v.size());
	for(const str& s: v)
	{
		bug_var(s);
		str chan_preg, who;
		std::istringstream(s) >> chan_preg >> who;
		bug_var(chan_preg);
		bug_var(who);
		if(bot.preg_match(chan_preg, msg.to, true) && bot.wild_match(user, who))
		{
			bug("match:");
			if(flag == "+b")
				flag = "-b";
			else if(flag == "-o")
				flag = "+b";
			else if(flag == "-v")
				flag = "+v";
			irc->mode(chan, flag , who);
		}
	}

	return true;
}

bool ChanopsIrcBotPlugin::join_event(const message& msg)
{
//	BUG_COMMAND(msg);

//	bug_var(bot.get(GREET_JOINERS, GREET_JOINERS_DEFAULT));

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
		if(bot.preg_match(preg, msg.get_userhost()))
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
//	bug_var(v.size());
	for(const str& s: v)
	{
//		bug_var(s);
//		bug_var(msg.from);
		std::istringstream(s) >> chan >> who;
//		bug_var(chan);
//		bug_var(who);
		if(bot.preg_match(chan, msg.params, true) && bot.preg_match(who, msg.from))
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
		if(bot.preg_match(chan, msg.params, true) && bot.preg_match(who, msg.from))
			irc->mode(msg.params, "+v", msg.get_nick());
	}

	v = bot.get_vec("chanops.kick");
	for(const str& s: v)
	{
		str why;
		std::getline(std::istringstream(s) >> chan >> who, why);
		if(bot.preg_match(chan, msg.params, true) && bot.preg_match(who, msg.from))
			irc->kick({msg.params}, {msg.get_nick()}, why);
	}

	v = bot.get_vec("chanops.mode");
	for(const str& s: v)
	{
		str mode;
		std::istringstream(s) >> chan >> who >> mode;
		if(bot.preg_match(chan, msg.params, true) && bot.preg_match(who, msg.from))
			irc->mode(msg.params, mode , msg.get_nick());
	}

	return true;
}

}} // sookee::ircbot

