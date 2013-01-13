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

#include <skivvy/rpc.h>
#include <skivvy/logrep.h>
#include <skivvy/socketstream.h>
#include <skivvy/FloodController.h>

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

const str DATA_DIR = "data_dir";

/**
 * Relevant components from incoming the messages sent
 * by the IRC server.
 */
struct message
{
	str from; // ":Nick!user@network"
	str cmd; // "PRIVMSG"
	str params; // not same as user_params()
	str to; // "#oa-ictf" | Nick;
	str text; // "evening all";

	void clear()
	{
		from.clear();
		cmd.clear();
		params.clear();
		to.clear();
		text.clear();
	}

	/**
	 * Parse an incoming line from the IRC server and break it down into
	 * its relevant parts.
	 */
	friend std::istream& parsemsg(std::istream& is, message& m);

	/**
	 * Output the message parts for diagnostics.
	 */
	friend std::ostream& printmsg(std::ostream& os, const message& m);

	/**
	 * deserialize
	 */
	friend std::istream& operator>>(std::istream& is, message& m);

	/**
	 * serialize
	 */
	friend std::ostream& operator<<(std::ostream& os, const message& m);

	/**
	 * Extract the nick of the message sender from the surrounding
	 * server qualifier. Eg from "MyNick!~User@server.com it will"
	 * extract "MyNick" as the 'sender'.
	 */
	str get_nick() const; // MyNick
	str get_user() const; // ~User
	str get_host() const; // server.com
	str get_userhost() const; // ~User@server.com

	/**
	 * Extract the user command out from a channel/query message.
	 * That is the first word (command word) of the text line that the bot uses
	 * to test if it recognises it as a command, for example "!help".
	 */
	str get_user_cmd() const;

	/**
	 * Extract the parameters out from a channel/query message.
	 * That is everything after the first word (command word) of the text line that
	 * the bot uses as parameters to a command that it recognises.
	 */
	str get_user_params() const;

	/**
	 * Returns true if this message was sent from a channel
	 * rather than a private message.
	 */
	bool from_channel() const;

	/**
	 * Returns either the command sender, or the sender's channel.
	 * This is the correct person to sent replies to because
	 * it will correctly deal with QUERY sessions.
	 */
	str reply_to() const;
};

typedef std::set<message> message_set;
typedef message_set::iterator message_set_iter;
typedef message_set::const_iterator message_set_citer;

// Event to be triggered upon an incoming message
// For example when a task needs to be performed
// after the server has been queried for information
// these events can be triggered when that information
// asymmetrically arrives

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

/**
 * Class to manage the dialogue with a remote IRC server.
 * It provides functions to connect to, and communicate with
 * the server and other users on its network in various channels.
 *
 * Typically this class will be used like this:
 *
 * <pre>
 * RemoteIrcServer irc;
 *
 * user_info info;
 *
 * info.nick = "Nick";
 * info.user = "User;
 * info.mode = "0";
 * info.real = "Real Name";
 *
 * if(!irc.connect()) { log("Failed to connect:"); return; }
 * if(!irc.negotiate(info)) { log("Failed to negotiate:"); return; }
 *
 * str line;
 * while(irc.receive(line))
 * {
 * 		// ... deal with incoming lines from the
 * 		// IRC server here.
 * }
 * </pre>
 */
class RemoteIrcServer
{
private:
//	const str host;
//	const long port;

	std::mutex mtx_ss;
	net::socketstream ss;


public:

	bool send(const str& cmd);
	bool send_unlogged(const str& cmd);

	/**
	 * Construct a RemoteIrcServer object.
	 *
	 */
	RemoteIrcServer();

	/**
	 * Open a TCP/IP connection to the host on the given port
	 * that were passed as parameters.
	 *
	 * @host The hostname of the IRC server
	 * @port The port the IRC server is accepting requests on.
	 * @return false on failure.
	 */
	bool connect(const str& host, long port = 6667);

	/**
	 * Initiate the dialogue between the client and the server
	 * by sending an (optionally blank) password.
	 */
	bool pass(const str& pwd);

	/**
	 * Tell the server the nick you will be using. This may
	 * be rejected later so this call may need to be made again
	 * with an alternative nick.
	 */
	bool nick(const str& n);

	/**
	 * Provide the user information for the given nick. If the nick is rejected
	 * this information will have to be sent again after a new nick has been sent
	 * with a call to bool nick().
	 *
	 * @param u User Name
	 * @param m User Mode
	 * @param r Real Name
	 */
	bool user(const str& u, const str m, const str r);

	/**
	 * Join a specific channel.
	 *
	 * @channel The channel to join, eg. #stuff
	 * @return false on failure.
	 */
	bool join(const str& channel, const str& key = "");

	/**
	 * Part (leave) a channel.
	 *
	 * @channel the channel to leave
	 * @message an optional leaving message
	 * @return false on failure.
	 */
	bool part(const str& channel, const str& message = "");

	/**
	 * Send a PING message.
	 * @return false on failure.
	 */
	bool ping(const str& info);

	/**
	 * Send a PONG message.
	 * @return false on failure.
	 */
	bool pong(const str& info);

	/**
	 * Say something to either a channel or a specific user.
	 * This function facilitates "talking/chatting" on IRC.
	 *
	 * @to the channel or the user to direct this message to.
	 * @text The text of the message.
	 * @return false on failure.
	 */
	bool say(const str& to, const str& text);

	bool kick(const str_vec& chans, const str_vec&users, const str& comment);

	// non standard??
	bool auth(const str& user, const str& pass);

	/**
	 * Emote something to either a channel or a specific user.
	 * This function facilitates "emoting" on IRC.
	 *
	 * @to the channel or the user to direct this message to.
	 * @text The text of the emotion.
	 * @return false on failure.
	 */
	bool me(const str& to, const str& text);

	/**
	 * Open a one on one session with a specific user.
	 * @return false on failure.
	 */
	bool query(const str& nick);

	/**
	 * Close client session.
	 * @return false on failure.
	 */
	bool quit(const str& reason);

	bool whois(const str& nick);

	/**
	 * Change mode settings for nick.
	 *
	 * @chan the channel the mode is set in
	 * @mode The mode flags.
	 * @nick the nick of the user to change mode flags for.
	 * @return false on failure.
	 */
	bool mode(const str& chan, const str& mode, const str& nick);

	/**
	 * Change mode settings for nick. // CORRECT???
	 *
	 * @nick the nick of the user to change mode flags for.
	 * @mode The mode flags.
	 * @return false on failure.
	 */
	bool mode(const str& nick, const str& mode);

	/**
	 * Reply is like say() but it deduces who to send the reply
	 * to based on the received message. This could either be
	 * the channel the message was sent from or a specific user
	 * from a PM.
	 *
	 * @return false on failure.
	 */
	bool reply(const message& msg, const str& text);

	/**
	 * This function is like reply except that it always sends a PM (QUERY)
	 * to the sender of the message.
	 *
	 * @param msg The message from the person you wish to
	 * reply to using PM (QUERY).
	 */
	bool reply_pm(const message& msg, const str& text);

	/**
	 * Get a line from the IRC server.
	 *
	 * @return false on failure.
	 */
	bool receive(str& line);
};

// ==================================================
// Bot Utils
// ==================================================

/**
 * multi-user aware random callback timer
 */
class RandomTimer
{
private:
	typedef std::set<const void*> user_set;

	user_set users;
	std::mutex mtx;
	std::future<void> fut;
	siz mindelay; // = 1;
	siz maxdelay; // = 120;

	std::function<void(const void*)> cb;

	void timer();

public:

	RandomTimer(std::function<void(const void*)> cb);
	RandomTimer(const RandomTimer& rt);
	~RandomTimer();

	void set_mindelay(size_t seconds);
	void set_maxdelay(size_t seconds);

	/**
	 * @return false if already on for this user else true.
	 */
	bool on(const void* user);

	/**
	 * @return false if already off for this user else true.
	 */
	bool off(const void* user);

	/**
	 * @return false if already off for this user else true.
	 */
	bool turn_off();
};

/**
 * multi-user aware random callback timer
 */
class MessageTimer
{
private:
	struct comp
	{
	  bool operator()(const message& lhs, const message& rhs) const
	  {return lhs.reply_to() < rhs.reply_to();}
	};

	typedef std::set<message, comp> message_set;

	message_set messages;
	std::mutex mtx;
	std::future<void> fut;
	siz mindelay; // = 1;
	siz maxdelay; // = 120;

	std::function<void(const message&)> cb;

	void timer();

public:

	MessageTimer(std::function<void(const message&)> cb);
	MessageTimer(const MessageTimer& mt);
	~MessageTimer();

	void set_mindelay(size_t seconds);
	void set_maxdelay(size_t seconds);

	/**
	 * @return false if already on for this user else true.
	 */
	bool on(const message& m);

	/**
	 * @return false if already off for this user else true.
	 */
	bool off(const message& m);

	/**
	 * @return false if already off for this user else true.
	 */
	bool turn_off();
};

///**
// * Producer/Consumer priority que for dispatching
// * event at a configurable minimal interval.
// */
//class FloodController
//{
//private:
//	struct dispatch
//	{
//		size_t priority;
//		//std::thread::id id;
//		std::function<bool()> func;
//		bool operator<(const dispatch& d) const { return priority > d.priority; }
//		dispatch(size_t priority, std::function<bool()> func): priority(priority), func(func) {}
//	};
//	typedef std::priority_queue<dispatch> dispatch_que;
//
//	std::mutex mtx;
//	std::future<void> fut;
//	dispatch_que que;
//	bool dispatching;
//	size_t priority; // current priority entry point
//	size_t max;
//
//
//	typedef std::map<const std::thread::id, dispatch_que> mtpq_map;
//	typedef std::pair<const std::thread::id, dispatch_que> mtpq_pair;
//	typedef mtpq_map::iterator mtpq_map_iter;
//	typedef mtpq_map::const_iterator mtpq_map_citer;
//
//	std::map<std::thread::id, siz> mt_priority;
//	mtpq_map mt_que;
////	std::map<std::thread::id, std::priority_queue<dispatch>>::iterator mt_que_iter;
//	siz robin;
//
//	size_t get_priority(const std::thread::id& id);
//	void dispatcher();
//
//public:
//	FloodController();
//	~FloodController();
//
//	bool send(std::function<bool()> func);
//	void start();
//	void stop();
//};
//
//class FloodController2
//{
//private:
//	typedef std::queue<std::function<bool()>> dispatch_que;
//	typedef std::map<str, dispatch_que> dispatch_map;
//	typedef std::pair<const str, dispatch_que> dispatch_pair;
//
//	typedef str_set dispatch_set;
//
//	std::mutex mtx;
//	//dispatch_set s; // channels
//	dispatch_map m; // channel -> dispatch_que
//	str c; // current channel being dispatched
//
//	std::future<void> fut;
//	bool dispatching = false;
//
//	bool find_next(str& c);
//	void dispatcher();
//
//public:
//	FloodController2();
//	~FloodController2();
//
//	bool send(const str& channel, std::function<bool()> func);
//	void start();
//	void stop();
//};

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
	typedef std::vector<str> command_list;
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

typedef std::shared_ptr<IrcBotPlugin> IrcBotPluginPtr;

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

	/*
	 * compatability struct
	 */
//	struct oldaction
//	{
//		str cmd;
//		str help;
//		std::function<void(const message& msg)> func;
//	};

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
	//void add(const oldaction& a);

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
	virtual std::vector<str> list() const; // final

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
IrcBotPluginPtr skivvy_ircbot_factory(IrcBot& bot) \
{ \
	return IrcBotPluginPtr(new name(bot)); \
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
	IrcBotPluginPtr operator()(const str& file, IrcBot& bot);
};

template<typename Plugin>
class IrcBotPluginHandle
{
	typedef std::auto_ptr<Plugin> PluginPtr;
	IrcBot& bot;
	const str name;
	PluginPtr plugin;// = 0;
	time_t plugin_load_time = 0;

	static Plugin null_plugin;

public:
	IrcBotPluginHandle(IrcBot& bot)
	: bot(bot)
	{
	}

	IrcBotPluginHandle(IrcBotPluginHandle<Plugin>&& handle)
	: bot(handle.bot), name(handle.name), plugin(handle.plugin)
	, plugin_load_time(handle.plugin_load_time)
	{
	}

	IrcBotPluginHandle(IrcBot& bot, const str& name)
	: bot(bot), name(name)
	{
	}

	void ensure_plugin();

	Plugin& operator*()
	{
		ensure_plugin();

		if(!plugin)
		{
			log("ERROR: Bad IrcBotPluginHandle: " << this);
			return *null_plugin;
		}
		return *plugin;
	}

	Plugin* operator->()
	{
		ensure_plugin();

		if(!plugin)
		{
			log("ERROR: Bad IrcBotPluginHandle: " << this);
			return null_plugin;
		}
		return plugin;
	}
};

// ==================================================
// The core Bot
// ==================================================

class IrcBot _final_
//: private IrcBotPlugin
{
public:

//	typedef std::multimap<str, str> property_mmap;
	typedef std::map<str, str_vec> property_map;
	typedef std::pair<const str, str_vec> property_pair;
	typedef property_map::iterator property_iter;
	typedef property_map::const_iterator property_citer;

	typedef std::pair<property_iter, property_iter> property_iter_pair;
	typedef std::pair<property_iter, property_iter> property_range;

//	typedef std::set<IrcBotPluginPtr> plugin_set;
	typedef std::vector<IrcBotPluginPtr> plugin_vec;
	typedef plugin_vec::iterator plugin_vec_iter;
	typedef plugin_vec::const_iterator plugin_vec_citer;
	typedef std::set<IrcBotMonitor*> monitor_set;
	typedef std::set<IrcBotChoiceListener*> listener_set;
	typedef std::map<str, IrcBotRPCService*> service_map;
	typedef std::set<str> channel_set;
//	typedef std::set<str> ban_set;
	typedef std::map<str, IrcBotPluginPtr> command_map;
	typedef std::pair<str, IrcBotPluginPtr> command_map_pair;

	typedef std::map<const str, str_set> nicks_map;
	typedef std::pair<const str, str_set> nicks_map_pair;
	typedef std::map<const str, str_set> str_str_set_map;
	typedef nicks_map::iterator nicks_iter;
	typedef nicks_map::const_iterator nick_citer;

private:
	RemoteIrcServer irc;
	FloodController fc;

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

	str host;
	siz port;

	str locate_file(const str& name);

	time_t config_loaded;
	time_t plugin_loaded;

	void load_plugins();

	bool has_access(const str& cmd, const str& chan);

	bool allow_cmd_access(const str& cmd, const message& msg);

public:
	bool restart = false;

//	time_t when;
//	siz timeout = 30; // 30 seconds
//	std::function<void(const message&)> func;

	void add_msgevent(const str& msg, const std::function<void(const message&)>& func, siz timeout = 30)
	{
		msgevent me;
		me.timeout = timeout;
		me.func = func;
		lock_guard lock(msgevent_mtx);
		msgevents.insert(msgevent_pair(msg, me));
	}

	void add_msgevent(const str& msg, const msgevent& me)
	{
		lock_guard lock(msgevent_mtx);
		msgevents.insert(msgevent_pair(msg, me));
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
	bool ereg_match(const str& r, const str& s); // extended regex
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

//	template<typename Iter>
//	void set_all(const str& s, Iter begin, Iter end)
//	{
//		property_iter_pair i = props.equal_range(s);
//		props.erase(i.first, i.second);
//		for(; begin != end; ++begin)
//		{
//			std::ostringstream oss;
//			oss << begin->second;
//			props.insert(property_pair(begin->first, oss.str()));
//		}
//	}

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
	 * to the value of the property variable data_dir:;
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

	void del_plugin(const str& name);
	void add_plugin(IrcBotPluginPtr p);
	void add_monitor(IrcBotMonitor& m);
	void add_rpc_service(IrcBotRPCService& s);

	IrcBotRPCService* get_rpc_service(const str& name)
	{
		return services.find(name) == services.end() ? 0 : services[name];
	}

	IrcBotPluginPtr get_plugin(const str& name)
	{
		for(IrcBotPluginPtr plugin: plugins)
			if(plugin->get_name() == name)
				return plugin;
		return IrcBotPluginPtr(0);
	}

	template<typename Plugin>
	std::shared_ptr<Plugin> get_typed_plugin(const str& name)
	{
		for(IrcBotPluginPtr plugin: plugins)
			if(plugin->get_name() == name)
				return std::shared_ptr<Plugin>(dynamic_cast<Plugin*>(plugin.get()));
		return std::shared_ptr<Plugin>(0);
	}

//	template<typename Plugin>
//	IrcBotPluginHandle<Plugin> get_plugin_handle(const str& name)
//	{
//		return IrcBotPluginHandle<Plugin>(*this, name);
//	}

	template<typename Plugin>
	IrcBotPluginHandle<Plugin> get_plugin_handle(const str& name)
	{
		return IrcBotPluginHandle<Plugin>(*this, name);
	}

	/**
	 * Add channel to the list of channels
	 * to be joined at startup.
	 */
	void add_channel(const str& c);


	RemoteIrcServer& get_irc_server();

	bool has_plugin(const str& name, const str& version = "");

	// Utility

	// flood control

public:

	str configfile; // current config file

	bool fc_reply(const message& msg, const str& text);
	bool fc_reply_help(const message& msg, const str& text, const str& prefix = "");
	bool fc_reply_pm(const message& msg, const str& text);
	bool fc_reply_pm_help(const message& msg, const str& text, const str& prefix = "");

	bool cmd_error(const message& msg, const str& text, bool rv = false);
	bool cmd_error_pm(const message& msg, const str& text, bool rv = false);

	/**
	 * Return a list of nicks registered to a given channel
	 */
	//str_set get_nicks(const str& channel);

	// INTERFACE: IrcBotPlugin

	virtual str get_name() const;
	virtual str get_version() const;

	virtual bool init(const str& config_file = "");
	virtual str_vec list() const;
	virtual void execute(const str& cmd, const message& msg);
	virtual str help(const str& cmd) const;
	virtual void exit();
};

template<typename Plugin>
void IrcBotPluginHandle<Plugin>::ensure_plugin()
{
	if(bot.get_plugin_load_time() > plugin_load_time)
	{
		IrcBotPluginPtr ptr = bot.get_plugin(name);
		plugin.reset(dynamic_cast<Plugin*>(ptr.get()));
		plugin_load_time = std::time(0);
	}
}

//bool wildmatch(str_citer mi, const str_citer me
//	, str_citer ii, const str_citer ie);
//
//bool wildmatch(const str& mask, const str& id);

}} // namespace skivvy { namespace ircbot {

#endif // _SKIVVY_IRCBOT_H_
