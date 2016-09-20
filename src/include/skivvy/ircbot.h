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

#include <experimental/filesystem>

//#include <sookee/socketstream.h>
//#include <skivvy/socketstream.h> // TODO: REMOVE THIS!!!
//#include <sookee/ssl_socketstream.h>

#include <boost/asio.hpp>

#include <skivvy/FloodController.h>
#include <hol/small_types.h>
#include <hol/bug.h>
#include <hol/simple_logger.h>
#include <skivvy/message.h>
#include <skivvy/IrcServer.h>
#include <skivvy/rpc.h>
//#include <skivvy/socketstream.h>
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

namespace fs = std::experimental::filesystem;

namespace skivvy { namespace ircbot {

using namespace hol::small_types::basic;
using namespace hol::small_types::string_containers;
using namespace hol::simple_logger;
using namespace skivvy::utils;

const str DATA_DIR = "home";

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

//class RemoteIrcServer;
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
	 * Opportunity for plugin to offer API for other
	 * plugins to communicate over.
	 *
	 * @param call
	 * @param args
	 * @return
	 */
	virtual str_vec api(unsigned call, const str_vec& args = {})
	{
		(void) call;
		(void) args;
		return {};
	}

	/**
	 * This provides an opportunity for a plugin to initialize
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

typedef IrcBotPlugin* IrcBotPluginRPtr;
typedef std::unique_ptr<IrcBotPlugin> IrcBotPluginUPtr;
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
			INVISIBLE = 0b01, ALIAS = 0b10
		};
		str cmd;
		str help;
		action_func func;
		siz flags = 0;
		action() {}
		action(const str& cmd, const str& help, action_func func, siz flags = 0)
		: cmd(cmd), help(help), func(func), flags(flags) {}
	};

//	typedef std::vector<action> action_vec;
	typedef std::map<str, action> action_map;
	typedef std::pair<str, action> action_pair;

protected:
	IrcBot& bot;
	IrcServer* irc = nullptr;
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
	virtual bool init() final;

	/**
	 * Implementing classes should not override this
	 * function. This class provides its own implementation
	 * which supplies the return information from the list of
	 * BasicIrcBotPlugin::action objects provided by calling
	 * the void BasicIrcBotPlugin::add(const action& a) function.
	 */
	virtual str_vec list() const final;

	/**
	 * Implementing classes should not override this
	 * function. This class provides its own implementation
	 * which calls the relevant std::function from the list of
	 * BasicIrcBotPlugin::action objects provided by calling
	 * the void BasicIrcBotPlugin::add(const action& a) function.
	 */
	virtual void execute(const str& cmd, const message& msg) final;

	/**
	 * Implementing classes should not override this
	 * function. This class provides its own implementation
	 * which supplies the return information from the list of
	 * BasicIrcBotPlugin::action objects provided by calling
	 * the void BasicIrcBotPlugin::add(const action& a) function.
	 */
	virtual str help(const str& cmd) const final;

	virtual str get_id() const = 0;
	virtual str get_name() const = 0;
	virtual str get_version() const = 0;

	virtual void exit() = 0;
};

//class ManagedIrcBotPlugin
//: public IrcBotPlugin
//{
//public:
//	typedef std::function<void(const message& msg)> action_func;
//	typedef std::map<str, action_func> action_map;
//
//private:
//	str get_trigger_key(const str& part, const str& trigger) const
//	{
//		return get_id() + ".trigger." + part + "." + trigger;
//	}
//
//protected:
//	IrcBot& bot;
//	IrcServer* irc = nullptr;
//
//	struct info
//	{
//		str alias; // default alias
//		str help; // default help
//	};
//
//	using info_map = std::map<str, info>;
//
//	action_map actions; // trigger -> action_func
//	info_map infos; // trigger -> {alias, help}
//	str_map aliases; // alias -> trigger
//
//	void add(const str& trigger, const str& alias, const str& help, action_func func);
//
//public:
//	ManagedIrcBotPlugin(IrcBot& bot);
//	virtual ~ManagedIrcBotPlugin();
//
//	/**
//	 * Implementing bots must override this function
//	 * to initialize themselves.
//	 *
//	 * This is where the bot should
//	 * check for the presence of plugins
//	 * that it depends on.
//	 *
//	 * Also action objects should be registered in this function
//	 * by calling void add():
//	 *
//	 * add
//	 * ({
//	 *     "trigger"
//	 *     , "default alias" // !alarm
//	 *     , "default help" // Does stuff
//	 *     , [&](const message& msg){ do_command(msg); }
//	 * });
//	 *
//	 * Where do_command(const message& msg) is the function that
//	 * actually performs the command.
//	 *
//	 * @return false on failure
//	 */
//	virtual bool initialize() = 0;
//
//	// INTERFACE: IrcBotPlugin DON'T Override
//
//	/**
//	 * Implementing classes should not override this
//	 * function. This class provides its own implementation
//	 * which calls the pure virtual function
//	 * BasicIrcBotPlugin::initialize(). Implementations
//	 * should override that instead.
//	 */
//	bool init() final override;
//
//	/**
//	 * Implementing classes should not override this
//	 * function. This class provides its own implementation
//	 * which supplies the return information from the list of
//	 * BasicIrcBotPlugin::action objects provided by calling
//	 * the void BasicIrcBotPlugin::add(const action& a) function.
//	 */
//	str_vec list() const final override;
//
//	/**
//	 * Implementing classes should not override this
//	 * function. This class provides its own implementation
//	 * which calls the relevant std::function from the list of
//	 * BasicIrcBotPlugin::action objects provided by calling
//	 * the void BasicIrcBotPlugin::add(const action& a) function.
//	 */
//	virtual void execute(const str& cmd, const message& msg) final;
//
//	/**
//	 * Implementing classes should not override this
//	 * function. This class provides its own implementation
//	 * which supplies the return information from the list of
//	 * BasicIrcBotPlugin::action objects provided by calling
//	 * the void BasicIrcBotPlugin::add(const action& a) function.
//	 */
//	virtual str help(const str& cmd) const final;
//
//	virtual str get_id() const = 0;
//	virtual str get_name() const = 0;
//	virtual str get_version() const = 0;
//
//	virtual void exit() = 0;
//};

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
skivvy::ircbot::IrcBotPluginRPtr skivvy_ircbot_factory(skivvy::ircbot::IrcBot& bot) \
{ \
	return new name(bot); \
} struct _missing_semicolon_

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
	IrcBotPluginRPtr operator()(const str& file, IrcBot& bot);
};

// ==================================================
// The core Bot
// ==================================================

class IrcBot;

//template<typename Plugin>
//class IrcBotPluginHandle
//{
//	typedef std::shared_ptr<Plugin> PluginSPtr;
//
//	IrcBot& bot;
//	str id;
//	PluginSPtr plugin;// = 0;
//	time_t plugin_load_time = 0;
//
//public:
//	IrcBotPluginHandle(IrcBot& bot, const str& id);
//	IrcBotPluginHandle(const IrcBotPluginHandle<Plugin>& handle);
////	IrcBotPluginHandle(IrcBotPluginHandle<Plugin>&& handle);
//
//	operator void*() /*const*/;
//	IrcBotPluginHandle& operator=(const IrcBotPluginHandle& handle);
//	void ensure_plugin();
//	Plugin& operator*();
//	Plugin* operator->();
//};

class IrcBotPluginHandle
{
	IrcBot& bot;
	str id;
	IrcBotPluginRPtr plugin = 0;
	time_t plugin_load_time = 0;

public:
	IrcBotPluginHandle(IrcBot& bot, const str& id);
	IrcBotPluginHandle(const IrcBotPluginHandle& handle);
//	IrcBotPluginHandle(IrcBotPluginHandle<Plugin>&& handle);

	operator void*() /*const*/;
	IrcBotPluginHandle& operator=(const IrcBotPluginHandle& handle);
	void ensure_plugin();
	IrcBotPlugin& operator*();
	IrcBotPlugin* operator->();
};

class IrcBot
//: private IrcBotPlugin
{
	friend class IrcBotPlugin;
public:
	virtual ~IrcBot() {}

	typedef std::map<str, str_vec> property_map;
	typedef std::pair<const str, str_vec> property_pair;
	typedef property_map::iterator property_iter;
	typedef property_map::const_iterator property_citer;

	typedef std::pair<property_iter, property_iter> property_iter_pair;
	typedef std::pair<property_iter, property_iter> property_range;

	typedef std::vector<IrcBotPluginUPtr> plugin_vec;
	typedef plugin_vec::iterator plugin_vec_iter;
	typedef plugin_vec::const_iterator plugin_vec_citer;

	typedef std::set<IrcBotMonitor*> monitor_set;
	typedef std::set<IrcBotChoiceListener*> listener_set;
	typedef std::map<str, IrcBotRPCService*> service_map;
	typedef std::set<str> channel_set;
	typedef std::map<str, IrcBotPluginRPtr> command_map;
	typedef std::pair<str, IrcBotPluginRPtr> command_map_pair;

	typedef std::map<const str, str_set> nicks_map;
	typedef std::pair<const str, str_set> nicks_map_pair;
	typedef std::map<const str, str_set> str_str_set_map;
	typedef nicks_map::iterator nicks_iter;
	typedef nicks_map::const_iterator nick_citer;

private:
	FloodController fc;

	// pointers because need to load config before creating
	IrcServerUptr irc;
	StoreUPtr store;

	msgevent_map msgevents;
	std::mutex msgevent_mtx;

	void dispatch_msgevent(const message& msg);

	std::mutex plugin_mtx; // use for plugins & monitors
	plugin_vec plugins;
	monitor_set monitors;
	listener_set listeners;
	service_map services;

	command_map commands; // str -> IrcBotPluginRPtr
	// str -> { str -> IrcBotPluginPtr } // channel -> ( cmd -> plugin }

//	/*
//	 * plugin_aliases is build by reading the config and
//	 * each "plugin.alias: plugin_id:!cmd !alias1 !alias2"
//	 * is converted into a set of entries in plugin_aliases,
//	 * one for each alias associated with a given cmd of the form:
//	 *
//	 * plugin_id:!cmd !alias1 !alias2
//	 *
//	 * When a plugin registers with command <cmd> that <cmd>
//	 * is looked up in the plugin_aliases and translated into
//	 * <alias1> <alias2> which are placed in command_map commands
//	 * against the same action function.
//	 */
//	str_map plugin_aliases; // "plugin:cmd" -> "alias"
//
//	/*
//	 * When a command <cmd> arrives at the bot, it applies
//	 * the channel_aliases to it producing <alias>.
//	 *
//	 * That <alias> is used to access command_map commands.
//	 */
//	str_map channel_aliases; // "plugin:cmd" -> "alias"

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

	bool debug;

	std::atomic_bool done{false};
	std::atomic_bool connected{false};
	std::atomic_bool registered{false};

	std::future<void> con;
	std::istream* is;
	std::ostream* os;
//	void console();

	std::future<void> png;
	std::mutex pinger_info_mtx;
	std::string pinger_info;
	void pinger();

	size_t nick_number;

	const std::time_t uptime;

//	str host;
//	siz port;

	str locate_config_dir() const;
	str locate_config_file(const str& name) const;

	time_t config_loaded;
	time_t plugin_loaded;

	std::ofstream logfile;
	std::ofstream bugfile;

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

	std::istream* set_istream(std::istream* is) { std::istream* r = this->is; this->is = is; return r; }
	std::ostream* set_ostream(std::ostream* os) { std::ostream* r = this->os; this->os = os; return r; }

	time_t get_plugin_load_time() const { return plugin_loaded; }
	time_t get_config_load_time() const { return config_loaded; }

	/**
	 * The nick from the list of nicks in user_info that was
	 * used to sign on with.
	 */
	str nick;

	user_info info;
	channel_set chans;
	nicks_map nicks; // last known nicks // chan -> nick_list

	friend std::istream& load_props(std::istream&, IrcBot&, str_map&, str&);
	friend std::istream& operator>>(std::istream& is, IrcBot& bot);
	friend std::istream& operator>>(std::istream&& is, IrcBot& bot) { return is >> bot; }

	/**
	 * Match s according to regular expression r.
	 */
	bool preg_match(const str& r, const str& s, bool full = false) const; // pcre regex
	bool wild_match(const str& w, const str& s, int flags = 0) const;
	bool sreg_match(const str& w, const str& s, bool full = false) const; // std::regex

	bool is_connected() const { return connected; }

	bool has(const str& s) const
	{
		auto i = props.equal_range(s);
		return i.first != i.second;
	}

	bool have(const str& s) const { return has(s); }

	/**
	 * Join channel announcing bot version.
	 */
	void official_join(const str& channel);

	template<typename T>
	T get(const str& s, const T& dflt = T()) const
	{
		auto prop = props.find(s);
		if(prop == props.end() || prop->second.empty())
			return dflt;
		T t;
		std::istringstream(prop->second[0]) >> std::boolalpha >> t;
		return t;
	}

	str get(const str& s, const str& dflt = "") const
	{
		auto prop = props.find(s);
		if(prop == props.end() || prop->second.empty())
			return dflt;

		return prop->second.at(0);
//		return props[s].empty() ? dflt : props[s][0];
	}

	/**
	 * Get a property variable intended to be a filename.
	 * The name is resolved such that absolute paths are
	 * left as they are but relative paths are appended
	 * to the value of the property variable data_dir:
	 *
	 * If data_dir: is unset the $HOME/.skivvy is used.
	 */
	str getf(const str& s, const str& dflt = "") const
	{
		return locate_config_file(get(s, dflt));
	}

	str get_data_folder() const
	{
		return locate_config_dir();
	}

	str_vec get_vec(const str& s) const
	{
		auto prop = props.find(s);
		if(prop == props.end() || prop->second.empty())
			return {};
		return prop->second;
	}

	str_vec get_vec(const str& s, const str& dflt) const
	{
		str_vec v = get_vec(s);
		if(v.empty())
			v.push_back(dflt);
		return v;
//		str_vec& v = props[s];
//		return v.empty() ? str_vec{dflt} : v;
	}

	str_set get_set(const str& s) const
	{
		str_vec v = get_vec(s);
		return {v.begin(), v.end()};
	}

	str_set get_set(const str& s, const str& dflt) const
	{
		str_set r = get_set(s);
		if(r.empty())
			r.insert(dflt);
		return r;
	}

	str_set get_wild_keys(const str& prefix) const
	{
		str_set s;
		for(auto&& prop: props)
			if(wild_match(prefix + "*", prop.first))
				s.insert(prop.first);
		return s;
	}

	bool extract_params(const message& msg
		, std::initializer_list<str*> args, bool report = true);

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
	void add_plugin(IrcBotPluginRPtr p);
	void add_monitor(IrcBotMonitor& m);
	void add_rpc_service(IrcBotRPCService& s);

	IrcBotRPCService* get_rpc_service(const str& name)
	{
		return services.find(name) == services.end() ? 0 : services[name];
	}
private:
	friend class IrcBotPluginHandle;
	IrcBotPluginRPtr get_plugin(const str& id)
	{
		for(auto& plugin: plugins)
			if(plugin->get_id() == id)
				return plugin.get();
		return IrcBotPluginRPtr(0);
	}
public:
	IrcBotPluginHandle get_plugin_handle(const str& id)
	{
		bug_fun();
		bug_var(id);
		return IrcBotPluginHandle(*this, id);
	}

	/**
	 * Add channel to the list of channels
	 * to be joined at startup.
	 */
	void add_channel(const str& c);


	IrcServer* get_irc_server();

public:
	str configfile; // current config file

	bool fc_say(const str& to, const str& text);

	bool fc_reply(const message& msg, const str& text);
	bool fc_reply_help(const message& msg, const str& text, const str& prefix = "");
	bool fc_reply_notice(const message& msg, const str& text);
	bool fc_reply_pm(const message& msg, const str& text);
	bool fc_reply_pm_help(const message& msg, const str& text, const str& prefix = "");
	bool fc_reply_pm_notice(const message& msg, const str& text);

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

IrcBotPluginHandle::IrcBotPluginHandle(IrcBot& bot, const str& id)
: bot(bot), id(id)
{
}

IrcBotPluginHandle::IrcBotPluginHandle(const IrcBotPluginHandle& handle)
: bot(handle.bot), id(handle.id), plugin(handle.plugin)
, plugin_load_time(handle.plugin_load_time)
{
}

IrcBotPluginHandle::operator void*() /*const*/
{
	ensure_plugin();
	return plugin;
}

IrcBotPluginHandle& IrcBotPluginHandle::operator=(const IrcBotPluginHandle& handle)
{
	id = handle.id;
	plugin = handle.plugin;
	plugin_load_time = handle.plugin_load_time;
	return *this;
}

void IrcBotPluginHandle::ensure_plugin()
{
	if(bot.get_plugin_load_time() > plugin_load_time)
	{
		plugin = bot.get_plugin(id);
		plugin_load_time = std::time(0);
	}
}

IrcBotPlugin& IrcBotPluginHandle::operator*()
{
	ensure_plugin();

	if(!plugin)
		LOG::E << "Bad IrcBotPluginHandle: " << this;
	return *plugin;
}

IrcBotPlugin* IrcBotPluginHandle::operator->()
{
	ensure_plugin();

	if(!plugin)
		LOG::E << "Bad IrcBotPluginHandle: " << this;
	return plugin;
}

}} // skivvy::ircbot

#endif // _SKIVVY_IRCBOT_H_
