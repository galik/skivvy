/*
 * ircbot.cpp
 *
 *  Created on: 29 Jul 2011
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

#include <skivvy/ios.h>
#include <skivvy/stl.h>
#include <skivvy/str.h>
#include <skivvy/logrep.h>
#include <skivvy/irc-constants.h>

#include <random>
#include <functional>

#include <dirent.h>
#include <regex.h>
#include <pcrecpp.h>

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

	// :SooKee!~SooKee@SooKee.users.quakenet.org MODE #skivvy-admin +o Zim-blackberry

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
	os << "msg.get_nick()       : " << m.get_nick() << '\n';
	os << "msg.get_user()       : " << m.get_user() << '\n';
	os << "msg.get_host()       : " << m.get_host() << '\n';
	os << "msg.get_userhost()   : " << m.get_userhost() << '\n';
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

// MyNick!~User@server.com

str message::get_nick() const
{
//	bug_func();
	return from.substr(0, from.find("!"));
}

str message::get_user() const
{
//	bug_func();
	return from.substr(from.find("!") + 1, from.find("@") - from.find("!") - 1);
}

str message::get_host() const
{
//	bug_func();
	return from.substr(from.find("@") + 1);
}

str message::get_userhost() const
{
//	bug_func();
	return from.substr(from.find("!") + 1);
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
	return from_channel() ? to : get_nick();
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

bool RemoteIrcServer::join(const str& channel, const str& key)
{
	return send(JOIN + " " + channel + " " + key);
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

bool RemoteIrcServer::whois(const str& n)
{
	return send(WHOIS + " " + n);
}

bool RemoteIrcServer::quit(const str& reason)
{
	return send(QUIT + " :" + reason);
}

// :SooKee!~SooKee@SooKee.users.quakenet.org KICK #skivvy Skivvy :Skivvy

// Parameters: <channel> *( "," <channel> ) <user> *( "," <user> ) [<comment>]
// KICK KICK #skivvy Skivvy :
bool RemoteIrcServer::kick(const str_vec& chans, const str_vec&users, const str& comment)
{
	str cmd = KICK;

	str sep = " ";
	for(const str& s: chans)
		{ cmd += sep + s; sep = ","; }

	sep = " ";
	for(const str& s: users)
		{ cmd += sep + s; sep = ","; }

	return send(cmd + " :" + comment);
}

// non standard??
bool RemoteIrcServer::auth(const str& user, const str& pass)
{
	return send(AUTH + " " + user + " " + pass);
}

bool RemoteIrcServer::me(const str& to, const str& text)
{
	return say(to, "\001ACTION " + text + "\001");
}

bool RemoteIrcServer::query(const str& nick)
{
	return send(QUERY + " " + nick);
}

// MODE #skivvy-admin +o Zim-blackberry

bool RemoteIrcServer::mode(const str& chan, const str& mode, const str& nick)
{
	return send(MODE + " " + chan + " " + mode + " " + nick);
}

// correct???
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
	return say(msg.get_nick(), text);
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

IrcBotPluginPtr IrcBotPluginLoader::operator()(const str& file, IrcBot& bot)
{
	IrcBotPluginPtr plugin;
//	union { void* dl; void* dlsym; IrcBotPluginPtr(*plugin)(IrcBot&); } ptr;
	void* dl = 0;
//	void* skivvy_ircbot_factory = 0;
	IrcBotPluginPtr(*skivvy_ircbot_factory)(IrcBot&) = 0;

	log("PLUGIN LOAD: " << file);
	if(!(dl = dlopen(file.c_str(), RTLD_NOW|RTLD_GLOBAL)))
//	if(!(ptr.dl = dlopen(file.c_str(), RTLD_LAZY|RTLD_GLOBAL)))
	{
		log(dlerror());
		return plugin;
	}

	if(!(skivvy_ircbot_factory = reinterpret_cast<IrcBotPluginPtr(*)(IrcBot&)>(dlsym(dl, "skivvy_ircbot_factory"))))
	{
		log(dlerror());
		return plugin;
	}

	if(!(plugin = skivvy_ircbot_factory(bot)))
		return plugin;
	plugin->dl = dl;
	// ("#channel"|"*"|"PM") -> { "plugin#1", "plugin#2" }
	bot.del_plugin(plugin->get_name());
	bot.add_plugin(plugin);
	return plugin;
}

void IrcBot::add_plugin(IrcBotPluginPtr plugin)
{
	bug_func();
	if(plugin)
	{
		this->plugins.push_back(plugin);
		// give every plugin 'free pass' access to channels
		//chan_access["*"].insert(plugin->get_name());
	}
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

void IrcBot::del_plugin(const str& name)
{
	bug_func();
//	IrcBotPlugin* pp;
	for(plugin_vec_iter p =  plugins.begin(); p != plugins.end();)
	{
		if((*p)->get_name() == name)
		{
//			(*pp)->
			lock_guard lock(plugin_mtx);
			if(monitors.erase(dynamic_cast<IrcBotMonitor*>(p->get())))
				log("Unregistering monitor: " << (*p)->get_name());

			bug("exiting plugin: " << (*p)->get_name());
			(*p)->exit();
			bug("dlclose plugin: " << (*p)->get_name());
//			dlclose(p->get()); // TODO add this again
			dlclose((*p)->dl); // TODO add this again
			log("Unregistering plugin : " << (*p)->get_name());
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
	siz pos;
	str line;
	//bot.props.clear();

	while(std::getline(is, line))
	{
		if(line.empty() || line[0] == '#')
			continue;

		if((pos = line.find("//")) != str::npos)
			line.erase(pos);
		trim(line);

		bug_var(line);

		str key, val;
		std::istringstream iss(line);
		if(!std::getline(std::getline(iss >> std::ws, key, ':') >> std::ws, val))
		{
			log("Error parsing prop: " << line);
			continue;
		}

		trim(key);
		trim(val);

		if(key == "nick") bot.info.nicks.push_back(val);
		else if(key == "user") bot.info.user = val;
		else if(key == "mode") bot.info.mode = val;
		else if(key == "real") bot.info.real = val;
		else if(key == "join") bot.add_channel(val);
		else if(key == "ban") bot.banned.insert(val);
		else if(key == "include")
		{
			bug("include:" << val);
			str file_name;
			if(val.empty())
				continue;
			if(val[0] == '/')
				file_name = val;
			else
			{
				str file_path = bot.configfile.substr(0, bot.configfile.find_last_of('/'));
				bug_var(file_path);
				file_name = file_path + "/" + val;
				bug_var(file_name);
			}
			bug_var(file_name);
			std::ifstream ifs(file_name);
			if(!(ifs >> bot))
				log("Failed to include: " << file_name);
		}
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
	return fc.send(msg.to, [&,msg,text]()->bool{ return irc.reply(msg, text); });
}

bool IrcBot::fc_reply_help(const message& msg, const str& text, const str& prefix)
{
	const str_vec v = split(text, '\n');
	for(const str& s: v)
		if(!fc.send(msg.to, [&,msg,prefix,s]()->bool{ return irc.reply(msg, prefix + s); }))
			return false;
	return true;
}

bool IrcBot::fc_reply_pm(const message& msg, const str& text)//, size_t priority)
{
	return fc.send(msg.to, [&,msg,text]()->bool{ return irc.reply_pm(msg, text); });
}

bool IrcBot::fc_reply_pm_help(const message& msg, const str& text, const str& prefix)
{
	const str_vec v = split(text, '\n');
	for(const str& s: v)
		if(!fc.send(msg.to, [&,msg,prefix,s]()->bool{ return irc.reply_pm(msg, prefix + s); }))
			return false;
	return true;
}

bool IrcBot::cmd_error(const message& msg, const str& text, bool rv)
{
	fc_reply(msg, msg.get_user_cmd() + ": " + text);
	return rv;
}

bool IrcBot::cmd_error_pm(const message& msg, const str& text, bool rv)
{
	fc_reply_pm(msg, msg.get_user_cmd() + ": " + text);
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

void IrcBot::load_plugins()
{
	bug_func();
	plugin_loaded = std::time(0);

	IrcBotPluginLoader load;

	str_vec plugin_dirs;

	if(have("plugin_dir"))
	{
		str_vec dirs = get_vec("plugin_dir");
		plugin_dirs.insert(plugin_dirs.end(), dirs.begin(), dirs.end());
	}
	plugin_dirs.push_back(DEFAULT_PLUGIN_DIR);

	str_vec pv = get_vec("plugin");
	str_set loaded;

	// We need to try exhaustively to load each plugin in turn
	// in order to respect dependencies.

	for(const str& line: pv)
	{
		bug_var(line);
		str p, access;
		sgl(sgl(siss(line), p, '['), access, ']');
		trim(p);
		bug_var(p);
		bug_var(access);
		for(const str& plugin_dir: plugin_dirs)
		{
			log("Searching plugin dir: " << plugin_dir);

			str_vec files;
			if(int e = ls(plugin_dir, files))
			{
				log(strerror(e));
				continue;
			}

			if(stl::find(files, p) == files.end())
				continue;

			IrcBotPluginPtr plugin;
			if(!(plugin = load(plugin_dir + "/" + p, *this)))
			{
				log("Failed to load: " << p);
				continue;
			}

			// // ("#channel"|"*"|"PM") -> {{ {"plugin#1", "prefix1"}, {"plugin#2", "prefix2"} }
			// access = [#skivvy,#clanchan(sk)]
			str chan;
			siss iss(access);
			while(sgl(iss, chan, ','))
			{
				trim(chan);
				bug_var(chan);
				str skip, prefix;
				sgl(sgl(siss(chan), skip, '('), prefix, ')');
				bug_var(prefix);
				chan_prefix cpf;
				cpf.plugin = plugin->get_name();
				cpf.prefix = prefix;
				chan_access[chan].insert(cpf);
			}

			break; // only load one of the given name
		}
	}
}

void IrcBot::dispatch_msgevent(const message& msg)
{
	time_t now = std::time(0);
	lock_guard lock(msgevent_mtx);
	msgevent_range mr = msgevents.equal_range(msg.cmd);
	for(msgevent_itr mi = mr.first; mi != mr.second; mi = msgevents.erase(mi))
		if(now - mi->second.when < int(mi->second.timeout))
			mi->second.func(msg);
}

const str FLOOD_TIME_BETWEEN_POLLS = "flood.poll.time";
const siz FLOOD_TIME_BETWEEN_POLLS_DEFAULT = 300;
const str FLOOD_TIME_BETWEEN_EVENTS = "flood.event.time";
const siz FLOOD_TIME_BETWEEN_EVENTS_DEFAULT = 1600;

bool IrcBot::init(const str& config_file)
{
	std::srand(std::time(0));

	if(have(FLOOD_TIME_BETWEEN_POLLS))
		fc.set_time_between_checks(get(FLOOD_TIME_BETWEEN_POLLS, FLOOD_TIME_BETWEEN_POLLS_DEFAULT));
	if(have(FLOOD_TIME_BETWEEN_EVENTS))
		fc.set_time_between_checks(get(FLOOD_TIME_BETWEEN_EVENTS, FLOOD_TIME_BETWEEN_EVENTS_DEFAULT));

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

	load_plugins();

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
	png = std::async(std::launch::async, [&]{ pinger(); });

	while(!done && !connected)
		std::this_thread::sleep_for(std::chrono::seconds(1));

	// start console
	con = std::async(std::launch::async, [&]{ console(); });

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

		// RAW plugins ? TODO: implement as a plugin!

		// send: register
		// recv: name v0.0
		// recv: !cmd1
		// recv: !cmd2

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
			const str who = msg.get_nick();
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
			if(msg.get_nick() == nick)
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
				str cmd = lowercase(msg.get_user_cmd());
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
				else if(cmd == "!restart")
				{
					str pass;
					std::istringstream iss(msg.get_user_params());

					if(!(ios::getstring(iss, pass)))
					{
						fc_reply(msg, "!restart <password>");
						continue;
					}
					bug_var(pass);
					if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == pass)
					{
						done = true;
						restart = true;
					}
				}
				else if(cmd == "!pset")
				{
					//str_vec params = split_params(msg.get_user_params(), ' ');

					str pass;
					std::istringstream iss(msg.get_user_params());

					if(!(ios::getstring(iss, pass)))
					{
						fc_reply(msg, "!pset <password> <key>: [<val1> <val2> ...]");
						continue;
					}
					bug_var(pass);
					// !pset <password> var: val1, val2, val3
					if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == pass)
					{
						str key, val;

						if(!std::getline(iss >> std::ws, key, ':') || trim(key).empty())
						{
							fc_reply(msg, "Expected <key>: [<val1> <val2> ...]");
							continue;
						}

						bug_var(key);

//						if(!key.empty() && key.top() == "]")

						str_vec vals;

						while(ios::getstring(iss, val))
						{
							bug_var(val);
							vals.push_back(val);
						}
						if(vals.empty())
						{
							// report current values
							siz i = 0;
							for(const str& val: props[key])
								fc_reply(msg, key + "[" + std::to_string(i++) + "]: " + val);
							continue;
						}

						props.erase(key);
						for(str val: vals)
							props[key].push_back(trim(val));
						fc_reply(msg, "Property " + key + " has been set.");
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
					const str sender = msg.get_nick();
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
					if(banned.find(msg.get_nick()) == banned.end())
						// TODO: make this async (make plugins thread-safe)
						execute(cmd, msg);
					else log("BANNED: " << msg.get_nick());
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

bool IrcBot::has_access(const str& cmd, const str& chan)
{
	std::function<bool(const chan_prefix&)> chan_prefix_pred_eq = [&](const chan_prefix& cp)
	{
		return cp.plugin == commands[cmd]->get_name();
	};

	if(stl::find_if(chan_access[chan], chan_prefix_pred_eq) != chan_access[chan].end())
		return true;
	return false;
}

bool IrcBot::allow_cmd_access(const str& cmd, const message& msg)
{
	// "*" -> free pass list
	// "PM" -> PM list

	std::function<bool(const chan_prefix&)> chan_prefix_pred_eq = [&](const chan_prefix& cp)
	{
		return cp.plugin == commands[cmd]->get_name();
	};

	if(has_access(cmd, "*"))
		return true;

	if(msg.from_channel())
		if(has_access(cmd, msg.to))
			return true;

	if(has_access(cmd, "PM"))
		return true;

	return false;
}

void IrcBot::execute(const str& cmd, const message& msg)
{
	bug_func();
	bug_var(cmd);
	bug_msg(msg);
	if(commands.find(cmd) == commands.end())
	{
		log("IrcBot::execute(): Unknown command: " << cmd);
		return;
	}
	if(commands[cmd])
	{
		if(allow_cmd_access(cmd, msg))
			commands[cmd]->execute(cmd, msg);
		else if(msg.from_channel())
			log("PLUGIN " << commands[cmd]->get_name()
				<< " NOT AUTHORISED FOR CHANNEL: " << msg.to);
		else
			log("PLUGIN " << commands[cmd]->get_name()
				<< " NOT AUTHORISED FOR PM TO: " << msg.get_nick());
	}
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
		if(png.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			png.get();

	log("Closing down console:");
	if(con.valid())
		if(con.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			con.get();

	irc.quit(restart ? "Reeeeeeeeeeeeeeeeeeboot!" : "Going off line...");
	log("ENDED");
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
		else if(cmd == "/botnick")
		{
			if(os)
				(*os) << nick << std::flush;
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
		else if(cmd == "/restart")
		{
			irc.quit("Reeeebooooot!");
			restart = true;
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
			load_plugins();
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
			if(std::getline(iss, s))
				irc.say(to, s);
		}
		else if(cmd == "/auth")
		{
			// TODO: FIXME
			str user, pass;
			if(iss >> user >> pass)
				irc.auth(user, pass);
		}
		else if(cmd == "/mode")
		{
			str nick, mode;
			iss >> nick >> std::ws;
			if(std::getline(iss, mode))
				irc.mode(nick, mode);
		}
		else if(cmd == "/whois")
		{
			str nick;
			iss >> nick;
			irc.whois(nick);
		}
		else if(cmd == "/me")
		{
			str channel;
			iss >> channel >> std::ws;
			if(!trim(channel).empty() && channel[0] == '#')
			{
				std::getline(iss, line);
				irc.me(channel, line);
				if(os)
					(*os) << "OK";
			}
			else if(os)
				(*os) << "ERROR: /me #channel then some dialogue.\n";
		}
		else if(cmd == "/join")
		{
			str channel, key;
			iss >> channel >> key >> std::ws;
			if(!trim(channel).empty() && channel[0] == '#')
			{
				if(irc.join(channel, key)) chans.insert(channel);
				if(os)
					(*os) << "OK";
			}
			else if(os)
				(*os) << "ERROR: /join #channel.\n";
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
			else if(os)
				(*os) << "ERROR: /part #channel [message].\n";
		}
		else if(cmd == "/ping")
		{
			str dest;
			std::getline(iss, dest);
			if(!irc.ping(dest)) { if(os) (*os) << "OK"; }
			else if(os)
				(*os) << "ERROR: Ping failure.\n";
		}
		else if(cmd == "/reconnect")
		{
			if(irc.connect(host, port)) { if(os) (*os) << "OK"; }
			else if(os)
				(*os) << "ERROR: Unable to reconnect.\n";
		}
		else if(os)
			(*os) << "ERROR: Unknown command.\n";
	}
	else
		if(os)
			(*os) << "ERROR: Commands begin with /.\n";
}

str get_regerror(int errcode, regex_t *compiled)
{
	size_t length = regerror(errcode, compiled, NULL, 0);
	char *buffer = new char[length];
	(void) regerror(errcode, compiled, buffer, length);
	str e(buffer);
	delete[] buffer;
	return e;
}

bool IrcBot::ereg_match(const str& r, const str& s)
{
	bug_func();
	bug_var(s);
	bug_var(r);
	regex_t regex;

	if(regcomp(&regex, r.c_str(), REG_EXTENDED | REG_ICASE))
	{
		log("Could not compile regex: " << r);
		return false;
	}

	int reti = regexec(&regex, s.c_str(), 0, NULL, 0);
	regfree(&regex);
	if(!reti)
		return true;
	else if(reti != REG_NOMATCH)
		log("regex: " << get_regerror(reti, &regex));

	return false;
//	return lowercase(s).find(lowercase(r)) != str::npos;
}

bool IrcBot::preg_match(const str& r, const str& s, bool full)
{
//	bug_func();
//	bug_var(s);
//	bug_var(r);
//	bug_var(full);

	if(full)
		return pcrecpp::RE(r).FullMatch(s);
	return pcrecpp::RE(r).PartialMatch(s);
}

bool IrcBot::wild_match(const str& w, const str& s, int flags)
{
	return !fnmatch(w.c_str(), s.c_str(), flags | FNM_EXTMATCH);
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
				std::this_thread::sleep_for(std::chrono::seconds(3));
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
		if(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			fut.get();
//	if(fut.valid())
//		fut.get();

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

	{
		lock_guard lock(mtx);
		if(!users.count(user))
			return false;

		users.erase(user);
		if(!users.empty())
			return true;
	}
	if(fut.valid())
		if(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			fut.get();
//	if(end)
//		fut.get();

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

}} // skivvy::ircbot
