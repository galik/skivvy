#pragma once
#ifndef _SKIVVY_MESSAGE_H_
#define _SKIVVY_MESSAGE_H_

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

#include <skivvy/types.h>


namespace skivvy { namespace ircbot {

using namespace skivvy::types;

/**
 * Relevant components from incoming the messages sent
 * by the IRC server.
 */
struct message
{
	str line; // original message line
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
	friend std::istream& parsemsg(std::istream&& is, message& m);

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

// TODO: Sort this mess out
void bug_message(const std::string& K, const std::string& V, const message& msg);

#define BUG_MSG(m,M) do{ bug_message(#M, M, m); }while(false)

}} // skivvy::ircbot

#endif // _SKIVVY_MESSAGE_H_
