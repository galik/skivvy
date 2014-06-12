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

#include <sookee/types.h>
#include <sookee/bug.h>
#include <sookee/log.h>
#include <sookee/str.h>

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
#include <skivvy/logrep.h>
#include <skivvy/utils.h>
#include <skivvy/irc-constants.h>
#include <skivvy/message.h>

#include <random>
#include <functional>

#include <dirent.h>
//#include <regex.h>
#include <pcrecpp.h>
//#include <readline/readline.h>
//#include <readline/history.h>

namespace skivvy { namespace ircbot {

PLUGIN_INFO("skivvy", "IrcBot", "0.3");

using namespace skivvy;
using namespace skivvy::irc;
using namespace sookee::types;
using namespace skivvy::utils;

using namespace sookee::bug;
using namespace sookee::log;
using namespace sookee::string;

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

BasicIrcBotPlugin::BasicIrcBotPlugin(IrcBot& bot)
: bot(bot), irc(0)
{
}

BasicIrcBotPlugin::~BasicIrcBotPlugin() {}

void BasicIrcBotPlugin::add(const action& a) { actions.insert(action_pair{a.cmd, a}); }

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
	bug_func();
	bug_var(cmd);
	if(!actions.count(cmd))
	{
		log("ERROR: unknown command: " << cmd);
		return "unknown command";
	}
	if(actions.at(cmd).flags & action::INVISIBLE)
		return "none";
	bug_var(actions.at(cmd).help);
	return actions.at(cmd).help;
}

IrcBotPluginSPtr IrcBotPluginLoader::operator()(const str& file, IrcBot& bot)
{
	void* dl = 0;
	IrcBotPluginSPtr plugin;
	IrcBotPluginPtr(*skivvy_ircbot_factory)(IrcBot&) = 0;

	log("PLUGIN LOAD: " << file);

	//if(!(dl = dlopen(file.c_str(), RTLD_NOW|RTLD_GLOBAL)))
	if(!(dl = dlopen(file.c_str(), RTLD_LAZY|RTLD_GLOBAL)))
	{
		log(dlerror());
		return plugin;
	}

	if(!(*(void**)&skivvy_ircbot_factory = dlsym(dl, "skivvy_ircbot_factory")))
	{
		log(dlerror());
		return plugin;
	}

	if(!(plugin = IrcBotPluginSPtr(skivvy_ircbot_factory(bot))))
		return plugin;

	plugin->dl = dl;
	bot.del_plugin(plugin->get_id());
	bot.add_plugin(plugin);
	return plugin;
}

void IrcBot::add_plugin(IrcBotPluginSPtr plugin)
{
	if(plugin)
		this->plugins.push_back(plugin);
	else
		log("ERROR: Adding non-plugin.");
}

bool IrcBot::has_plugin(const str& id, const str&  version)
{
	for(const IrcBotPluginSPtr& p: plugins)
		if(p->get_id() == id && p->get_version() >= version)
			return true;
	return false;
}

void IrcBot::del_plugin(const str& id)
{
	bug_func();
	for(plugin_vec_iter p =  plugins.begin(); p != plugins.end();)
	{
		if((*p)->get_id() != id)
			++p;
		else
		{
			lock_guard lock(plugin_mtx);
			if(monitors.erase(dynamic_cast<IrcBotMonitor*>(p->get())))
				log("Unregistering monitor: " << (*p)->get_name());

			bug("exiting plugin: " << (*p)->get_name());
			(*p)->exit();
			bug("dlclose plugin: " << (*p)->get_name());
			dlclose((*p)->dl);
			log("Unregistering plugin : " << (*p)->get_name());
			p = plugins.erase(p);
		}
	}
}

// IRCBot

IrcBot::IrcBot()
: irc()
, store(0)
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
		if((pos = line.find("//")) != str::npos)
			line.erase(pos);
		trim(line);

		if(line.empty() || line[0] == '#')
			continue;

//		bug("prop: " << line);

		str key, val;
		if(!sgl(sgl(siss(line) >> std::ws, key, ':') >> std::ws, val))
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
		else if(key == "include")
		{
			bug("include:" << val);
			if(val.empty())
				continue;
			str file_name;
			if(val[0] == '/')
				file_name = val;
			else
			{
				str file_path = bot.configfile.substr(0, bot.configfile.find_last_of('/'));
				//bug_var(file_path);
				file_name = file_path + "/" + val;
				//bug_var(file_name);
			}
			//bug_var(file_name);
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

IrcServer* IrcBot::get_irc_server() { return irc; }

// Utility

// flood control
bool IrcBot::fc_reply_note(const message& msg, const str& text)
{
	return fc.send(msg.get_to(), [&,msg,text]()->bool{ return irc->notice(msg.get_to(), text); });
}

bool IrcBot::fc_reply(const message& msg, const str& text)
{
	return fc.send(msg.get_to(), [&,msg,text]()->bool{ return irc->reply(msg, text); });
}

bool IrcBot::fc_say(const str& to, const str& text)
{
	return fc.send(to, [&,to,text]()->bool{ return irc->say(to, text); });
}

//bool IrcBot::fc_reply(const str& to, const str& text)
//{
//	message msg;
//	msg.params = " " + to + " :"; // fudge message for reply to correct channel|person
//	return fc_reply(msg, text);
//}

bool IrcBot::fc_reply_help(const message& msg, const str& text, const str& prefix)
{
	const str_vec v = split(text, '\n'); // FIXME: crasher?
	for(const str& s: v)
		if(!fc.send(msg.get_to(), [&,msg,prefix,s]()->bool{ return irc->reply(msg, prefix + s); }))
			return false;

	return true;
}

bool IrcBot::fc_reply_pm(const message& msg, const str& text)//, size_t priority)
{
	return fc.send(msg.get_to(), [&,msg,text]()->bool{ return irc->reply_pm(msg, text); });
}

bool IrcBot::fc_reply_pm_help(const message& msg, const str& text, const str& prefix)
{
	str_vec v = split(text, '\n');
	//split(text, v, '\n');
	for(const str& s: v)
	{
		str to = msg.get_to();
		if(!fc.send(to, [&,msg,prefix,s]()->bool{ return irc->reply_pm(msg, prefix + s); }))
			return false;
	}
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

str IrcBot::get_id() const { return ID; }
str IrcBot::get_name() const { return NAME; }
str IrcBot::get_version() const { return VERSION; }

void IrcBot::official_join(const str& channel)
{
	std::this_thread::sleep_for(std::chrono::seconds(get(PROP_JOIN_DELAY, PROP_JOIN_DELAY_DEFAULT)));

	if(!irc->join(channel))
		return;

	chans.insert(channel);
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
	bug_func();
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

	for(const str& line: pv)
	{
		//bug_var(line);
		str p, access;
		sgl(sgl(siss(line), p, '['), access, ']');
		trim(p);
		bug_var(p);
		bug_var(access);
		for(const str& plugin_dir: plugin_dirs)
		{
			str_vec files;
			if(int e = ios::ls(plugin_dir, files))
			{
				log(strerror(e));
				continue;
			}

			if(stl::find(files, p) == files.end())
				continue;

			IrcBotPluginSPtr plugin;
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
				trim(prefix);
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
	bug_func();
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

//	bug_do_color = get("bug.do.color", true);

	// =====================================
	// CREATE CRITICAL RESOURCES
	// =====================================

	store = new BackupStore(getf(IRCBOT_STORE_FILE, IRCBOT_STORE_FILE_DEFAULT));
	if(!store)
	{
		log("Error creating Store.");
		return false;
	}

	if(get<bool>("irc.test.mode") == true)
	{
		irc = new TestIrcServer;
		str host = get("irc.test.host", "test-host");
		siz port = get("irc.test.port", 0);
		if(irc)
			irc->connect(host, port);
	}
	else
		irc = new RemoteIrcServer;

	if(!irc)
	{
		log("Error creating IrcServer.");
		return false;
	}

	// =====================================
	// OPEN LOG FILE
	// =====================================

	std::ofstream logfile;

	if(has("log.file"))
	{
		logfile.open(getf("log.file"));
		if(logfile)
		{
			sookee::log::out(&logfile);
			sookee::bug::out(&logfile);
		}
	}

	load_plugins();

	if(!have(SERVER_HOST))
	{
		log("ERROR: No server configured.");
		return false;
	}

	if(info.nicks.empty())
	{
		log("ERROR: No nick set.");
		return false;
	}

	fc.start();

	log("Initializing plugins [" << plugins.size() << "]:");
	plugin_vec_iter p = plugins.begin();
	while(p != plugins.end())
	{
		//bug_var(&(*p));
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
		log("\tPlugin initialised: " << (*p)->get_id() << ": " << (*p)->get_name() << " v" << (*p)->get_version());
		for(str& c: (*p)->list())
		{
			log("\t\tRegister command: " << c);
			commands[c] = *p;
		}
		++p;
	}

	bug_var(done);

	if(get<bool>("irc.test.mode") == true)
		connected = true;
	else
		png = std::async(std::launch::async, [&]{ pinger(); });

	while(!done && !connected)
		std::this_thread::sleep_for(std::chrono::seconds(1));

	if(get<bool>("irc.test.mode") == false)
		con = std::async(std::launch::async, [&]{ console(); });

	str line;
	message msg;
	while(!done)
	{
		if(!irc->receive(line))
		{
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

		if(line.empty())
			continue;

		msg.clear();
		if(!parsemsg(line, msg))
			continue;

		dispatch_msgevent(msg);

		for(IrcBotMonitor* m: monitors)
			if(m) m->event(msg);

		// LOGGING

		static const str_vec nologs =
		{
			"PING", "PONG", "372"
		};

		if(stl::find(nologs, msg.command) == nologs.cend())
			log("recv: " << line);

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
				exec(replace(prop, "$me", nick));
			for(const str& chan: get_vec(PROP_JOIN))
				official_join(chan);
			for(const str& chan: store->get_set("invite"))
				official_join(chan);
			registered = true;
		}
		else if(msg.command == RPL_NAMREPLY)
		{
			//BUG_MSG(msg, RPL_NAMREPLY);
			str channel, nick;
			str_vec params = msg.get_params();
			if(params.size() > 2)
				channel = params[2];
			bug("channel: " << channel);
			siss iss(msg.get_trailing());
			while(iss >> nick)
				if(!nick.empty())
					nicks[channel].insert((nick[0] == '@' || nick[0] == '+') ? nick.substr(1) : nick);
		}
		else if(msg.command == ERR_NOORIGIN)
		{
			//BUG_MSG(msg, ERR_NOORIGIN);
		}
		else if(msg.command == ERR_NICKNAMEINUSE)
		{
			//BUG_MSG(msg, ERR_NICKNAMEINUSE);
			if(nick_number < info.nicks.size())
			{
				irc->nick(info.nicks[nick_number++]);
				irc->user(info.user, info.mode, info.real);
			}
			else
			{
				log("ERROR: Unable to connect with any of the nicks.");
				done = true;
			}
		}
		else if(msg.command == RPL_WHOISCHANNELS)
		{
			//BUG_MSG(msg, RPL_WHOISCHANNELS);
		}
		else if(msg.command == INVITE)
		{
			//BUG_MSG(msg, INVITE);
			str who, chan;
			str_vec params = msg.get_params();

			if(params.size() > 0)
				who = params[0];
			if(params.size() > 1)
				chan = params[1];

			str_vec parts = get_vec("part");

			for(const str& part: parts)
				bug_var(part);

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
			//BUG_MSG(msg, JOIN);
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
			//BUG_MSG(msg, PART);
		}
		else if(msg.command == KICK)
		{
			//BUG_MSG(msg, KICK);
		}
		else if(msg.command == NICK)
		{
			// Is the bot changed its own nick?
			if(msg.get_nickname() == nick)
				nick = msg.get_trailing();
		}
		else if(msg.command == PRIVMSG)
		{
			if(!msg.get_trailing().empty() && msg.get_trailing()[0] == '!')
			{
				str cmd = lower_copy(msg.get_user_cmd());
				log("Processing command: " << cmd);
				if(cmd == "!die")
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
				else if(cmd == "!uptime")
				{
					soss oss;
					print_duration(st_clk::now() - st_clk::from_time_t(uptime), oss);
					str time = oss.str();
					trim(time);
					fc_reply(msg, "I have been active for " + time);
				}
				else if(cmd == "!join")
				{
					str_vec param = split(msg.get_user_params(), ' ', true);
					//split(msg.get_user_params(), param, ' ', true);

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
				else if(cmd == "!part")
				{
					str_vec param = split(msg.get_user_params(), ' ', true);

					//split(msg.get_user_params(), param, ' ', true);

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
				else if(cmd == "!reconfigure")
				{
					str pass;
					if(!(ios::getstring(siss(msg.get_user_params()), pass)))
					{
						fc_reply(msg, "!restart <password>");
						continue;
					}
					bug_var(pass);
					if(!have(PROP_PASSWORD) || get(PROP_PASSWORD) == pass)
					{
						exec("/reconfigure");
						continue;
					}
					fc_reply(msg, "Wrong password");
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
					for(const IrcBotPluginSPtr& p: plugins)
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
					if(have("help.append"))
						fc_reply_pm(msg, "Additional Info:");
					for(str h: get_vec("help.append"))
						fc_reply_pm(msg, "\t" + replace(h, "$me", nick));
				}
				else
				{
					for(const str& ban: get_vec("ban"))
						if(wild_match(ban, msg.get_userhost()))
						{
							log("BANNED: " << msg.get_nickname());
							continue;
						}
					//std::async(std::launch::async, [=]{ execute(cmd, msg); });
					execute(cmd, msg);
				}
			}
			else if(!msg.get_trailing().empty() && msg.get_to() == nick)
			{
				// PM to bot accepts commands without !
				str cmd = lower_copy(msg.get_user_cmd());
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
	for(const IrcBotPluginSPtr& p: plugins)
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
		if(has_access(cmd, msg.get_to()))
			return true;

	if(has_access(cmd, "PM"))
		return true;

	return false;
}

void IrcBot::execute(const str& cmd, const message& msg)
{
//	bug_func();
//	bug_var(cmd);
//	bug_msg(msg);
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
				<< " NOT AUTHORISED FOR CHANNEL: " << msg.get_to());
		else
			log("PLUGIN " << commands[cmd]->get_name()
				<< " NOT AUTHORISED FOR PM TO: " << msg.get_nickname());
	}
}

str IrcBot::help(const str& cmd) const
{
	bug_func();
	if(commands.count(cmd))// != commands.end())
		return commands.at(cmd)->help(cmd);

	if(commands.count('!' + cmd))// != commands.end())
		return commands.at('!' + cmd)->help('!' + cmd);

	return "No help available for " + cmd + ".";
}
void IrcBot::exit()
{
	log("Closing down plugins:");
	for(IrcBotPluginSPtr p: plugins)
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

	irc->quit(restart ? "Reeeeeeeeeeeeeeeeeeboot!" : "Going off line...");
	log("ENDED");
}

static void prompt(std::ostream& os, size_t delay)
{
	std::this_thread::sleep_for(std::chrono::seconds(delay));
	os << "> " << std::flush;
}

void IrcBot::exec(const std::string& cmd, std::ostream* os)
{
	bug_func();
	bug_var(cmd);
	bug_var(os);
	str line = cmd;

	if(line[0] == '/')
	{
		str cmd;
		std::istringstream iss(line);
		iss >> cmd >> std::ws;
		lower(cmd);
		if(cmd == "/say")
		{
			// /say #channel <text>
			str chan;
			iss >> chan >> std::ws;
			bug_var(chan);
			if(!trim(chan).empty())// && chan[0] == '#')
			{
				sgl(iss, line);
				//bug_var(line);
				//irc->say(chan, line);
				fc_say(chan, line);
				if(os)
					(*os) << "OK";
			}
			else if(os)
				(*os) << "ERROR: /say #channel then some dialogue.\n";
		}
		else if(cmd == "/nop") // do nothing
		{
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
			if(!trim(channel).empty() && channel[0] == '#')
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
			if(!trim(channel).empty() && channel[0] == '#')
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
			if(!trim(channel).empty() && channel[0] == '#')
			{
				std::getline(iss, line);
				if(chans.erase(channel)) irc->part(channel, trim(line));
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

//str get_regerror(int errcode, regex_t *compiled)
//{
//	size_t length = regerror(errcode, compiled, NULL, 0);
//	char *buffer = new char[length];
//	(void) regerror(errcode, compiled, buffer, length);
//	str e(buffer);
//	delete[] buffer;
//	return e;
//}
//
//bool IrcBot::ereg_match(const str& r, const str& s)
//{
//	bug_func();
//	bug_var(s);
//	bug_var(r);
//	regex_t regex;
//
//	if(regcomp(&regex, r.c_str(), REG_EXTENDED | REG_ICASE))
//	{
//		log("Could not compile regex: " << r);
//		return false;
//	}
//
//	int reti = regexec(&regex, s.c_str(), 0, NULL, 0);
//	regfree(&regex);
//	if(!reti)
//		return true;
//	else if(reti != REG_NOMATCH)
//		log("regex: " << get_regerror(reti, &regex));
//
//	return false;
////	return lowercase(s).find(lowercase(r)) != str::npos;
//}

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
	bug_func();
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

//void IrcBot::console()
//{
//	std::istream& is = this->is ? *this->is : std::cin;
//	std::ostream& os = this->os ? *this->os : std::cout;
//
//	log("Console started:");
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
//		trim(line = input.get());
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
//	log("Console ended:");
//}

void IrcBot::pinger()
{
	bug_func();
	log("Pinger started:");
	while(!done && irc)
	{
		const siz retries = get<int>(PROP_SERVER_RETRIES, 10);
		bug_var(retries);
		size_t attempt = 0;

		log("Connecting:");

		str	host = get(SERVER_HOST, SERVER_HOST_DEFAULT);
		siz	port = get(SERVER_PORT, SERVER_PORT_DEFAULT);
		for(attempt = 0; !done && !irc->connect(host, port) && attempt < retries; ++attempt)
		{
			log("Re-connection attempt: " << (attempt + 1));
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
				log("PING TIMEOUT: Attempting reconnect.");
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

}} // skivvy::ircbot
