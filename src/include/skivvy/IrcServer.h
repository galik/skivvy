//#pragma once
#ifndef _SKIVVY_REMOTE_IRC_SERVER_H_
#define _SKIVVY_REMOTE_IRC_SERVER_H_

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

//#include <sookee/socketstream.h>

#include <sookee/socketstream.h>
#include <sookee/ssl_socketstream.h>
#include <skivvy/message.h>

#include <sookee/types.h>

#include <memory>

namespace skivvy { namespace ircbot {

using namespace sookee;
using namespace sookee::types;

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
 *     // ... deal with incoming lines from the
 *     // IRC server here.
 * }
 * </pre>
 */
class IrcServer
{
public:
	virtual ~IrcServer() {}
	virtual bool send(const str& cmd) = 0;
	virtual bool send_unlogged(const str& cmd) = 0;

	/**
	 * Open a TCP/IP connection to the host on the given port
	 * that were passed as parameters.
	 *
	 * @host The hostname of the IRC server
	 * @port The port the IRC server is accepting requests on.
	 * @return false on failure.
	 */
	virtual bool connect(const str& host, long port = 6667) = 0;

	/**
	 * Initiate the dialogue between the client and the server
	 * by sending an (optionally blank) password.
	 */
	virtual bool pass(const str& pwd) = 0;

	/**
	 * Tell the server the nick you will be using. This may
	 * be rejected later so this call may need to be made again
	 * with an alternative nick.
	 */
	virtual bool nick(const str& n) = 0;

	/**
	 * Provide the user information for the given nick. If the nick is rejected
	 * this information will have to be sent again after a new nick has been sent
	 * with a call to bool nick().
	 *
	 * @param u User Name
	 * @param m User Mode
	 * @param r Real Name
	 */
	virtual bool user(const str& u, const str m, const str r) = 0;

	/**
	 * Join a specific channel.
	 *
	 * @channel The channel to join, eg. #stuff
	 * @return false on failure.
	 */
	virtual bool join(const str& channel, const str& key = "") = 0;

	/**
	 * Part (leave) a channel.
	 *
	 * @channel the channel to leave
	 * @message an optional leaving message
	 * @return false on failure.
	 */
	virtual bool part(const str& channel, const str& message = "") = 0;

	/**
	 * Send a PING message.
	 * @return false on failure.
	 */
	virtual bool ping(const str& info) = 0;

	/**
	 * Send a PONG message.
	 * @return false on failure.
	 */
	virtual bool pong(const str& info) = 0;

	/**
	 * Say something to either a channel or a specific user.
	 * This function facilitates "talking/chatting" on IRC.
	 *
	 * @to the channel or the user to direct this message to.
	 * @text The text of the message.
	 * @return false on failure.
	 */
	virtual bool say(const str& to, const str& text) = 0;

	// send cctp message
	virtual bool ctcp(const str& to, const str& text) = 0;

	/**
	 * Send a notice to either a channel or a specific user.
	 *
	 * @to the channel or the user to direct this message to.
	 * @text The text of the message.
	 * @return false on failure.
	 */
	virtual bool notice(const str& to, const str& text) = 0;

	virtual bool kick(const str_set& chans, const str_set& users, const str& comment) = 0;

	// non standard??
	virtual bool auth(const str& user, const str& pass) = 0;

	/**
	 * Emote something to either a channel or a specific user.
	 * This function facilitates "emoting" on IRC.
	 *
	 * @to the channel or the user to direct this message to.
	 * @text The text of the emotion.
	 * @return false on failure.
	 */
	virtual bool me(const str& to, const str& text) = 0;

	/**
	 * Open a one on one session with a specific user.
	 * @return false on failure.
	 */
	virtual bool query(const str& nick) = 0;

	/**
	 * Close client session.
	 * @return false on failure.
	 */
	virtual bool quit(const str& reason) = 0;

	virtual bool whois(const str_set& masks) = 0;

	/**
	 * Change mode settings for nick.
	 *
	 * @chan the channel the mode is set in
	 * @mode The mode flags.
	 * @nick the nick of the user to change mode flags for.
	 * @return false on failure.
	 */
	virtual bool mode(const str& chan, const str& mode, const str& nick) = 0;

	/**
	 * Change mode settings for nick. // CORRECT???
	 *
	 * @nick the nick of the user to change mode flags for.
	 * @mode The mode flags.
	 * @return false on failure.
	 */
	virtual bool mode(const str& nick, const str& mode) = 0;

	/**
	 * Reply is like say() but it deduces who to send the reply
	 * to based on the received message. This could either be
	 * the channel the message was sent from or a specific user
	 * from a PM.
	 *
	 * @return false on failure.
	 */
	virtual bool reply(const message& msg, const str& text) = 0;

	/**
	 * This function is like reply except that it always sends a PM (QUERY)
	 * to the sender of the message.
	 *
	 * @param msg The message from the person you wish to
	 * reply to using PM (QUERY).
	 */
	virtual bool reply_pm(const message& msg, const str& text) = 0;
	virtual bool reply_notice(const message& msg, const str& text) = 0;

	/**
	 * Get a line from the IRC server.
	 *
	 * @return false on failure.
	 */
	virtual bool receive(str& line) = 0;
};

using IrcServerUptr = std::unique_ptr<IrcServer>;

class BaseIrcServer
: public IrcServer
{
public:
	bool send(const str& cmd) override;
	bool pass(const str& pwd) override;
	bool nick(const str& n) override;
	bool user(const str& u, const str m, const str r) override;
	bool join(const str& channel, const str& key = "") override;
	bool part(const str& channel, const str& message = "") override;
	bool ping(const str& info) override;
	bool pong(const str& info) override;
	bool say(const str& to, const str& text) override;
	bool ctcp(const str& to, const str& text) override;
	bool notice(const str& to, const str& text) override;
	bool kick(const str_set& chans, const str_set& users, const str& comment) override;
	bool auth(const str& user, const str& pass) override;
	bool me(const str& to, const str& text) override;
	bool query(const str& nick) override;
	bool quit(const str& reason) override;
	bool whois(const str_set& masks) override;
	bool mode(const str& chan, const str& mode, const str& nick) override;
	bool mode(const str& nick, const str& mode) override;
	bool reply(const message& msg, const str& text) override;
	bool reply_pm(const message& msg, const str& text) override;
	bool reply_notice(const message& msg, const str& text) override;
};

template<typename SocketStream>
class BasicRemoteIrcServer
: public BaseIrcServer
{
private:
	std::mutex mtx_ss;
//	net::socketstream ss;
	SocketStream ss;

public:
	bool send_unlogged(const str& cmd)
	{
		lock_guard lock(mtx_ss);
		ss << cmd.substr(0, 510) << "\r\n" << std::flush;
		if(!ss)
			log("ERROR: send failed.");
		return (bool)ss;
	}

	bool connect(const str& host, long port)
	{
		ss.clear();
		ss.open(host, port);
		return (bool)ss;
	}

	bool receive(str& line)
	{
		return (bool)std::getline(ss, line);
	}
};

using RemoteIrcServer = BasicRemoteIrcServer<net::netstream>;
using RemoteSSLIrcServer = BasicRemoteIrcServer<net::ssl_socketstream>;

//class RemoteSSLIrcServer
//: public BaseIrcServer
//{
//private:
//	std::mutex mtx_ss;
//	net::ssl_socketstream ss;
//
//public:
//	bool send_unlogged(const str& cmd);
//	bool connect(const str& host, long port = 9999);
//	bool receive(str& line);
//};

class TestIrcServer
: public BaseIrcServer
{
private:
	std::mutex mtx_ifs;
	std::ifstream ifs;
	std::ofstream ofs;

public:
	bool send_unlogged(const str& cmd);
	bool connect(const str& host, long port = 6667);
	bool receive(str& line);
};

}} // skivvy::ircbot

#endif // _SKIVVY_REMOTE_IRC_SERVER_H_
