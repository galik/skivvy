#pragma once
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

#include <sookee/socketstream.h>

#include <skivvy/socketstream.h>
#include <skivvy/message.h>

namespace skivvy { namespace ircbot {

using namespace skivvy::types;

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

	/**
	 * Send a notice to either a channel or a specific user.
	 *
	 * @to the channel or the user to direct this message to.
	 * @text The text of the message.
	 * @return false on failure.
	 */
	virtual bool notice(const str& to, const str& text) = 0;

	virtual bool kick(const str_vec& chans, const str_vec&users, const str& comment) = 0;

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

	virtual bool whois(const str& nick) = 0;

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

	/**
	 * Get a line from the IRC server.
	 *
	 * @return false on failure.
	 */
	virtual bool receive(str& line) = 0;
};

class BaseIrcServer
: public IrcServer
{
public:
	bool send(const str& cmd);
	bool pass(const str& pwd);
	bool nick(const str& n);
	bool user(const str& u, const str m, const str r);
	bool join(const str& channel, const str& key = "");
	bool part(const str& channel, const str& message = "");
	bool ping(const str& info);
	bool pong(const str& info);
	bool say(const str& to, const str& text);
	bool notice(const str& to, const str& text);
	bool kick(const str_vec& chans, const str_vec&users, const str& comment);
	bool auth(const str& user, const str& pass);
	bool me(const str& to, const str& text);
	bool query(const str& nick);
	bool quit(const str& reason);
	bool whois(const str& nick);
	bool mode(const str& chan, const str& mode, const str& nick);
	bool mode(const str& nick, const str& mode);
	bool reply(const message& msg, const str& text);
	bool reply_pm(const message& msg, const str& text);
};

class RemoteIrcServer
: public BaseIrcServer
{
private:
	std::mutex mtx_ss;
	net::socketstream ss;

public:
	bool send_unlogged(const str& cmd);
	bool connect(const str& host, long port = 6667);
	bool receive(str& line);
};

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
