/*
 * ircbot.cpp
 *
 *  Created on: 29 Jul 2011
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

#include <skivvy/ircbot.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <future>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <cstdio>
#include <iterator>

#include <cstring>
#include <cstdlib>
#include <dirent.h>

#include <skivvy/ios.h>
#include <skivvy/stl.h>
#include <skivvy/str.h>
#include <skivvy/logrep.h>
#include <skivvy/irc-constants.h>

#include <random>
#include <functional>

namespace skivvy { namespace ircbot {

PLUGIN_INFO("IrcBot", "0.1");

using namespace skivvy;
using namespace skivvy::irc;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

const str SERVER_HOST = "server_host";
const str SERVER_PORT = "server_port";
const siz SERVER_PORT_DEFAULT = 6667;

static const str PROP_PASSWORD = "password";
static const str PROP_WELCOME = "welcome";
static const str PROP_GOODBYE = "goodbye";
static const str PROP_ON_CONNECT = "on_connect";
static const str PROP_SERVER_PASSWORD = "server_password";
static const str PROP_SERVER_RETRIES = "server_retries";


// :port80a.se.quakenet.org 353 Skivvy = #oa-ictf :Skivvy @SooKee pet @Q
std::istream& parsemsg(std::istream& is, message& m)
{
	str line;
	std::getline(is, line);

	if(!line.empty())
	{
		if(line[0] == ':')
		{
			std::istringstream iss(line.substr(1));
			iss >> m.from >> m.cmd;
			std::getline(iss, m.params, ':');
			std::getline(iss, m.text);
			iss.clear();
			iss.str(m.params);
			iss >> m.to;
		}
		else
		{
			std::istringstream iss(line);
			iss >> m.cmd;
			std::getline(iss, m.params, ':');
			std::getline(iss, m.text);
			iss.clear();
			iss.str(m.params);
			iss >> m.to;
		}

		trim(m.from);
		trim(m.cmd);
		trim(m.params);
		trim(m.to);
		trim(m.text);
	}

	return is;
}

std::ostream& printmsg(std::ostream& os, const message& m)
{
	os << "                 from: " << m.from << '\n';
	os << "                  cmd: " << m.cmd << '\n';
	os << "               params: " << m.params << '\n';
	os << "                   to: " << m.to << '\n';
	os << "                 text: " << m.text << '\n';
	os << "msg.from_channel()   : " << m.from_channel() << '\n';
	os << "msg.get_sender()     : " << m.get_sender() << '\n';
	os << "msg.get_user_cmd()   : " << m.get_user_cmd() << '\n';
	os << "msg.get_user_params(): " << m.get_user_params() << '\n';
	os << "msg.reply_to()       : " << m.reply_to() << '\n';
	return os << std::flush;
}

std::istream& operator>>(std::istream& is, message& m)
{
	std::getline(is, m.from);
	std::getline(is, m.cmd);
	std::getline(is, m.params);
	std::getline(is, m.to);
	std::getline(is, m.text);
	return is;
}

std::ostream& operator<<(std::ostream& os, const message& m)
{
	os << m.from << '\n';
	os << m.cmd << '\n';
	os << m.params << '\n';
	os << m.to << '\n';
	os << m.text;
	return os;
}

str message::get_sender() const
{
	str sender;
	if(!from.empty())
		sender = from.substr(0, from.find("!"));
	return sender;
}

str message::get_user_cmd() const
{
	str cmd;
	std::istringstream iss(text);
	std::getline(iss, cmd, ' ');
	return trim(cmd);
}

str message::get_user_params() const
{
	str params;
	std::istringstream iss(text);
	std::getline(iss, params, ' ');
	if(!std::getline(iss, params))
		return "";
	return trim(params);
}

bool message::from_channel() const
{
	return !to.empty() && to[0] == '#';
}

str message::reply_to() const
{
	return from_channel() ? to : get_sender();
}

// TODO: Sort this mess out
void bug_message(const std::string& K, const std::string& V, const message& msg)
{
	bug("===============================");
	bug(K << ": " << V);
	bug("-------------------------------");
	bug_msg(msg);
	bug("-------------------------------");
}

#define BUG_MSG(m,M) do{ bug_message(#M, M, m); }while(false)

bool RemoteIrcServer::send(const str& cmd)
{
	log("send: " << cmd);
	return send_unlogged(cmd);
}

bool RemoteIrcServer::send_unlogged(const str& cmd)
{
	mtx_ss.lock();
	ss << cmd.substr(0, 510) << "\r\n" << std::flush;
	mtx_ss.unlock();
	if(!ss)
		log("ERROR: send failed.");
	return ss;
}

RemoteIrcServer::RemoteIrcServer()
{
}

bool RemoteIrcServer::connect(const str& host, long port) { return ss.open(host, port); }

bool RemoteIrcServer::pass(const str& pwd)
{
	if(pwd.empty())
		return send(PASS + " none");
	return send(PASS + " " + pwd);
}

bool RemoteIrcServer::nick(const str& n)
{
	return send(NICK + " " + n);
}

bool RemoteIrcServer::user(const str& u, const str m, const str r)
{
	return send(USER + " " + u + " " + m + " * :" + r);
}

bool RemoteIrcServer::join(const str& channel)
{
	return send(JOIN + " " + channel);
}

bool RemoteIrcServer::part(const str& channel, const str& message)
{

	return send(PART + " " + channel + " " + message);
}

bool RemoteIrcServer::ping(const str& info)
{
	return send_unlogged(PING + " " + info);
}

bool RemoteIrcServer::pong(const str& info)
{
	return send_unlogged(PONG + " " + info);
}

bool RemoteIrcServer::say(const str& to, const str& text)
{
	return send(PRIVMSG + " " + to + " :" + text);
}

bool RemoteIrcServer::me(const str& to, const str& text)
{
	return say(to, "\001ACTION " + text + "\001");
}

bool RemoteIrcServer::query(const str& nick)
{
	return send(QUERY + " " + nick);
}

bool RemoteIrcServer::mode(const str& nick, const str& mode)
{
	return send(MODE + " " + nick + " " + mode);
}

bool RemoteIrcServer::reply(const message& msg, const str& text)
{
	str from;
	if(!msg.from.empty())
		from = msg.from.substr(0, msg.from.find("!"));

	if(!msg.to.empty() && msg.to[0] == '#')
		return say(msg.to, text);
	else
		return say(from, text);

	return false;
}

bool RemoteIrcServer::reply_pm(const message& msg, const str& text)
{
	return say(msg.get_sender(), text);
}

bool RemoteIrcServer::receive(str& line)
{
	return std::getline(ss, line);
}

BasicIrcBotPlugin::BasicIrcBotPlugin(IrcBot& bot)
: bot(bot), irc(0)
{
}

BasicIrcBotPlugin::~BasicIrcBotPlugin() {}

void BasicIrcBotPlugin::add(const action& a) { actions.insert(action_pair{a.cmd, a}); }
//void BasicIrcBotPlugin::add(const oldaction& o)
//{
//	action a;
//	a.cmd = o.cmd;
//	a.help = o.help;
//	a.func = o.func;
//	actions.insert(action_pair{a.cmd, a});
//}

bool BasicIrcBotPlugin::init()
{
	irc = &bot.get_irc_server();
	actions.clear();
	return initialize();
}

// INTERFACE: IrcBotPlugin

str_vec BasicIrcBotPlugin::list() const
{
	str_vec v;
	for(const action_pair& f: actions)
		if(!(f.second.flags & action::INVISIBLE))
			v.push_back(f.first);
	return v;
}
void BasicIrcBotPlugin::execute(const str& cmd, const message& msg)
{
	actions[cmd].func(msg);
}

str BasicIrcBotPlugin::help(const str& cmd) const
{
	if(actions.at(cmd).flags & action::INVISIBLE)
		return "none";
	return actions.at(cmd).help;
}

bool IrcBotPluginLoader::operator()(const str& file, IrcBot& bot)
{
	union { void* dl; void* dlsym; IrcBotPluginPtr(*plugin)(IrcBot&); } ptr;

	log("PLUGIN LOAD: " << file);
//	if(!(ptr.dl = dlopen(file.c_str(), RTLD_NOW|RTLD_GLOBAL)))
	if(!(ptr.dl = dlopen(file.c_str(), RTLD_LAZY|RTLD_GLOBAL)))
	{
		log(dlerror());
		return 0;
	}

	if(!(ptr.dlsym = dlsym(ptr.dl, "skivvy_ircbot_factory")))
	{
		log(dlerror());
		return 0;
	}

	IrcBotPluginPtr plugin;
	if(!(plugin = ptr.plugin(bot)))
		return false;

	bot.del_plugin(plugin->get_name());
	bot.add_plugin(plugin);
	return true;
}

void IrcBot::del_plugin(const str& name)
{
//	IrcBotPlugin* pp;
	for(plugin_vec_iter p =  plugins.begin(); p != plugins.end();)
	{
		if((*p)->get_name() == name)
		{
//			(*pp)->
			(*p)->exit();
			//dlclose(p->get()); // TODO add this again
			p = plugins.erase(p);
		}
		else
			++p;
	}
}

// IRCBot

IrcBot::IrcBot()
: irc()
, done(false)
, debug(false)
, connected(false)
, registered(false)
, is(0), os(0)
, nick_number(0)
, uptime(std::time(0))
, config_loaded(0)
{
}

std::istream& operator>>(std::istream& is, IrcBot& bot)
{
	log("Loading properties:");
	bot.props.clear();
	str line;
//	IrcBot::property_iter p = bot.props.end();
	while(std::getline(is, line))
	{
		trim(line);
		if(line.empty() or line[0] == '#') continue;

		if(siz pos = line.find("//") != str::npos)
			line.erase(pos);

		std::istringstream iss(line);
		str key, val;
		std::getline(iss, key, ':');
		iss >> std::ws;
		std::getline(iss, val);

		trim(key);
		trim(val);

		if(key == "nick") bot.info.nicks.push_back(val);
		else if(key == "user") bot.info.user = val;
		else if(key == "mode") bot.info.mode = val;
		else if(key == "real") bot.info.real = val;
		else if(key == "join") bot.add_channel(val);
		else if(key == "ban") bot.banned.insert(val);
//		else p = bot.props.insert(p, std::make_pair(key, val));
		else bot.props[key].push_back(val);
	}
	if(is.eof())
		is.clear();
	return is;
}

////    0: *!user@host.domain
////    1: *!*user@host.domain
////    2: *!*@host.domain
////    3: *!*user@*.domain
////    4: *!*@*.domain
////    5: nick!user@host.domain
////    6: nick!*user@host.domain
////    7: nick!*@host.domain
////    8: nick!*user@*.domain
////    9: nick!*@*.domain

void IrcBot::add_plugin(IrcBotPluginPtr plugin)
{
	if(plugin)
		this->plugins.push_back(plugin);
	else
		log("ERROR: Adding non-plugin.");
}

bool IrcBot::has_plugin(const str& name, const str&  version)
{
	for(const IrcBotPluginPtr& p: plugins)
		if(p->get_name() == name && p->get_version() >= version)
			return true;
	return false;
}

void IrcBot::add_monitor(IrcBotMonitor& m) { monitors.insert(&m); }

void IrcBot::add_rpc_service(IrcBotRPCService& s)
{
	services[s.get_name()] = &s;
}

void IrcBot::add_channel(const str& c) { chans.insert(c); }

RemoteIrcServer& IrcBot::get_irc_server() { return irc; }

// Utility

// flood control
bool IrcBot::fc_reply(const message& msg, const str& text)
{
//	return fc.send([&,msg,text]()->bool{ return irc.reply(msg, text); });
	return fc.send(msg.to, [&,msg,text]()->bool{ return irc.reply(msg, text); });
}

bool IrcBot::fc_reply_help(const message& msg, const str& text, const str& prefix)
{
	const str_vec v = split(text, '\n');
	for(const str& s: v)
//		if(!fc.send([&,msg,prefix,s]()->bool{ return irc.reply(msg, prefix + s); }))
//			return false;
		if(!fc.send(msg.to, [&,msg,prefix,s]()->bool{ return irc.reply(msg, prefix + s); }))
			return false;
	return true;
}

bool IrcBot::fc_reply_pm(const message& msg, const str& text)//, size_t priority)
{
//	bug_func();
//	return fc.send([&,msg,text]()->bool{ return irc.reply_pm(msg, text); });
	return fc.send(msg.to, [&,msg,text]()->bool{ return irc.reply_pm(msg, text); });
}

bool IrcBot::fc_reply_pm_help(const message& msg, const str& text, const str& prefix)
{
	const str_vec v = split(text, '\n');
	for(const str& s: v)
//		if(!fc.send([&,msg,prefix,s]()->bool{ return irc.reply_pm(msg, prefix + s); }))
//			return false;
		if(!fc.send(msg.to, [&,msg,prefix,s]()->bool{ return irc.reply_pm(msg, prefix + s); }))
			return false;
	return true;
}

bool IrcBot::cmd_error(const message& msg, const str& text, bool rv)
{
	fc_reply(msg, msg.get_user_cmd() + ": " + text);
	return rv;
}

str IrcBot::locate_file(const str& name)
{
	if(name.empty() || name[0] == '/')
		return name;
	return get(DATA_DIR, str(std::getenv("HOME")) + "/" + ".skivvy") + "/" + name;
}

// INTERFACE: IrcBotPlugin

str IrcBot::get_name() const { return NAME; }
str IrcBot::get_version() const { return VERSION; }

void IrcBot::official_join(const str& channel)
{
	std::this_thread::sleep_for(std::chrono::seconds(2));

	irc.join(channel);
	irc.say(channel, get_name() + " v" + get_version());

	str_vec welcomes = get_vec(PROP_WELCOME);
	if(!welcomes.empty())
		irc.say(channel, welcomes[rand_int(0, welcomes.size() - 1)]);
}

// =====================================
// LOAD DYNAMIC PLUGINS
// =====================================

/**
 * Get directory listing.
 */
int ls(const str& folder, str_vec &files)
{
    DIR* dir;
	dirent* dirp;

	if(!(dir = opendir(folder.c_str())))
		return errno;

	while((dirp = readdir(dir)))
		files.push_back(dirp->d_name);

	if(closedir(dir))
		return errno;

	return 0;
}

#ifndef DEFAULT_PLUGIN_DIR
#define DEFAULT_PLUGIN_DIR "/usr/local/share/skivvy/plugins"
#endif

void load_plugins(IrcBot& bot)
{
	IrcBotPluginLoader load;

	str_vec plugin_dirs;
	plugin_dirs.push_back(DEFAULT_PLUGIN_DIR);

	if(bot.has("plugin_dir"))
	{
		str_vec dirs = bot.get_vec("plugin_dir");
		plugin_dirs.insert(plugin_dirs.end(), dirs.begin(), dirs.end());
	}

	str_vec pv = bot.get_vec("plugin");

	for(const str& plugin_dir: plugin_dirs)
	{
		log("Searching plugin dir: " << plugin_dir);

		str_vec files;
		if(int e = ls(plugin_dir, files))
			log(strerror(e));
//
//		for(const str& file: files)
//		{
//			if(stl::find(pv, file) != pv.end())
//				if(load(plugin_dir + "/" + file, bot))
//					pv.erase(stl::find(pv, file));
//		}

		for(const str& p: pv)
		{
			if(stl::find(files, p) != files.end())
				if(!load(plugin_dir + "/" + p, bot))
					log("Failed to load: " << p);
		}
	}
}

struct msgevent
{
	time_t when;
	siz timeout = 30; // 30 seconds
	std::function<void(const message&)> func;
	msgevent(): when(std::time(0)) {}
};
typedef std::multimap<str, msgevent> msgevent_map;
typedef std::pair<const str, msgevent> msgevent_pair;
typedef msgevent_map::iterator msgevent_itr;
typedef msgevent_map::const_iterator msgevent_citr;
typedef std::pair<msgevent_itr, msgevent_itr> msgevent_range;
msgevent_map msgevents;
std::mutex msgevent_mtx;

void dispatch_msgevent(const message& msg)
{
	time_t now = std::time(0);
	lock_guard lock(msgevent_mtx);
	msgevent_range mr = msgevents.equal_range(msg.cmd);
	for(msgevent_itr mi = mr.first; mi != mr.second; mi = msgevents.erase(mi))
		if(now - mi->second.when < int(mi->second.timeout))
			mi->second.func(msg);
}

bool IrcBot::init(const str& config_file)
{
	std::srand(std::time(0));

	// =====================================
	// CONFIGURE BOT
	// =====================================

	// default
	configfile = str(std::getenv("HOME")) + "/.skivvy/skivvy.conf";

	if(!config_file.empty() && std::ifstream(config_file))
		log("Using config file: " << (configfile = config_file));

	if(!(std::ifstream(configfile) >> (*this)))
	{
		log("Error reading config file.");
		return false;
	}

	config_loaded = std::time(0);

	// =====================================
	// OPEN LOG FILE
	// =====================================

	std::ofstream logfile;

	if(has("log_file"))
	{
		logfile.open(getf("log_file"));
		if(logfile)
		{
			botlog(&logfile);
			botbug(&logfile);
		}
	}

	load_plugins(*this);

	if(!have(SERVER_HOST))
	{
		log("ERROR: No server configured.");
		return false;
	}

	host = get(SERVER_HOST);
	port = get(SERVER_PORT, SERVER_PORT_DEFAULT);

	if(info.nicks.empty())
	{
		log("ERROR: No nick set.");
		return false;
	}

	log(get_name() << " v" << get_version());

	fc.start();

	// Initialise plugins
	plugin_vec_iter p = plugins.begin();
	while(p != plugins.end())
	{
		if(!(*p))
		{
			log("Null plugin found during initialisation.");
			p = plugins.erase(p);
			continue;
		}
		if(!(*p)->init())
		{
			log("Plugin failed during initialization.");
			p = plugins.erase(p);
			continue;
		}
		log("\tPlugin initialised: " << (*p)->get_name() << " v" << (*p)->get_version());
		for(str& c: (*p)->list())
		{
			log("\t\tRegister command: " << c);
			commands[c] = *p;
		}
		++p;
	}

	// start pinger
	std::future<void> png = std::async(std::launch::async, [&]{ pinger(); });

	while(!done && !connected)
		std::this_thread::sleep_for(std::chrono::seconds(1));

	// start console
	std::future<void> con = std::async(std::launch::async, [&]{ console(); });

	str line;
	message msg;
	while(!done)
	{
		if(!irc.receive(line))
		{
			connected = false;
			registered = false;
			while(!done && !connected)
				std::this_thread::sleep_for(std::chrono::seconds(3));
		}

		if(done)
			break;

		if(line.empty())
			continue;

		std::istringstream iss(line);
		parsemsg(iss, msg);

		dispatch_msgevent(msg);

		for(IrcBotMonitor* m: monitors)
			if(m) m->event(msg);

		// LOGGING

		static const str_vec nologs =
		{
			"PING", "PONG", "372"
		};

		if(stl::find(nologs, msg.cmd) == nologs.cend())
			log("recv: " << line);

		if(msg.cmd == PING)
		{
			irc.pong(msg.text);
		}
		else if(msg.cmd == RPL_WELCOME)
		{
			BUG_MSG(msg, RPL_WELCOME);
			this->nick = msg.to;
			for(str prop: get_vec(PROP_ON_CONNECT))
				exec(replace(prop, "$me", nick));
			for(const str& channel: chans)
				official_join(channel);
			registered = true;
		}
		else if(msg.cmd == RPL_NAMREPLY)
		{
			BUG_MSG(msg, RPL_NAMREPLY);
			std::istringstream iss(msg.params);
			str channel, nick;
			iss >> channel >> channel >> channel;
			bug("channel: " << channel);
			iss.clear();
			iss.str(msg.text);
			while(iss >> nick)
				if(!nick.empty())
					nicks[channel].insert((nick[0] == '@' || nick[0] == '+')?nick.erase(0, 1):nick);
		}
		else if(msg.cmd == ERR_NOORIGIN)
		{
			BUG_MSG(msg, ERR_NOORIGIN);
		}
		else if(msg.cmd == ERR_NICKNAMEINUSE)
		{
			BUG_MSG(msg, ERR_NICKNAMEINUSE);
			if(nick_number < info.nicks.size())
			{
				irc.nick(info.nicks[nick_number++]);
				irc.user(info.user, info.mode, info.real);
			}
			else
			{
				log("ERROR: Unable to connect with any of the nicks.");
				done = true;
			}
		}
		else if(msg.cmd == JOIN)
		{
			BUG_MSG(msg, JOIN);
			// track known nicks
			const str who = msg.get_sender();
			str_set& known = nicks[msg.params];
			if(who != nick && known.find(who) == known.end())
			{
				known.insert(who);
			}
		}
		else if(msg.cmd == KICK)
		{
			BUG_MSG(msg, KICK);
			// TODO: Move this to ChanOps
			std::istringstream iss(msg.params);
			//  params: #google-admin Skivvy
			str k;
			iss >> k >> k;
			irc.me(msg.to, "1waves bye bye to " + k + " :P");
			//irc.say(msg.to, "4Don't let the door hit you on the way out!");
		}
		else if(msg.cmd == NICK)
		{
			// If the bot changed its own nick
			if(msg.get_sender() == nick)
				nick = msg.text;
		}
		else if(msg.cmd == PONG)
		{
			pinger_info_mtx.lock();
			pinger_info = msg.text;
			pinger_info_mtx.unlock();
		}
		else if(msg.cmd == PRIVMSG)
		{
			if(!msg.text.empty() && msg.text[0] == '!')
			{
				str cmd = msg.get_user_cmd();
				log("Processing command: " << cmd);
				if(cmd == "!die")
				{
					if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == msg.get_user_params())
					{
						str_vec goodbyes = get_vec(PROP_GOODBYE);
						for(const str& c: chans)
						{
							if(!goodbyes.empty())
								irc.say(c, goodbyes[rand_int(0, goodbyes.size() - 1)]);
							irc.part(c);
						}
						done = true;
					}
					else
					{
						fc_reply(msg, "Incorrect password.");
					}
				}
				else if(cmd == "!join")
				{
					str_vec param = split_params(msg.get_user_params(), ' ');

					if(param.size() == 2)
					{
						if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == param[1])
						{
							if(irc.join(param[0]))
							{
								chans.insert(param[0]);
								fc_reply(msg, nick + " has requested to join " + param[0]);
							}
						}
						else
						{
							fc_reply(msg, "Incorrect password.");
						}
					}
					else
					{
						fc_reply(msg, "Usage: !join #channel <password>");
					}
				}
				else if(cmd == "!part")
				{
					str_vec param = split_params(msg.get_user_params(), ' ');

					if(param.size() == 2)
					{
						if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == param[1])
						{
							if(irc.part(param[0]))
							{
								chans.insert(param[0]);
								fc_reply(msg, nick + " has requested to part " + param[0]);
							}
						}
						else
						{
							fc_reply(msg, "Incorrect password.");
						}
					}
				}
				else if(cmd == "!pset")
				{
					str_vec params = split_params(msg.get_user_params(), ' ');

					if(params.size() < 2)
					{
						fc_reply(msg, "!pset <password> var: [val1, val2...]");
						continue;
					}
					// !pset <password> var: val1, val2, val3
					if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == params[0])
					{
						std::istringstream iss(msg.get_user_params());
						str key, vals;
						if(std::getline(iss, key, ':') && !trim(key).empty())
						{
							std::getline(iss, vals);
							if(trim(vals).empty())
							{
								// report current values
	//							property_range r = props.equal_range(key);
								std::ostringstream oss;
								oss << key << ":";
								for(const str& val: props[key])
									oss << ' ' << val;
								fc_reply(msg, oss.str());
								continue;
							}
							str_vec valvec = split(vals, ',');
							props.erase(key);
							for(str val: valvec)
								props[key].push_back(trim(val));
							fc_reply(msg, "Property set.");
						}
					}
					else
					{
						fc_reply(msg, "Incorrect password.");
					}
				}
				else if(cmd == "!debug")
				{
					fc_reply(msg, "Debug mode " + str((debug = !debug) ? "on" : "off") + ".");
				}
				else if(cmd == "!help")
				{
					const str sender = msg.get_sender();
					const str params = msg.get_user_params();

					if(!params.empty())
					{
						fc_reply_pm_help(msg, help(params));
						continue;
					}

					if(msg.from.empty())
						continue;

					fc_reply_pm(msg, "List of commands:");
					for(const IrcBotPluginPtr& p: plugins)
					{
						fc_reply_pm(msg, "\t" + p->get_name() + " " + p->get_version());
						std::ostringstream oss;
						oss << "\t\t";
						std::string sep;
						for(str& cmd: p->list())
						{
							oss << sep << cmd;
							sep = ", ";
						}
						fc_reply_pm(msg, oss.str());
					}
				}
				else
				{
					// TODO: Make this a proper IRC mask
					if(banned.find(msg.get_sender()) == banned.end())
						// TODO: make this async (make plugins thread-safe)
						execute(cmd, msg);
					else log("BANNED: " << msg.get_sender());
				}
			}
			else if(!msg.text.empty() && msg.to == nick)
			{
				// PM to bot accepts commands without !
				str cmd = lowercase(msg.get_user_cmd());
				if(commands.find(cmd) != commands.end())
					execute(cmd, msg);
			}
		}
	}

	log("Closing down plugins:");
	for(IrcBotPluginPtr p: plugins)
	{
		log("\t" << p->get_name());
		p->exit();
	}

	fc.stop();
	log("Closing down pinger:");
	done = true;
	if(png.valid())
		png.get();

	// Don't close console because thread is blocking.
	log("Closing down console:");
	if(con.valid())
		con.get();

	log("ENDED");
	return true;
}

str_vec IrcBot::list() const
{
	str_vec v;
	for(const IrcBotPluginPtr& p: plugins)
		for(str& i: p->list())
			v.push_back(i);
	return v;
}
void IrcBot::execute(const str& cmd, const message& msg)
{
	if(commands.find(cmd) == commands.end())
	{
		log("IrcBot::execute(): Unknown command: " << cmd);
		return;
	}
	if(commands[cmd])
		commands[cmd]->execute(cmd, msg);
}
str IrcBot::help(const str& cmd) const
{
	if(commands.find(cmd) != commands.end())
		return commands.at(cmd)->help(cmd);

	if(commands.find('!' + cmd) != commands.end())
		return commands.at('!' + cmd)->help('!' + cmd);

	return "No help available for " + cmd + ".";
}
void IrcBot::exit()
{
//	dispatch = false;
//	for(IrcBotPlugin* p: plugins)
//		p->exit();
//	if(disp_fut.valid())
//		disp_fut.get();
}

// Free functions

bool wildmatch(str_citer mi, const str_citer me
	, str_citer ii, const str_citer ie)
{
	while(mi != me)
	{
		if(*mi == '*')
		{
			for(str_citer i = ii; i != ie; ++i)
				if(wildmatch(mi + 1, me, i, ie))
					return true;
			return false;
		}
		if(*mi != *ii) return false;
		++mi;
		++ii;
	}
	return ii == ie;
}

bool wildmatch(const str& mask, const str& id)
{
	return wildmatch(mask.begin(), mask.end(), id.begin(), id.end());
}

static void prompt(std::ostream& os, size_t delay)
{
	std::this_thread::sleep_for(std::chrono::seconds(delay));
	os << "> " << std::flush;
}

void IrcBot::exec(const std::string& cmd, std::ostream* os)
{
	std::string line = cmd;

	if(line[0] == '/')
	{
		str cmd;
		std::istringstream iss(line);
		iss >> cmd >> std::ws;
		cmd = lowercase(cmd);
		if(cmd == "/say")
		{
			str channel;
			iss >> channel >> std::ws;
			if(!trim(channel).empty() && channel[0] == '#')
			{
				std::getline(iss, line);
				irc.say(channel, line);
				if(os) (*os) << "OK";
			}
			else if(os) (*os) << "ERROR: /say #channel then some dialogue.\n";
		}
		else if(cmd == "/raw")
		{
			str raw;
			if(std::getline(iss, raw))
				irc.send(raw);
		}
		else if(cmd == "/nick")
		{
			str nick;
			if(iss >> nick)
				irc.nick(nick);
		}
		else if(cmd == "/die")
		{
			for(const str& c: chans)
			{
				irc.say(c, get(PROP_GOODBYE));
				irc.part(c);
			}
			done = true;
		}
		else if(cmd == "/reconfigure")
		{
			if(!(std::ifstream(configfile) >> (*this)))
				log("Error reading config file.");
			config_loaded = std::time(0);
		}
		else if(cmd == "/reload")
		{
			load_plugins(*this);
			// Initialise plugins
			plugin_vec_iter p = plugins.begin();
			while(p != plugins.end())
			{
				if(!(*p))
				{
					log("Null plugin found during reinitialisation.");
					p = plugins.erase(p);
					continue;
				}
				if(!(*p)->init())
				{
					log("Plugin failed during reinitialisation.");
					p = plugins.erase(p);
					continue;
				}
				log("\tPlugin initialised: " << (*p)->get_name() << " v" << (*p)->get_version());
				for(str& c: (*p)->list())
				{
					log("\t\tRegister command: " << c);
					commands[c] = *p;
				}
				++p;
			}
		}
		else if(cmd == "/msg")
		{
			str to, s;
			iss >> to >> std::ws;
			if(std::getline(iss, s)) irc.say(to, s);
		}
		else if(cmd == "/mode")
		{
			str nick, mode;
			iss >> nick >> std::ws;
			if(std::getline(iss, mode)) irc.mode(nick, mode);
		}
		else if(cmd == "/me")
		{
			str channel;
			iss >> channel >> std::ws;
			if(!trim(channel).empty() && channel[0] == '#')
			{
				std::getline(iss, line);
				irc.me(channel, line);
				if(os) (*os) << "OK";
			}
			else if(os) (*os) << "ERROR: /me #channel then some dialogue.\n";
//			str nick, mode;
//			std::getline(iss >> nick, mode);
//			if(!trim(nick).empty() && !trim(mode).empty())
//			{
//				irc.mode(nick, mode);
//				if(os) (*os) << "OK";
//			}
//			else if(os) (*os) << "ERROR: /mode <nick> <mode>.\n";
		}
		else if(cmd == "/join")
		{
			str channel;
			iss >> channel >> std::ws;
			if(!trim(channel).empty() && channel[0] == '#')
			{
				if(irc.join(channel)) chans.insert(channel);
				if(os) (*os) << "OK";
			}
			else if(os) (*os) << "ERROR: /join #channel.\n";
		}
		else if(cmd == "/part")
		{
			str channel;
			iss >> channel >> std::ws;
			if(!trim(channel).empty() && channel[0] == '#')
			{
				std::getline(iss, line);
				if(chans.erase(channel)) irc.part(channel, trim(line));
				if(os) (*os) << "OK";
			}
			else if(os) (*os) << "ERROR: /part #channel [message].\n";
		}
		else if(cmd == "/ping")
		{
			str dest;
			std::getline(iss, dest);
			if(!irc.ping(dest)) { if(os) (*os) << "OK"; }
			else if(os) (*os) << "ERROR: Ping failure.\n";
		}
		else if(cmd == "/reconnect")
		{
			if(irc.connect(host, port)) { if(os) (*os) << "OK"; }
			else if(os) (*os) << "ERROR: Unable to reconnect.\n";
		}
		else if(os) (*os) << "ERROR: Unknown command.\n";
	}
	else
		if(os) (*os) << "ERROR: Commands begin with /.\n";
}

void IrcBot::console()
{
	std::istream& is = this->is ? *this->is : std::cin;
	std::ostream& os = this->os ? *this->os : std::cout;

	log("Console started:");
	str line;
	prompt(os, 3);
	for(; !done && std::getline(is, line); prompt(os, 3))
	{
		trim(line);
		if(line.empty())
			continue;
		exec(line, &os);
	}
	log("Console ended:");
}

void IrcBot::pinger()
{
	while(!done)
	{
		const size_t retries = get<int>(PROP_SERVER_RETRIES, 10);
		size_t attempt = 0;

		log("Connecting:");

		for(attempt = 0; !done && !irc.connect(host, port) && attempt < retries; ++attempt)
		{
			log("Connection attempt: " << attempt);
			for(time_t now = time(0); !done && time(0) - now < 10;)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		if(!(attempt < retries))
		{
			log("Giving up.");
			done = true;
		}

		if(!done)
		{
			log("Connected:");
			connected = true;
			registered = false;

			irc.pass(get(PROP_SERVER_PASSWORD));

			// Attempt to connect with the first nick
			irc.nick(info.nicks[nick_number++]);
			irc.user(info.user, info.mode, info.real);

			while(!done && connected && !registered)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		while(!done && connected && registered)
		{
			std::ostringstream oss;
			oss << std::rand();

			irc.ping(oss.str());
			size_t count = 60 * 3; // 3 minutes
			while(!done && --count)
				std::this_thread::sleep_for(std::chrono::seconds(1));

			pinger_info_mtx.lock();
			connected = pinger_info == oss.str();
			pinger_info_mtx.unlock();

			if(!connected)
			{
				log("PING TIMEOUT: Attempting reconnect.");
				registered = false;
				nick_number = 0;
			}

			// each ping try to regain primary nick if
			// not already set.
			if(info.nicks.size() > 1 && nick != info.nicks[0])
			{
				nick_number = 0;
				irc.nick(info.nicks[nick_number++]);
			}
		}
	}
}

bool IrcBot::extract_params(const message& msg, std::initializer_list<str*> args, bool report)
{
	std::istringstream iss(msg.get_user_params());
	for(str* arg: args)
		if(!(ios::getstring(iss, *arg)))
		{
			if(report)
				fc_reply_help(msg, help(msg.get_user_cmd()), "04" + msg.get_user_cmd() + ": 01");
			return false;
		}
	return true;
}

RandomTimer::RandomTimer(std::function<void(const void*)> cb)
: mindelay(1)
, maxdelay(60)
, cb(cb)
{
}

RandomTimer::RandomTimer(const RandomTimer& rt)
: mindelay(rt.mindelay)
, maxdelay(rt.maxdelay)
, cb(rt.cb)
{
}

RandomTimer::~RandomTimer()
{
	turn_off();
}

void RandomTimer::timer()
{
	bug_func();
	while(!users.empty())
	{
		try
		{
			for(const void* user: users)
				cb(user);

			std::time_t end = std::time(0) + rand_int(mindelay, maxdelay);
			//bug("NEXT QUOTE: " << ctime(&end));
			while(!users.empty() && std::time(0) < end)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		catch(std::exception& e)
		{
			log("e.what(): " << e.what());
		}
	}
}

bool RandomTimer::turn_off()
{
	bug_func();
	if(users.empty())
		return false;
	{
		lock_guard lock(mtx);
		if(users.empty())
			return false;
		users.clear();
	}
	if(fut.valid())
		fut.get();

	return true;
}

void RandomTimer::set_mindelay(size_t seconds) { mindelay = seconds; }
void RandomTimer::set_maxdelay(size_t seconds) { maxdelay = seconds; }

bool RandomTimer::on(const void* user)
{
	bug_func();
	if(users.count(user))
		return false;

	lock_guard lock(mtx);
	if(users.count(user))
		return false;

	users.insert(user);
	if(users.size() == 1)
		fut = std::async(std::launch::async, [&]{ timer(); });

	return true;
}

bool RandomTimer::off(const void* user)
{
	bug_func();
	if(!users.count(user))
		return false;

	bool end = false;
	{
		lock_guard lock(mtx);
		if(!users.count(user))
			return false;

		users.erase(user);
		if(users.empty())
			end = true;
	}
	if(end)
		fut.get();

	return true;
}

MessageTimer::MessageTimer(std::function<void(const message&)> cb)
: mindelay(1)
, maxdelay(60)
, cb(cb)
{
}

MessageTimer::MessageTimer(const MessageTimer& rt)
: mindelay(rt.mindelay)
, maxdelay(rt.maxdelay)
, cb(rt.cb)
{
}

MessageTimer::~MessageTimer()
{
	turn_off();
}

void MessageTimer::timer()
{
	while(!messages.empty())
	{
		for(const message& m: messages)
			cb(m);

		std::time_t end = std::time(0) + rand_int(mindelay, maxdelay);
		bug("NEXT CALL: " << ctime(&end));
		while(!messages.empty() && std::time(0) < end)
			std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

bool MessageTimer::turn_off()
{
	if(messages.empty())
		return false;
	{
		lock_guard lock(mtx);
		if(messages.empty())
			return false;
		messages.clear();
	}
	if(fut.valid())
		fut.get();

	return true;
}

void MessageTimer::set_mindelay(size_t seconds) { mindelay = seconds; }
void MessageTimer::set_maxdelay(size_t seconds) { maxdelay = seconds; }

bool MessageTimer::on(const message& m)
{
	if(messages.count(m))
		return false;

	lock_guard lock(mtx);
	if(messages.count(m))
		return false;

	messages.insert(m);
	if(messages.size() == 1)
		fut = std::async(std::launch::async, [&]{ timer(); });

	return true;
}

bool MessageTimer::off(const message& m)
{
	if(!messages.count(m))
		return false;

	bool end = false;
	{
		lock_guard lock(mtx);
		if(!messages.count(m))
			return false;

		messages.erase(m);
		if(messages.empty())
			end = true;
	}
	if(end)
		fut.get();

	return true;
}

//FloodController::FloodController()
//: dispatching(false)
//, priority(0)
//, max(100)
//{
//}
//
//FloodController::~FloodController() {}
//
//size_t FloodController::get_priority(const std::thread::id& id)
//{
//	lock_guard lock(mtx);
////	std::thread::id id = std::this_thread::get_id();
//	if(mt_que[id].empty())
//		return (mt_priority[id] = 0);
//	return ++mt_priority[id];
//}
//
//void FloodController::dispatcher()
//{
//	while(dispatching)
//	{
//		std::this_thread::sleep_for(std::chrono::seconds(2));
//		if(mt_que.empty())
//			continue;
//		lock_guard lock(mtx);
//		if(!(++robin < mt_que.size()))
//			robin = 0;
//
//		mtpq_map_iter i = mt_que.begin();
//		std::advance(i, robin);
//		mtpq_map_iter end = i;
//
//		while(i != mt_que.end() && i->second.empty())
//		{
//			++i;
//		}
//
//		if(i == mt_que.end() || i->second.empty())
//			continue;
//
//		i->second.top().func();
//		i->second.pop();
//		robin = std::distance(mt_que.begin(), i);
//		bug("D: robin:" << robin);
//	}
//
//	for(mtpq_pair& p: mt_que)
//		while(!p.second.empty())
//		{
//			std::this_thread::sleep_for(std::chrono::seconds(2));
//			p.second.top().func();
//			p.second.pop();
//		}
//
//	log("Dispatcher ended.");
//}
//
//bool FloodController::send(std::function<bool()> func)
//{
////	bug_func();
//	std::thread::id id = std::this_thread::get_id();
////	bug("id: " << id);
//	if(!(mt_que[id].size() < max))
//		return false;
//
//	const size_t priority = get_priority(id);
////	bug("priority: " << priority);
//	lock_guard lock(mtx);
////	bug("pushing item in que: " << id);
//	mt_que[id].push({priority, func});
//	//mt_que_iter = mt_que.end();
//	return true;
//}
//
//void FloodController::start()
//{
//	log("Starting up dispatcher.");
//	if(!dispatching)
//	{
//		dispatching = true;
//		//mt_que_iter = mt_que.end();
//		fut = std::async(std::launch::async, [&]{ dispatcher(); });
//	}
//}
//
//void FloodController::stop()
//{
//	log("Closing down dispatcher.");
//	if(!dispatching)
//		return;
//	dispatching = false;
//	if(fut.valid())
//		fut.get();
//}
//
//// FC 2
//
//FloodController2::FloodController2()
////: dispatching(false)
//{
//}
//
//FloodController2::~FloodController2() {}
//
//bool FloodController2::find_next(str& c)
//{
//	if(m.empty())
//		return false;
//
//	const dispatch_map::iterator j = m.find(c);
//	dispatch_map::iterator i;
//
//	for(i = j; i != m.end(); ++i)
//	{
//		if(!i->second.empty())
//		{
//			c = i->first;
//			return true;
//		}
//	}
//
//	for(i = m.begin(); i != j; ++i)
//	{
//		if(!i->second.empty())
//		{
//			c = i->first;
//			return true;
//		}
//	}
//
//	return false;
//}
//
//void FloodController2::dispatcher()
//{
//	bug_func();
//	while(dispatching)
//	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(200));
//		{
//			lock_guard lock(mtx);
//			if(!find_next(c))
//				continue;
//			m[c].front()();
//			m[c].pop();
//		}
//		std::this_thread::sleep_for(std::chrono::milliseconds(1200));
//	}
//
//	log("Dispatcher ended.");
//}
//
//bool FloodController2::send(const str& channel, std::function<bool()> func)
//{
//	lock_guard lock(mtx);
//	//s.insert(channel);
//	m[channel].push(func);
//	return true;
//}
//
//void FloodController2::start()
//{
//	log("Starting up dispatcher.");
//	if(!dispatching)
//	{
//		dispatching = true;
//		fut = std::async(std::launch::async, [&]{ dispatcher(); });
//	}
//}
//
//void FloodController2::stop()
//{
//	log("Closing down dispatcher.");
//	if(!dispatching)
//		return;
//	dispatching = false;
//	if(fut.valid())
//		fut.get();
//}

}} // sookee::ircbot
