#pragma once
#ifndef _SKIVVY_IRCBOT_H_
#define _SKIVVY_IRCBOT_H_

/*
 * ircbot.h
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

#include <sookee/socketstream.h>

#include <skivvy/FloodController.h>
#include <skivvy/logrep.h>
#include <skivvy/message.h>
#include <skivvy/RemoteIrcServer.h>
#include <skivvy/rpc.h>
#include <skivvy/socketstream.h>
#include <skivvy/store.h>
#include <skivvy/Timers.h>

#include <map>
#include <set>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <future>
#include <memory>
#include <queue>

#include <dlfcn.h>
#include <fnmatch.h>

#define _final_
#define _override_

namespace skivvy { namespace ircbot {

using namespace skivvy::types;
using namespace skivvy::utils;

const str DATA_DIR = "data_dir";

// Event to be triggered upon an incoming message
// For example when a task needs to be performed
// after the server has been queried for information
// these events can be triggered when that information
// asymmetrically arrives

struct msgevent
{
	bool done = false; // signal when complete (remove me)
	time_t when;
	time_t timeout = 30; // 30 seconds
	std::function<void(const message&)> func;
	msgevent(): when(std::time(0)) {}
};

typedef std::list<msgevent> msgevent_lst;
typedef std::map<str, msgevent_lst> msgevent_map;
typedef std::pair<const str, msgevent_lst> msgevent_pair;

struct user_info
{
	/**
	 * The in-channel nick (name) list.
	 * This is a list because a nick may be taken
	 * so alternatives may be supplied.
	 */
	std::vector<str> nicks;

	/**
	 * The username.
	 */
	str user;

	/**
	 * Requested connection mode.
	 */
	str mode;

	/**
	 * User's real name (yeah right).
	 */
	str real;
};

// ==================================================
// Bot Utils
// ==================================================

// ==================================================
// Bot Plugin Framework
// ==================================================

class RemoteIrcServer;
class IrcBot;
class message;

/**
 * IrcBotMonitor is an interface class for plugins
 * to implement if they want to receive all channel
 * messages, not just the ones triggered by their
 * commands.
 *
 * Plugins interested in receiving these events should
 * implement this interface and register themselves with
 * the IrcBot using IrcBot::add_monitor(IrcBotMonitor& mon).
 *
 * This should ideally be done in the bots implementation
 * of void BasicIrcBotPlugin::initialize().
 */
class IrcBotMonitor
{
public:
	virtual ~IrcBotMonitor() {}

	/**
	 * This function will be called by the IrcBot on
	 * each message it receives from the RemoteIrcServer.
	 *
	 */
	virtual void event(const message& msg) = 0;
};

/**
 * BotChoiceListener is an interface class for plugins
 * to implement if they want to receive answers to
 * multiple choice options, previously presented.
 *
 * Plugins interested in receiving these events should
 * implement this interface and register themselves with
 * the IrcBot using
 * IrcBot::add_choice_listener(BotChoiceListener& listener, time_t timeout).
 *
 * This should be done when the plugin presents a multiple choice question.
 *
 * The listener will be deregistered after an answer is given or
 * after a tmeout period.
 */
class IrcBotChoiceListener
{
public:
	virtual ~IrcBotChoiceListener() {}

	/**
	 * This function will receive each message and
	 * should return true if the message contains a valid choice.
	 */
	virtual bool valid(const message& msg) = 0;

	/**
	 * This function will be called by the IrcBot on
	 * each message it receives from the RemoteIrcServer
	 * that fits the choice criteria (valis() returns true.
	 *
	 */
	virtual void choice(const message& msg) = 0;
};

class IrcBotRPCService
: public rpc::service
{
public:
	typedef std::shared_ptr<rpc::call> call_ptr;

	virtual ~IrcBotRPCService() {}

	// INTERFACE RPC (Remote Procedure Call)

	/**
	 * Service name
	 */
	virtual str get_name() const = 0;

	/**
	 * Factory method for creating calls. Caller has ownership of
	 * the returned object and must delete it.
	 */
	virtual call_ptr create_call() const = 0;

	/**
	 * Remote procedure call. Allows plugins to communicate. A plugin
	 * will implement this function to expose useful functions that
	 * other plugins may find useful.
	 */
	virtual bool rpc(rpc::call& c) = 0;
};

/**
 * IrcBotPlugin is an interface class for all IrcBot plugins.
 * IrcBot plugin developers do not need to inherit from this
 * class directly as the abstract superclass BasicIrcBotPlugin
 * provides a plugin framework and inheriting from that minimises
 * the amount of work needed to produce a plugin.
 */
class IrcBotPlugin
{
public:
	void* dl = 0;
	typedef str_vec command_list;
	virtual ~IrcBotPlugin() {}

	/**
	 * This provides an opportunity for a plugin to initialise
	 * itself. It is called as the plugin is added to the IrcBot
	 * but before its commands are registered.
	 *
	 * @return false on failure
	 */
	virtual bool init() = 0;

	/**
	 * Return the name of the plugin.
	 */
	virtual str get_id() const = 0;

	/**
	 * Return the name of the plugin.
	 */
	virtual str get_name() const = 0;

	/**
	 * Return the version of the plugin.
	 */
	virtual str get_version() const = 0;

	/**
	 * Provide a list of commands that the plugin recognises. These
	 * all need to begin with the pling (bang) character '!'.
	 *
	 * @return a command_list of supported commands.
	 */
	virtual command_list list() const = 0;

	/**
	 * This function should perform the specified command. Any parameters
	 * required by the command can be extracted from the message object
	 * passed as a parameter to this function.
	 *
	 * @param cmd The command to be performed (eg. !dostuf)
	 * @param msg The message object received from the IRC server that
	 * contained this command. Any parameters can be extracted from this.
	 */
	virtual void execute(const str& cmd, const message& msg) = 0;

	/**
	 * Provide some textual help on a given command.
	 *
	 * @return The help text associated with the given command.
	 */
	virtual str help(const str& cmd) const = 0;

	/**
	 * This provides an opportunity for a plugin to clean
	 * itself up. It is called when the IrcBot is closed down.
	 * This is a good place to clean up any threads, close files etc.
	 */
	virtual void exit() = 0;
};

typedef std::shared_ptr<IrcBotPlugin> IrcBotPluginSPtr;

/**
 * This is the abstract superclass for bot
 * implementations. It implements most of the IrcBot
 * interface leaving the bot implementer to override
 * four virtual functions:
 *
 * <pre>
 * virtual void initialize() = 0;
 *
 *  This function should add the bot's actions by
 *  calling BasicIrcBotPlugin::add(const action& a) and
 *  otherwise initialize the bot. Also this is where the
 *  bot should register any IrcBotMonitor objects by
 *  calling IrcBot::add_monitor(IrcBotMonitor& mon).
 *
 * virtual str get_id() = 0;
 *
 *  This function should return the Bot's id.
 *
 * virtual str get_name() = 0;
 *
 *  This function should return the Bot's name.
 *
 * virtual str get_version() = 0;
 *
 *  This function should return the Bot's version.
 *
 * virtual void exit() = 0;
 *
 *  This function should clean up the bot before it is destroyed.
 *
 * </pre>
 *
 */
class BasicIrcBotPlugin
: public IrcBotPlugin
{
public:

	/**
	 * Implementing plugins create an action object for each
	 * command that the plugin understands. Each action object
	 * contains the command, instructions on how to use the command
	 * and a function object to execute the command.
	 *
	 * Implementing plugins should register their action objects by
	 * calling void add() in their implementation of bool initialize().
	 *
	 */
	struct action
	{
		typedef std::function<void(const message& msg)> action_func;
		enum
		{
			INVISIBLE = 0x01
		};
		str cmd;
		str help;
		action_func func;
		siz flags = 0;
		action() {}
		action(const str& cmd, const str& help, action_func func, siz flags = 0)
		: cmd(cmd), help(help), func(func), flags(flags) {}
	};

	typedef std::vector<action> action_vec;
	typedef std::map<str, action> action_map;
	typedef std::pair<str, action> action_pair;

protected:
	IrcBot& bot;
	RemoteIrcServer* irc;
	action_map actions;

	/**
	 * Implementing classes should call this method
	 * to register their commands. This should ideally
	 * be done in their implementation of the
	 * BasicIrcBotPlugin::initialize() function.
	 */
	void add(const action& a);

public:
	BasicIrcBotPlugin(IrcBot& bot);
	virtual ~BasicIrcBotPlugin();

	/**
	 * Implementing bots must override this function
	 * to initialize themselves.
	 *
	 * This is where the bot should
	 * check for the presence of plugind
	 * that it depends on.
	 *
	 * Also action objects should be registered in this function
	 * by calling void add():
	 *
	 * add
	 * ({
	 *     "!command"
	 *     , "Instructions on how to use !command"
	 *     , [&](const message& msg){ do_command(msg); }
	 * });
	 *
	 * Where do_command(const message& msg) is the function that
	 * actually performs the command.
	 *
	 * @return false on failure
	 */
	virtual bool initialize() = 0;

	// INTERFACE: IrcBotPlugin DON'T Override

	/**
	 * Implementing classes should not override this
	 * function. This class provides its own implementation
	 * which calls the pure virtual function
	 * BasicIrcBotPlugin::initialize(). Implementations
	 * should override that instead.
	 */
	virtual bool init(); // final

	/**
	 * Implementing classes should not override this
	 * function. This class provides its own implementation
	 * which supplies the return information from the list of
	 * BasicIrcBotPlugin::action objects provided by calling
	 * the void BasicIrcBotPlugin::add(const action& a) function.
	 */
	virtual str_vec list() const; // final

	/**
	 * Implementing classes should not override this
	 * function. This class provides its own implementation
	 * which calls the relevant std::function from the list of
	 * BasicIrcBotPlugin::action objects provided by calling
	 * the void BasicIrcBotPlugin::add(const action& a) function.
	 */
	virtual void execute(const str& cmd, const message& msg); // final

	/**
	 * Implementing classes should not override this
	 * function. This class provides its own implementation
	 * which supplies the return information from the list of
	 * BasicIrcBotPlugin::action objects provided by calling
	 * the void BasicIrcBotPlugin::add(const action& a) function.
	 */
	virtual str help(const str& cmd) const; // final

	virtual str get_id() const = 0;
	virtual str get_name() const = 0;
	virtual str get_version() const = 0;

	virtual void exit() = 0;
};

/**
 * The plugin implementation source should
 * use this macro to make itself loadable as
 * a plugin.
 *
 * The macro variable should be set to the name
 * of the plugin class to instantiate.
 *
 * The plugin class should derive from either
 * IrcBotPlugin or (more usually) BasicIrcBotPlugin.
 *
 */

#define IRC_BOT_PLUGIN(name) \
extern "C" \
IrcBotPluginSPtr skivvy_ircbot_factory(IrcBot& bot) \
{ \
	return IrcBotPluginSPtr(new name(bot)); \
} extern int _missing_semicolon_()

/**
 * Deals with all the dynamic loading jiggery pokery.
 *
 * Usage:
 *
 * IrcBot bot;
 * IrcBotPluginLoader load;
 * if(load(plugin_file_name, bot))
 * {
 *     // success
 * }
 *
 * @param file pathnam of plugin file to be loaded.
 * @param bot The bot that will be using this plugin.
 * @return true on success false on error
 * or 0 on false;
 */
class IrcBotPluginLoader
{
public:
	IrcBotPluginSPtr operator()(const str& file, IrcBot& bot);
};

// ==================================================
// The core Bot
// ==================================================

class IrcBot;

template<typename Plugin>
class IrcBotPluginHandle
{
	typedef std::shared_ptr<Plugin> PluginSPtr;

	IrcBot& bot;
	str id;
	PluginSPtr plugin;// = 0;
	time_t plugin_load_time = 0;

public:
	IrcBotPluginHandle(IrcBot& bot, const str& id);
	IrcBotPluginHandle(const IrcBotPluginHandle<Plugin>& handle);
//	IrcBotPluginHandle(IrcBotPluginHandle<Plugin>&& handle);

	operator void*() /*const*/;
	IrcBotPluginHandle& operator=(const IrcBotPluginHandle& handle);
	void ensure_plugin();
	Plugin& operator*();
	Plugin* operator->();
};

class IrcBot
//: private IrcBotPlugin
{
	friend class IrcBotPlugin;
public:

	typedef std::map<str, str_vec> property_map;
	typedef std::pair<const str, str_vec> property_pair;
	typedef property_map::iterator property_iter;
	typedef property_map::const_iterator property_citer;

	typedef std::pair<property_iter, property_iter> property_iter_pair;
	typedef std::pair<property_iter, property_iter> property_range;

	typedef std::vector<IrcBotPluginSPtr> plugin_vec;
	typedef plugin_vec::iterator plugin_vec_iter;
	typedef plugin_vec::const_iterator plugin_vec_citer;

	typedef std::set<IrcBotMonitor*> monitor_set;
	typedef std::set<IrcBotChoiceListener*> listener_set;
	typedef std::map<str, IrcBotRPCService*> service_map;
	typedef std::set<str> channel_set;
	typedef std::map<str, IrcBotPluginSPtr> command_map;
	typedef std::pair<str, IrcBotPluginSPtr> command_map_pair;

	typedef std::map<const str, str_set> nicks_map;
	typedef std::pair<const str, str_set> nicks_map_pair;
	typedef std::map<const str, str_set> str_str_set_map;
	typedef nicks_map::iterator nicks_iter;
	typedef nicks_map::const_iterator nick_citer;

private:
	RemoteIrcServer irc;
	FloodController fc;
	Store* store; // TODO: Why is this a pointer?

	msgevent_map msgevents;
	std::mutex msgevent_mtx;

	void dispatch_msgevent(const message& msg);

	std::mutex plugin_mtx; // use for plugins & monitors
	plugin_vec plugins;
	monitor_set monitors;
	listener_set listeners;
	service_map services;

	command_map commands; // str -> IrcBotPluginPtr
	// str -> { str -> IrcBotPluginPtr } // channel -> ( cmd -> plugin }

//	command_map channels; //
//	ban_set banned;
	property_map props;

	struct chan_prefix
	{
		str plugin;
		str prefix;

		bool operator<(const chan_prefix& cp) const { return plugin < cp.plugin; }
		bool operator==(const chan_prefix& cp) const { return plugin == cp.plugin; }
	};

	typedef std::set<chan_prefix> chan_prefix_set;
	typedef std::map<str, chan_prefix_set> chan_access_map;
	typedef std::pair<const str, chan_prefix_set> chan_access_pair;

	chan_access_map chan_access; // ("#channel"|"*"|"PM") -> { {"plugin#1", "prefix1"}, {"plugin#2", "prefix2"} }

	bool done;
	bool debug;

	bool connected;
	bool registered;

	std::future<void> con;
	std::istream* is;
	std::ostream* os;
	void console();

	std::future<void> png;
	std::mutex pinger_info_mtx;
	std::string pinger_info;
	void pinger();

	size_t nick_number;

	const std::time_t uptime;

//	str host;
//	siz port;

	str locate_file(const str& name);

	time_t config_loaded;
	time_t plugin_loaded;

	void load_plugins();

	bool has_access(const str& cmd, const str& chan);

	bool allow_cmd_access(const str& cmd, const message& msg);

public:
	bool restart = false;

	msgevent_lst::iterator add_msgevent(const str& msg, const msgevent& me)
	{
		lock_guard lock(msgevent_mtx);
		return msgevents[msg].insert(msgevents[msg].begin(), me);
	}

	msgevent_lst::iterator add_msgevent(const str& msg, const std::function<void(const message&)>& func, siz timeout = 30)
	{
		msgevent me;
		me.timeout = timeout;
		me.func = func;
		return add_msgevent(msg, me);
	}

	time_t get_plugin_load_time() { return plugin_loaded; }
	time_t get_config_load_time() { return config_loaded; }

	/**
	 * The nick from the list of nicks in user_info that was
	 * used to sign on with.
	 */
	str nick;

	user_info info;
	channel_set chans;
	nicks_map nicks; // last known nicks

	friend std::istream& operator>>(std::istream& is, IrcBot& bot);
	friend std::istream& operator>>(std::istream&& is, IrcBot& bot) { return is >> bot; }

	/**
	 * Match s according to regular expression r.
	 */
	bool preg_match(const str& r, const str& s, bool full = false); // peare regex
	bool wild_match(const str& w, const str& s, int flags = 0);

	bool is_connected() { return connected; }

	bool has(const str& s)
	{
		property_iter_pair i = props.equal_range(s);
		return i.first != i.second;
	}

	bool have(const str& s) { return has(s); }

	/**
	 * Join channel announcing bot version.
	 */
	void official_join(const str& channel);

	template<typename T>
	T get(const str& s, const T& dflt = T())
	{
		if(props[s].empty())
			return dflt;
		T t;
		std::istringstream(props[s][0]) >> std::boolalpha >> t;
		return t;
	}

	str get(const str& s, const str& dflt = "")
	{
		return props[s].empty() ? dflt : props[s][0];
	}

	/**
	 * Get a property variable intended to be a filename.
	 * The name is resolved such that absolute paths are
	 * left as they are but relative paths are apended
	 * to the value of the property variable data_dir:
	 *
	 * If data_dir: is unset the $HOME/.skivvy is used.
	 */
	str getf(const str& s, const str& dflt = "")
	{
		return locate_file(get(s, dflt));
	}

	str_vec get_vec(const str& s)
	{
		return props[s];
	}

	bool extract_params(const message& msg, std::initializer_list<str*> args, bool report = true);

public:
	IrcBot();

	/**
	 * Process external command, for example
	 * used by the rserver plugin to allow remote
	 * control.
	 */
	void exec(const std::string& cmd, std::ostream* os = 0);

	bool has_plugin(const str& id, const str& version = "");
	void del_plugin(const str& id);
	void add_plugin(IrcBotPluginSPtr p);
	void add_monitor(IrcBotMonitor& m);
	void add_rpc_service(IrcBotRPCService& s);

	IrcBotRPCService* get_rpc_service(const str& name)
	{
		return services.find(name) == services.end() ? 0 : services[name];
	}

	IrcBotPluginSPtr get_plugin(const str& id)
	{
		for(IrcBotPluginSPtr plugin: plugins)
			if(plugin->get_id() == id)
				return plugin;
		return IrcBotPluginSPtr(0);
	}

	template<typename Plugin>
	std::shared_ptr<Plugin> get_typed_plugin(const str& id)
	{
		for(IrcBotPluginSPtr plugin: plugins)
			if(plugin->get_id() == id)
				return std::shared_ptr<Plugin>(dynamic_cast<Plugin*>(plugin.get()));
		return std::shared_ptr<Plugin>(0);
	}

	template<typename Plugin>
	IrcBotPluginHandle<Plugin> get_plugin_handle(const str& id)
	{
		bug_func();
		bug_var(id);
		return IrcBotPluginHandle<Plugin>(*this, id);
	}

	/**
	 * Add channel to the list of channels
	 * to be joined at startup.
	 */
	void add_channel(const str& c);


	RemoteIrcServer& get_irc_server();

public:
	str configfile; // current config file

	bool fc_reply(const message& msg, const str& text);
	bool fc_reply_help(const message& msg, const str& text, const str& prefix = "");
	bool fc_reply_pm(const message& msg, const str& text);
	bool fc_reply_pm_help(const message& msg, const str& text, const str& prefix = "");

	bool cmd_error(const message& msg, const str& text, bool rv = false);
	bool cmd_error_pm(const message& msg, const str& text, bool rv = false);

	// INTERFACE: IrcBotPlugin

	virtual str get_id() const;
	virtual str get_name() const;
	virtual str get_version() const;

	virtual bool init(const str& config_file = "");
	virtual str_vec list() const;
	virtual void execute(const str& cmd, const message& msg);
	virtual str help(const str& cmd) const;
	virtual void exit();
};

template<typename Plugin>
IrcBotPluginHandle<Plugin>::IrcBotPluginHandle(IrcBot& bot, const str& id)
: bot(bot), id(id)
{
}

template<typename Plugin>
IrcBotPluginHandle<Plugin>::IrcBotPluginHandle(const IrcBotPluginHandle<Plugin>& handle)
: bot(handle.bot), id(handle.id), plugin(handle.plugin)
, plugin_load_time(handle.plugin_load_time)
{
}

template<typename Plugin>
IrcBotPluginHandle<Plugin>::operator void*() /*const*/
{
	ensure_plugin();
	return plugin.get();
}

template<typename Plugin>
IrcBotPluginHandle<Plugin>& IrcBotPluginHandle<Plugin>::operator=(const IrcBotPluginHandle& handle)
{
	id = handle.id;
	plugin = handle.plugin;
	plugin_load_time = handle.plugin_load_time;
	return *this;
}

template<typename Plugin>
void IrcBotPluginHandle<Plugin>::ensure_plugin()
{
	if(bot.get_plugin_load_time() > plugin_load_time)
	{
		plugin = bot.get_typed_plugin<Plugin>(id);
		plugin_load_time = std::time(0);
	}
}

template<typename Plugin>
Plugin& IrcBotPluginHandle<Plugin>::operator*()
{
	ensure_plugin();

	if(!plugin.get())
	{
		log("ERROR: Bad IrcBotPluginHandle: " << this);
	}
	return *plugin;
}

template<typename Plugin>
Plugin* IrcBotPluginHandle<Plugin>::operator->()
{
	ensure_plugin();

	if(!plugin.get())
	{
		log("ERROR: Bad IrcBotPluginHandle: " << this);
	}
	return plugin.get();
}

}} // skivvy::ircbot

#endif // _SKIVVY_IRCBOT_H_
