/*
 * ircbot.cpp
 *
 *  Created on: 29 Jul 2011
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

#include <random>
#include <functional>

#include <dirent.h>

#include <hol/small_types.h>
#include <hol/bug.h>
#include <hol/simple_logger.h>
#include <hol/string_utils.h>

#include <skivvy/ios.h>
#include <skivvy/stl.h>
#include <skivvy/logrep.h>
#include <skivvy/utils.h>
#include <skivvy/irc-constants.h>
#include <skivvy/message.h>

#include <skivvy/IrcServer.h>
//#include <skivvy/socketstream.h>
//#include <skivvy/ssl_socketstream.h>
#include <regex>

namespace skivvy { namespace ircbot {

using clock = std::chrono::system_clock;

PLUGIN_INFO("skivvy", "IrcBot", "0.5.1");

using namespace skivvy;
using namespace skivvy::irc;
using namespace skivvy::utils;

using namespace hol::simple_logger;
using namespace hol::small_types::basic;
using namespace hol::small_types::string_containers;

static const str SERVER_HOST = "server.host";
static const str SERVER_HOST_DEFAULT = "localhost";
static const str SERVER_PORT = "server.port";
static const siz SERVER_PORT_DEFAULT = 6667;

static const str PROP_PASSWORD = "password";
static const str PROP_WELCOME = "welcome";
static const str PROP_GOODBYE = "goodbye";
static const str PROP_ON_CONNECT = "on_connect";
static const str PROP_SERVER_PASSWORD = "server.password";
static const str PROP_SERVER_RETRIES = "server.retries";
static const str PROP_JOIN = "join";
static const str PROP_JOIN_DELAY = "join.delay";
static const siz PROP_JOIN_DELAY_DEFAULT = 10; // seconds

static const str IRCBOT_STORE_FILE = "store.file";
static const str IRCBOT_STORE_FILE_DEFAULT = "store.txt";

static const str FLOOD_TIME_BETWEEN_POLLS = "flood.poll.time";
static const siz FLOOD_TIME_BETWEEN_POLLS_DEFAULT = 300;
static const str FLOOD_TIME_BETWEEN_EVENTS = "flood.event.time";
static const siz FLOOD_TIME_BETWEEN_EVENTS_DEFAULT = 1600;

//// TODO: move these to IrcBot class def
//std::mutex status_mtx;
//str_vec status;

BasicIrcBotPlugin::BasicIrcBotPlugin(IrcBot& bot)
: bot(bot), irc(0)
{
}

BasicIrcBotPlugin::~BasicIrcBotPlugin() {}

void BasicIrcBotPlugin::add(const action& a)
{
	actions.insert(action_pair{a.cmd, a});
}

bool BasicIrcBotPlugin::init()
{
	if(!(irc = bot.get_irc_server()))
		return false;
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
	bug_fun();
	bug_var(cmd);
	if(!actions.count(cmd))
	{
		LOG::E << "unknown command: " << cmd;
		return "unknown command";
	}
	if(actions.at(cmd).flags & action::INVISIBLE)
		return "none";
	bug_var(actions.at(cmd).help);
	return actions.at(cmd).help;
}

// =======================================================================

IrcBotPluginRPtr IrcBotPluginLoader::operator()(const str& file, IrcBot& bot)
{
	void* dl = 0;
	IrcBotPluginRPtr(*skivvy_ircbot_factory)(IrcBot&) = 0;

	LOG::I << "PLUGIN LOAD: " << file;

	//if(!(dl = dlopen(file.c_str(), RTLD_NOW|RTLD_GLOBAL)))
	if(!(dl = dlopen(file.c_str(), RTLD_LAZY|RTLD_GLOBAL)))
	{
		LOG::E << dlerror();
		return 0;
	}

	if(!(*(void**)&skivvy_ircbot_factory = dlsym(dl, "skivvy_ircbot_factory")))
	{
		LOG::E << dlerror();
		return 0;
	}

	IrcBotPluginRPtr plugin;
	if((plugin = skivvy_ircbot_factory(bot)))
	{
		plugin->dl = dl;
		bot.del_plugin(plugin->get_id());
		bot.add_plugin(plugin);
	}
	return plugin;
}

void IrcBot::add_plugin(IrcBotPluginRPtr plugin)
{
	if(plugin)
		this->plugins.emplace_back(plugin);
	else
		LOG::E << "Adding non-plugin.";
}

bool IrcBot::has_plugin(const str& id, const str&  version)
{
	for(const auto& p: plugins)
		if(p->get_id() == id && p->get_version() >= version)
			return true;
	return false;
}

void IrcBot::del_plugin(const str& id)
{
	bug_fun();
	for(plugin_vec_iter p =  plugins.begin(); p != plugins.end();)
	{
		if((*p)->get_id() != id)
			++p;
		else
		{
			lock_guard lock(plugin_mtx);
			if(monitors.erase(dynamic_cast<IrcBotMonitor*>(p->get())))
				LOG::I << "Unregistering monitor: " << (*p)->get_name();

			str name = (*p)->get_name();
			bug("exiting plugin: " << name);
			(*p)->exit();
			bug("dlclose plugin: " << name);
			dlclose((*p)->dl);
			LOG::I << "Unregistering plugin : " << name;
			p = plugins.erase(p);
		}
	}
}

// =======================================================================
// IRCBot
// =======================================================================

IrcBot::IrcBot()
: debug(false)
, done(false)
, connected(false)
, registered(false)
, is(0), os(0)
, nick_number(0)
, uptime(std::time(0))
, config_loaded(0)
, plugin_loaded(0)
{
}

std::istream& load_props(std::istream& is, IrcBot& bot, str_map& vars, str& prefix)
{
	LOG::I << "Loading properties:";
	siz pos;
	str line;
	while(std::getline(is, line))
	{
		if((pos = line.find("//")) != str::npos)
			line.erase(pos);

		hol::trim_mute(line);

		if(line.empty() || line[0] == '#')
			continue;

		for(auto&& var: vars)
			hol::replace_all_mute(line, "${" + var.first + "}", var.second);

		// replace environment vars
		str var;
		str::size_type pos= 0;
		while((pos = extract_delimited_text(line, "${", "}", var, pos)) != str::npos)
			hol::replace_all_mute(line, "${" + var + "}", std::getenv(var.c_str()));

		// prefix processing

		if(line.front() == '[' && line.back() == ']')
		{
			prefix = line.substr(1, line.size() - 2);
			hol::trim_mute(prefix);
			continue;
		}

		// include: is special
		if(line.find("include:") && !prefix.empty())
			line = prefix + "." + line;

		// prefix processing end

//		bug("post-prop: " << line);

		if(line[0] == '$') // variable
		{
//			bug("VARIABLE DETECTED";
			str key, val;
			if(!sgl(sgl(siss(line.substr(1)) >> std::ws, key, ':') >> std::ws, val))
			{
				LOG::I << "Error parsing variable: " << line;
				continue;
			}
//			bug_var(key);
//			bug_var(val);
			vars[hol::trim_mute(key)] = hol::trim_mute(val);
			continue;
		}

		str key, val;
		if(!sgl(sgl(siss(line) >> std::ws, key, ':') >> std::ws, val))
		{
			LOG::I << "Error parsing prop: " << line;
			continue;
		}

		hol::trim_mute(key);
		hol::trim_mute(val);

		if(key == "nick")
			bot.info.nicks.push_back(val);
		else if(key == "user")
			bot.info.user = val;
		else if(key == "mode")
			bot.info.mode = val;
		else if(key == "real")
			bot.info.real = val;
		else if(key == "include")
		{
			LOG::I << "include: " << val;
			if(val.empty())
				continue;
			str file_name;
			if(val[0] == '/')
				file_name = val;
			else
			{
				str file_path;
				pos = bot.configfile.find_last_of('/');
//				bug_var(pos);
				if(pos != str::npos)
					file_path = bot.configfile.substr(0, pos);
//				bug_var(file_path);

				file_name = val;
//				bug_var(file_name);

				if(!file_path.empty())
					file_name = file_path + "/" + val;
//				bug_var(file_name);
			}

			std::ifstream ifs(file_name);
			if(!ifs)
				LOG::W << "include not found: " << file_name;
			if(!load_props(ifs, bot, vars, prefix))
				LOG::W << "failed to load include: " << file_name;
		}
		else
		{
			bot.props[key].push_back(val);
			LOG::I << '\t' << key << ": " << val;
		}
	}
	if(is.eof())
		is.clear();
	return is;
}

std::istream& operator>>(std::istream& is, IrcBot& bot)
{
	str prefix;
	str_map vars;
	return load_props(is, bot, vars, prefix);
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

IrcServer* IrcBot::get_irc_server() { return irc.get(); }

// Utility

// flood control
bool IrcBot::fc_say(const str& to, const str& text)
{
	return fc.send(to, [this,to,text]()->bool{ return irc->say(to, text); });
}

//bool IrcBot::fc_reply(const str& to, const str& text)
//{
//	message msg;
//	msg.params = " " + to + " :"; // fudge message for reply to correct channel|person
//	return fc_reply(msg, text);
//}

bool IrcBot::fc_reply_help(const message& msg, const str& text, const str& prefix)
{
	for(const str& s: hol::split_copy(text, "\n"))
		if(!fc.send(msg.get_to(), [this,msg,prefix,s]{ return irc->reply(msg, prefix + s); }))
			return false;
	return true;
}

bool IrcBot::fc_reply(const message& msg, const str& text)
{
	return fc.send(msg.get_to(), [this,msg,text]()->bool{ return irc->reply(msg, text); });
}

bool IrcBot::fc_reply_pm(const message& msg, const str& text)
{
	return fc.send(msg.get_to(), [this,msg,text]()->bool{ return irc->reply_pm(msg, text); });
}

bool IrcBot::fc_reply_notice(const message& msg, const str& text)
{
	return fc.send(msg.get_to(), [this,msg,text]()->bool{ return irc->reply_notice(msg, text); });
}

bool IrcBot::fc_reply_pm_notice(const message& msg, const str& text)
{
	return fc.send(msg.get_to(), [this,msg,text]()->bool{ return irc->reply_pm_notice(msg, text); });
}

bool IrcBot::fc_reply_pm_help(const message& msg, const str& text, const str& prefix)
{
	for(const str& s: hol::split_copy(text, "\n"))
		if(!fc.send(msg.get_to(), [this,msg,prefix,s]{ return irc->reply_pm(msg, prefix + s); }))
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

str safe_home()
{
	auto HOME = std::getenv("HOME");
	return HOME ? HOME : "";
}

str IrcBot::locate_config_dir() const
{
	return get(DATA_DIR, safe_home() + "/" + ".skivvy");
}

str IrcBot::locate_config_file(const str& name) const
{
	if(name.empty() || name[0] == '/')
		return name;
	return locate_config_dir() + "/" + name;
}

// INTERFACE: IrcBotPlugin

str IrcBot::get_id() const { return ID; }
str IrcBot::get_name() const { return NAME; }
str IrcBot::get_version() const { return VERSION; }

void IrcBot::official_join(const str& channel)
{
	std::this_thread::sleep_for(std::chrono::seconds(get(PROP_JOIN_DELAY, PROP_JOIN_DELAY_DEFAULT)));

	if(!irc->join(channel))
		return;

	chans.insert(channel);

	if(get("join.announce", false))
		irc->say(channel, get_name() + " v" + get_version());

	str_vec welcomes = get_vec(PROP_WELCOME);

	if(!welcomes.empty())
		irc->say(channel, welcomes[rand_int(0, welcomes.size() - 1)]);
}

// =====================================
// LOAD DYNAMIC PLUGINS
// =====================================

#ifndef DEFAULT_PLUGIN_DIR
#define DEFAULT_PLUGIN_DIR "/usr/lib/skivvy"
#endif

void IrcBot::load_plugins()
{
	bug_fun();
	plugin_loaded = std::time(0);

	IrcBotPluginLoader load;

	str_vec plugin_dirs;

	if(have("plugin.dir"))
	{
		str_vec dirs = get_vec("plugin.dir");
		plugin_dirs.insert(plugin_dirs.end(), dirs.begin(), dirs.end());
	}
	plugin_dirs.push_back(DEFAULT_PLUGIN_DIR);

	str_vec pv = get_vec("plugin");
	str_set loaded;

	// We need to try exhaustively to load each plugin in turn
	// in order to respect dependencies.

	// We can't load two plugins with the same id
	str_set ids;

	for(const str& line: pv)
	{
		//bug_var(line);
		str p, access;
		sgl(sgl(siss(line), p, '['), access, ']');
		hol::trim_mute(p);
		bug_var(p);
		bug_var(access);
		for(const str& plugin_dir: plugin_dirs)
		{
			str_vec files;
			if(int e = ios::ls(plugin_dir, files))
			{
				LOG::I << strerror(e);
				continue;
			}

			if(stl::find(files, p) == files.end())
				continue;

			IrcBotPluginRPtr plugin;
			if(!(plugin = load(plugin_dir + "/" + p, *this)))
			{
				LOG::I << "Failed to load: " << p;
				continue;
			}

			// Ensure unique id
			if(!ids.insert(plugin->get_id()).second)
			{
				LOG::E << "Duplicate plugin id: " << plugin->get_id();
				del_plugin(plugin->get_id());
				break;
			}

			// // ("#channel"|"*"|"PM") -> {{ {"plugin#1", "prefix1"}, {"plugin#2", "prefix2"} }
			// access = [#skivvy,#clanchan(sk)]
			str chan;
			siss iss(access);
			while(sgl(iss, chan, ','))
			{
				hol::trim_mute(chan);
				bug_var(chan);
				str skip, prefix;
				sgl(sgl(siss(chan), skip, '('), prefix, ')');
				hol::trim_mute(prefix);
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

	msgevent_map::iterator mei;
	if((mei = msgevents.find(msg.command)) != msgevents.end())
	{
		msgevent_lst& lst = mei->second;
		msgevent_lst::iterator li;
		for(li = lst.begin(); li != lst.end();)
		{
			if(li->done)
				li = lst.erase(li);
			else if(now - li->when > li->timeout)
				li = lst.erase(li);
			else
			{
				li->func(msg);
				++li;
			}
		}
	}
}

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
		LOG::I << "Using config file: " << (configfile = config_file);

	bug_var(configfile);

	if(!(std::ifstream(configfile) >> (*this)))
	{
		LOG::I << "Error reading config file.";
		return false;
	}

	config_loaded = std::time(0);

	// aliases


	// =====================================
	// CREATE CRITICAL RESOURCES
	// =====================================

	store = std::make_unique<BackupStore>(getf(IRCBOT_STORE_FILE, IRCBOT_STORE_FILE_DEFAULT));

	if(get("irc.test.mode", false))
	{
		str host = get("irc.test.host", "test-host");
		siz port = get("irc.test.port", 0);
		(irc = std::make_unique<TestIrcServer>())->connect(host, port);
	}
	else if(get("server.ssl", false))
	{
		LOG::I << "INFO: Using SSL";
		// irc = std::make_unique<RemoteSSLIrcServer>();
		hol_throw_runtime_error("SSL not implemented yet");
	}
	else
	{
		LOG::W << "Using INSECURE connection";
		irc = std::make_unique<RemoteIrcServer>();
	}

	load_plugins();

	if(!have(SERVER_HOST))
	{
		LOG::E << "No server configured.";
		return false;
	}

	if(info.nicks.empty())
	{
		LOG::E << "No nick set.";
		return false;
	}

	fc.start();

	LOG::I << "Initializing plugins [" << plugins.size() << "]:";

	plugin_vec_iter p = plugins.begin();
	while(p != plugins.end())
	{
		if(!(*p))
		{
			LOG::I << "Null plugin found during initialisation.";
			p = plugins.erase(p);
			continue;
		}
		if(!(*p)->init())
		{
			LOG::I << "Plugin failed during initialization.";
			p = plugins.erase(p);
			continue;
		}

		LOG::I << "\tPlugin initialized: " << (*p)->get_id() << ": " << (*p)->get_name() << " v" << (*p)->get_version();

		for(str& c: (*p)->list())
		{
			LOG::I << "\t\tRegister command: " << c;
			commands[c] = p->get();
		}
		++p;
	}

	if(get<bool>("irc.test.mode") == true)
		connected = true;
	else
	{
		LOG::I << "Starting pinger:";
		png = std::async(std::launch::async, [this]{ pinger(); });
	}

	LOG::I << "Awaiting connection: ";

	while(!done && !connected)
		std::this_thread::sleep_for(std::chrono::seconds(1));

	LOG::I << "Starting main loop: ";

	while(!done)
	{
		str line;
		message msg;

		if(!irc->receive(line))
		{
			bug("failed to receive");
			irc->close();
			if(get<bool>("irc.test.mode") == true)
			{
				done = true;
				continue;
			}
			// signal pinger to reconnect
			connected = false;
			registered = false;

			// wait for pinger to reconnect
			// If pinger times/retries out done == true
			while(!done && !connected)
				std::this_thread::sleep_for(std::chrono::seconds(3));
		}

		if(done)
			break;

		if(hol::trim_mute(line).empty())
			continue;

		msg.clear();
		if(!parsemsg(line, msg))
			continue;

		dispatch_msgevent(msg);

		for(IrcBotMonitor* m: monitors)
		{
			assert(m);
			m->event(msg);
		}

		// LOGGING

		static const str_vec nologs =
		{
			"PING", "PONG"//, "372"
		};

		if(stl::find(nologs, msg.command) == nologs.cend())
			LOG::I << "recv: " << line;

		if(msg.command == "END_OF_TEST")
		{
			done = true;
		}
		else if(msg.command == PING)
		{
			irc->pong(msg.get_trailing());
		}
		else if(msg.command == PONG)
		{
			lock_guard lock(pinger_info_mtx);
			pinger_info = msg.get_trailing();
		}
		else if(msg.command == RPL_WELCOME)
		{
			//BUG_MSG(msg, RPL_WELCOME);
			this->nick = msg.get_to();

			for(str prop: get_vec(PROP_ON_CONNECT))
				exec(hol::replace_all_mute(prop, "$me", nick));
			for(const str& chan: get_vec(PROP_JOIN))
				official_join(chan);
			for(const str& chan: store->get_set("invite"))
				official_join(chan);
			registered = true;
		}
		else if(msg.command == RPL_NAMREPLY)
		{
			str channel, nick;

			str_vec params = msg.get_params();
			if(params.size() > 2)
				channel = params[2];

			siss iss(msg.get_trailing());
			while(iss >> nick)
				if(!nick.empty())
					nicks[channel].insert((nick[0] == '@' || nick[0] == '+') ? nick.substr(1) : nick);
		}
		else if(msg.command == ERR_NOORIGIN)
		{
		}
		else if(msg.command == ERR_NICKNAMEINUSE)
		{
			if(nick_number < info.nicks.size())
			{
				irc->nick(info.nicks[nick_number++]);
				irc->user(info.user, info.mode, info.real);
			}
			else
			{
				LOG::E << "Unable to connect with any of the nicks.";
				done = true;
			}
		}
		else if(msg.command == RPL_WHOISCHANNELS)
		{
		}
		else if(msg.command == INVITE)
		{
			str who, chan;
			str_vec params = msg.get_params();

			if(params.size() > 0)
				who = params[0];
			if(params.size() > 1)
				chan = params[1];

			str_vec parts = get_vec("part");

			if(std::find(parts.begin(), parts.end(), chan) != parts.end())
				continue;

			if(who == nick)
			{
				official_join(chan);
				irc->say(chan, "I was invited by " + msg.get_nickname());
				if(!store->get_set("invite").count(chan))
					store->add("invite", chan);
			}
		}
		else if(msg.command == JOIN)
		{
			// track known nicks
			const str who = msg.get_nickname();
			str_vec params = msg.get_params();
			if(!params.empty())
			{
				// Am I joining a part channel?
				str_vec parts = get_vec("part");
				if(who == nick && std::find(parts.begin(), parts.end(), params[0]) != parts.end())
				{
					irc->part(params[0], "Not authorized for this channel.");
					continue;
				}

				str_set& known = nicks[params[0]];
				if(who != nick && known.find(who) == known.end())
					known.insert(who);
			}
		}
		else if(msg.command == PART)
		{
		}
		else if(msg.command == KICK)
		{
		}
		else if(msg.command == NICK)
		{
			// Is the bot changing its own nick?
			if(msg.get_nickname() == nick)
				nick = msg.get_trailing();
		}
		else if(msg.command == PRIVMSG)
		{
			str cmd = hol::lower_copy(msg.get_user_cmd());
			LOG::I << "Processing command: " << cmd;

			if(get_set("trigger.word.die", "!die").count(cmd))
			{
				if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == msg.get_user_params())
				{
					str_vec goodbyes = get_vec(PROP_GOODBYE);

					for(const str& c: chans)
					{
						if(!goodbyes.empty())
							irc->say(c, goodbyes[rand_int(0, goodbyes.size() - 1)]);
						irc->part(c);
					}
					done = true;
				}
				else
				{
					fc_reply(msg, "Incorrect password.");
				}
			}
			else if(get_set("trigger.word.uptime", "!uptime").count(cmd))//"!uptime")
			{
				soss oss;
				print_duration(clock::now() - clock::from_time_t(uptime), oss);
				str time = oss.str();
				hol::trim_mute(time);
				fc_reply(msg, "I have been active for " + time);
			}
			else if(get_set("trigger.word.join", "!join").count(cmd))//"!join")
			{
				str_vec param = hol::split_copy(msg.get_user_params());

				if(param.size() == 2)
				{
					if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == param[1])
					{
						if(irc->join(param[0]))
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
			else if(get_set("trigger.word.part", "!part").count(cmd))//"!part")
			{
				str_vec param = hol::split_copy(msg.get_user_params());

				if(param.size() == 2)
				{
					if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == param[1])
					{
						if(irc->part(param[0]))
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
			else if(get_set("trigger.word.reconfigure", "!reconfigure").count(cmd))
			{
				str pass;
				if(!(ios::getstring(siss(msg.get_user_params()), pass)))
				{
					fc_reply(msg, "!reconfigure <password>");
					continue;
				}
				if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == pass)
				{
					exec("/reconfigure");
					continue;
				}
				fc_reply(msg, "Wrong password");
			}
			else if(get_set("trigger.word.restart", "!restart").count(cmd))//"!restart")
			{
				str pass;
				std::istringstream iss(msg.get_user_params());

				if(!(ios::getstring(iss, pass)))
				{
					fc_reply(msg, "!restart <password>");
					continue;
				}
				if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == pass)
				{
					done = true;
					restart = true;
				}
			}
			else if(get_set("trigger.word.pset", "!pset").count(cmd))//"!pset")
			{
				str pass;
				std::istringstream iss(msg.get_user_params());

				if(!(ios::getstring(iss, pass)))
				{
					fc_reply(msg, "!pset <password> <key>: [<val1> <val2> ...]");
					continue;
				}

				// !pset <password> var: val1, val2, val3
				if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == pass)
				{
					str key, val;

					if(!std::getline(iss >> std::ws, key, ':') || hol::trim_mute(key).empty())
					{
						fc_reply(msg, "Expected <key>: [<val1> <val2> ...]");
						continue;
					}

					bug_var(key);
					str_vec vals;
					while(ios::getstring(iss, val))
						vals.push_back(val);

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
						props[key].push_back(hol::trim_mute(val));
					fc_reply(msg, "Property " + key + " has been set.");
				}
				else
				{
					fc_reply(msg, "Incorrect password.");
				}
			}
			else if(get_set("trigger.word.debug", "!debug").count(cmd))//"!debug")
			{
				fc_reply(msg, "Debug mode " + str(((debug = !debug)) ? "on" : "off") + ".");
			}
			else if(get_set("trigger.word.help", "!help").count(cmd))
			{
				const str sender = msg.get_nickname();
				const str params = msg.get_user_params();

				if(!params.empty())
				{
					fc_reply_pm_help(msg, help(params));
					continue;
				}

				if(msg.prefix.empty())
					continue;

				fc_reply_pm(msg, "List of commands:");

				// builtin
				fc_reply_pm(msg, "\tBuilt in");
				std::ostringstream oss;
				oss << "\t\t";
				str supersep;
				for(auto&& key: get_wild_keys("trigger.word"))
				{
					oss << supersep;
					str_set cmds = get_set(key);
					std::string sep = cmds.size() > 1 ? "[":"";
					for(auto&& cmd: cmds)
					{
						oss << sep << cmd;
						sep = ", ";
					}
					if(cmds.size() > 1)
						oss << "]";
					supersep = ", ";
				}
				fc_reply_pm(msg, oss.str());

				for(const auto& p: plugins)
				{
					fc_reply_pm(msg, "\t" + p->get_name() + " " + p->get_version());
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

				if(have("help.append"))
					fc_reply_pm(msg, "Additional Info:");

				for(str const& h: get_vec("help.append"))
					fc_reply_pm(msg, "\t" + hol::replace_all_copy(h, "$me", nick));
			}
			else
			{
				for(const str& ban: get_vec("ban"))
					if(wild_match(ban, msg.get_userhost()))
					{
						LOG::I << "BANNED: " << msg.get_nickname();
						continue;
					}
				//std::async(std::launch::async, [=]{ execute(cmd, msg); });
				execute(cmd, msg);
			}
		}
	}
	return true;
}

str_vec IrcBot::list() const
{
	str_vec v;
	for(const auto& p: plugins)
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

//	std::function<bool(const chan_prefix&)> chan_prefix_pred_eq = [&](const chan_prefix& cp)
//	{
//		return cp.plugin == commands[cmd]->get_name();
//	};

	if(has_access(cmd, "*"))
		return true;

	if(msg.from_channel())
		if(has_access(cmd, msg.get_to()))
			return true;

	if(has_access(cmd, "PM"))
		return true;

	return false;
}

void IrcBot::execute(const str& cmd, const message& msg)
{
	auto found = commands.find(cmd);
	if(found == commands.end())
	{
		LOG::I << "IrcBot::execute(): Unknown command: " << cmd;
		return;
	}

	if(auto plugin = found->second)
	{
		try
		{
			if(allow_cmd_access(cmd, msg))
				plugin->execute(cmd, msg);
			else if(msg.from_channel())
				LOG::I << "PLUGIN " << plugin->get_name()
					<< " NOT AUTHORISED FOR CHANNEL: " << msg.get_to();
			else
				LOG::I << "PLUGIN " << plugin->get_name()
					<< " NOT AUTHORISED FOR PM TO: " << msg.get_nickname();
		}
		catch(const std::exception& e)
		{
			LOG::E << "from plugin: " << plugin->get_name() << ": " << e.what();
		}
	}
}
str IrcBot::help(const str& cmd) const
{
	bug_fun();
	if(commands.count(cmd))// != commands.end())
		return commands.at(cmd)->help(cmd);

//	if(commands.count('!' + cmd))// != commands.end())
//		return commands.at('!' + cmd)->help('!' + cmd);

	// builtins
	str builtin;
	str_set keys = get_wild_keys("trigger.word");
	for(auto&& key: keys)
	{
		bug_var(key);
		bug_var(key.size());
		if((builtin = get(key)) != cmd)
			continue;

		bug_var(builtin);
		bug_var(key.size());

		auto pos = key.find_last_of('.');
		bug_var(pos);

		if(pos > key.size() - 1)
			continue;

		auto trigger = key.substr(pos + 1);
		bug_var(trigger);
		trigger = "trigger.help." + trigger;
		bug_var(trigger);
		if(have(trigger))
		{
			str h;
			str sep;
			for(auto&& t: get_vec(trigger))
			{
				hol::replace_all_mute(t, "\\t", "\t");
				hol::replace_all_mute(t, "\\n", "\n");
				hol::replace_all_mute(t, "\\s", "  ");
				h += sep + builtin + " " + t;
				sep = "\n";
			}
			return h;
		}
		break;
	}

	return "No help available for " + cmd + ".";
}

void IrcBot::exit()
{
	LOG::I << "Closing down plugins:";
	for(auto& p: plugins)
	{
		LOG::I << "\t" << p->get_name();
		p->exit();
	}

	fc.stop();

	LOG::I << "Closing down pinger:";
	done = true;
	if(png.valid())
		if(png.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			png.get();

	LOG::I << "Closing down console:";
	if(con.valid())
		if(con.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			con.get();

	irc->quit(restart ? "Reeeeeeeeeeeeeeeeeeboot!" : "Going off line...");
	LOG::I << "ENDED";
}

//static void prompt(std::ostream& os, size_t delay)
//{
//	std::this_thread::sleep_for(std::chrono::seconds(delay));
//	os << "> " << std::flush;
//}

void exec_reply(std::ostream* os, const str& msg)
{
	if(os)
		(*os) << msg << std::flush;
}

void IrcBot::exec(const std::string& cmd, std::ostream* os)
{
	bug_fun();
	bug_var(cmd);
	bug_var(os);
	str line = cmd;

	if(line[0] == '/')
	{
		str cmd;
		std::istringstream iss(line);
		iss >> cmd >> std::ws;
		hol::lower_mute(cmd);
		if(cmd == "/say")
		{
			// /say #channel <text>
			str chan;
			iss >> chan >> std::ws;
			bug_var(chan);
			if(!hol::trim_mute(chan).empty())// && chan[0] == '#')
			{
				sgl(iss, line);
				//bug_var(line);
				//irc->say(chan, line);
				fc_say(chan, line);
				exec_reply(os, "OK");
			}
			else
				exec_reply(os, "ERROR: /say #channel then some dialogue.\n");
		}
		else if(cmd == "/nop") // do nothing
		{
		}
		else if(cmd == "/ctcp")
		{
			// CTCP <target> <command> <arguments>
			// <target> is the target nickname or channel
			// <command> is the CTCP command (e.g. VERSION)
			// <arguments> are additional information to be sent to the <target>.

			// << PRIVMSG Mysfyt :VERSION
			// >> :Mysfyt!~Mysfyt@3cf137b8.1dad8cc8.fios.verizon.net NOTICE Lelly :VERSION mIRC v7.36 Khaled Mardam-Bey

			str target;
			str command;

			if(!(iss >> target))
				exec_reply(os, "ERROR: ctcp target not found");
			else if(!sgl(iss >> std::ws, command))
				exec_reply(os, "ERROR: ctcp command not found");
			else if(!irc->ctcp(target, command))
				exec_reply(os, "ERROR: sending ctcp command");
			else
				exec_reply(os, "OK");
		}
		else if(cmd == "/raw")
		{
			str raw;
			if(std::getline(iss, raw))
				irc->send(raw);
		}
		else if(cmd == "/nick")
		{
			str nick;
			if(iss >> nick)
				irc->nick(nick);
		}
		else if(cmd == "/botnick")
		{
			if(os)
				(*os) << nick << std::flush;
		}
//		else if(cmd == "/get_status")
//		{
//			lock_guard lock(status_mtx);
//			if(os)
//			{
//				for(const auto& s: status)
//					(*os) << s << '\n';
//				(*os) << std::flush;
//				status.clear();
//			}
//		}
		else if(cmd == "/die")
		{
			str_vec goodbyes = get_vec(PROP_GOODBYE);

			for(const str& c: chans)
			{
				if(!goodbyes.empty())
					irc->say(c, goodbyes[rand_int(0, goodbyes.size() - 1)]);
				irc->part(c);
			}
			done = true;
		}
		else if(cmd == "/restart")
		{
			str_vec goodbyes = get_vec(PROP_GOODBYE);

			for(const str& c: chans)
			{
				if(!goodbyes.empty())
					irc->say(c, goodbyes[rand_int(0, goodbyes.size() - 1)]);
				irc->part(c);
			}
			restart = true;
			done = true;
		}
		else if(cmd == "/reconfigure")
		{
			props.clear();
			if(!(std::ifstream(configfile) >> (*this)))
				LOG::I << "Error reading config file.";
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
					LOG::I << "Null plugin found during reinitialisation.";
					p = plugins.erase(p);
					continue;
				}
				if(!(*p)->init())
				{
					LOG::I << "Plugin failed during reinitialisation.";
					p = plugins.erase(p);
					continue;
				}
				LOG::I << "\tPlugin initialised: " << (*p)->get_name() << " v" << (*p)->get_version();
				for(str& c: (*p)->list())
				{
					LOG::I << "\t\tRegister command: " << c;
					commands[c] = p->get();
				}
				++p;
			}
		}
		else if(cmd == "/msg")
		{
			str to, s;
			iss >> to >> std::ws;
			if(std::getline(iss, s))
				irc->say(to, s);
		}
		else if(cmd == "/auth")
		{
			// TODO: FIXME
			str user, pass;
			if(iss >> user >> pass)
				irc->auth(user, pass);
		}
		else if(cmd == "/mode")
		{
			str nick, mode;
			iss >> nick >> std::ws;
			if(std::getline(iss, mode))
				irc->mode(nick, mode);
		}
		else if(cmd == "/whois")
		{
			str nick;
			iss >> nick;
			irc->whois({nick});
		}
		else if(cmd == "/me")
		{
			str channel;
			iss >> channel >> std::ws;
			if(!hol::trim_mute(channel).empty() && channel[0] == '#')
			{
				std::getline(iss, line);
				irc->me(channel, line);
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
			if(!hol::trim_mute(channel).empty() && channel[0] == '#')
			{
				if(irc->join(channel, key))
					chans.insert(channel);
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
			if(!hol::trim_mute(channel).empty() && channel[0] == '#')
			{
				std::getline(iss, line);
				if(chans.erase(channel)) irc->part(channel, hol::trim_mute(line));
				if(os) (*os) << "OK";
			}
			else if(os)
				(*os) << "ERROR: /part #channel [message].\n";
		}
		else if(cmd == "/ping")
		{
			str dest;
			std::getline(iss, dest);
			if(!irc->ping(dest)) { if(os) (*os) << "OK"; }
			else if(os)
				(*os) << "ERROR: Ping failure.\n";
		}
		else if(cmd == "/reconnect")
		{
			// TODO: not this - invoke pinger() reconnect cycle instead!
			str host = get(SERVER_HOST, SERVER_HOST_DEFAULT);
			siz port = get(SERVER_PORT, SERVER_PORT_DEFAULT);
			if(irc->connect(host, port)) { if(os) (*os) << "OK"; }
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

bool IrcBot::preg_match(const str& r, const str& s, bool full) const
{
//	if(full)
//		return pcrecpp::RE(r).FullMatch(s);
//	return pcrecpp::RE(r).PartialMatch(s);
	LOG::W << "deprecated function use: preg_match()";
	return sreg_match(r, s, full);
}

bool IrcBot::sreg_match(const str& r, const str& s, bool full) const
{
	std::regex e(r);
	if(full)
		return std::regex_match(s, e);
	return std::regex_search(s, e);
}

bool IrcBot::wild_match(const str& w, const str& s, int flags) const
{
	return !fnmatch(w.c_str(), s.c_str(), flags | FNM_EXTMATCH);
}

//void IrcBot::console()
//{
//	bug_fun();
//	std::istream& is = this->is ? *this->is : std::cin;
//	std::ostream& os = this->os ? *this->os : std::cout;
//
//	LOG::I << "CLOG::I << le started:";
//	str line;
//	prompt(os, 3);
//	for(; !done && std::getline(is, line); prompt(os, 3))
//	{
//		hol::trim_mute(line);
//		if(line.empty())
//			continue;
//		exec(line, &os);
//	}
//	LOG::I << "Console ended:";
//}

//void IrcBot::console()
//{
//	std::istream& is = this->is ? *this->is : std::cin;
//	std::ostream& os = this->os ? *this->os : std::cout;
//
//	LOG::I << "Console started:";
//	str line;
//	cstring_uptr input;
//
//	using_history();
//	read_history((str(getenv("HOME")) + "/.skivvy/.history").c_str());
//
//	input.reset(readline((nick + ": ").c_str()));
//
//	while(!done)
//	{
//		hol::trim_mute(line = input.get());
//		if(!line.empty())
//		{
//			add_history(line.c_str());
//			write_history((str(getenv("HOME")) + "/.skivvy/.history").c_str());
//
//			exec(line, &os);
//			os  << '\n';
//		}
//		input.reset(readline((nick + ": ").c_str()));
//	}
//	LOG::I << "Console ended:";
//}

void IrcBot::pinger()
{
	bug_fun();
	LOG::I << "Pinger started:";
	while(!done && irc)
	{
		const siz retries = get<int>(PROP_SERVER_RETRIES, 10);
		bug_var(retries);
		size_t attempt = 0;

		LOG::I << "Connecting:";

		str	host = get(SERVER_HOST, SERVER_HOST_DEFAULT);
		siz	port = get(SERVER_PORT, SERVER_PORT_DEFAULT);
		for(attempt = 0; !done && !irc->connect(host, port) && attempt < retries; ++attempt)
		{
			LOG::I << "Re-connection attempt: " << (attempt + 1);
			for(time_t now = time(0); !done && time(0) - now < 10;)
				std::this_thread::sleep_for(std::chrono::seconds(3));
		}

		if(!(attempt < retries))
		{
			LOG::I << "Giving up.";
			done = true;
		}

		if(!done)
		{
			LOG::I << "Connected:";
			connected = true;
			registered = false;

			irc->pass(get(PROP_SERVER_PASSWORD));

			// Attempt to connect with the first nick
			irc->nick(info.nicks[nick_number++]);
			irc->user(info.user, info.mode, info.real);

			while(!done && connected && !registered)
				std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		while(!done && connected && registered)
		{
			std::ostringstream oss;
			oss << std::rand();

			irc->ping(oss.str());
			size_t count = 60 * 3; // 3 minutes
			while(!done && --count)
				std::this_thread::sleep_for(std::chrono::seconds(1));

			pinger_info_mtx.lock();
			connected = pinger_info == oss.str();
			pinger_info_mtx.unlock();

			if(!connected)
			{
				LOG::I << "PING TIMEOUT: Attempting reconnect.";
				registered = false;
				nick_number = 0;
			}

			// each ping try to regain primary nick if
			// not already set.
			if(info.nicks.size() > 1 && nick != info.nicks[0])
			{
				nick_number = 0;
				irc->nick(info.nicks[nick_number++]);
			}
		}
	}
}

bool IrcBot::extract_params(const message& msg
	, std::initializer_list<str*> args, bool report)
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

}} // skivvy::ircbot
