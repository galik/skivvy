#pragma once
#ifndef _SKIVVY_MESSAGE_H__
#define _SKIVVY_MESSAGE_H__

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

//#include <sookee/stl.h>
#include <sookee/str.h>
#include <sookee/types.h>
#include <sookee/bug.h>


namespace skivvy { namespace ircbot {

using namespace sookee::types;

class message
{
public:

	/**
	 * Returns true if this message was sent from a channel
	 * rather than a private message.
	 */
	bool from_channel() const;

	/**
	 * The first parameter (if present)
	 * @return usually the channl or the nick (PM)
	 */
	str get_to() const;

	/**
	 * Returns either the command sender, or the sender's channel.
	 * This is the correct person to sent replies to because
	 * it will correctly deal with QUERY sessions.
	 */
	str reply_to() const;

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
	 * Extract the nick of the message sender from the surrounding
	 * server qualifier. Eg from "MyNick!~User@server.com it will"
	 * extract "MyNick" as the 'sender'.
	 */
	str get_nick() const; // MyNick
//	str get_user() const; // ~User
//	str get_host() const; // server.com
	str get_userhost() const; // ~User@server.com

	/**
	 * Return the relevant channel field (if any) for the
	 * particular command this message contains.
	 * @return
	 */
	str get_chan() const;

	// TODO: get_subject() & get_object() ?

	// The Augmented BNF representation for this is:
	//
	// message    =  [ ":" prefix SPACE ] command [ params ] crlf
	// prefix     =  servername / ( nickname [ [ "!" user ] "@" host ] )
	// command    =  1*letter / 3digit
	// params     =  *14( SPACE middle ) [ SPACE ":" trailing ]
	//            =/ 14( SPACE middle ) [ SPACE [ ":" ] trailing ]
	//
	// nospcrlfcl =  %x01-09 / %x0B-0C / %x0E-1F / %x21-39 / %x3B-FF
	//                 ; any octet except NUL, CR, LF, " " and ":"
	// middle     =  nospcrlfcl *( ":" / nospcrlfcl )
	// trailing   =  *( ":" / " " / nospcrlfcl )
	//
	// SPACE      =  %x20        ; space character
	// crlf       =  %x0D %x0A   ; "carriage return" "linefeed"
	//
	//  NOTES:
	//     1) After extracting the parameter list, all parameters are equal
	//        whether matched by <middle> or <trailing>. <trailing> is just a
	//        syntactic trick to allow SPACE within the parameter.
	//
	//     2) The NUL (%x00) character is not special in message framing, and
	//        basically could end up inside a parameter, but it would cause
	//        extra complexities in normal C string handling. Therefore, NUL
	//        is not allowed within messages.
	//
	//  Most protocol messages specify additional semantics and syntax for
	//  the extracted parameter strings dictated by their position in the
	//  list.  For example, many server commands will assume that the first
	//  parameter after the command is the list of targets, which can be
	//  described with:
	//
	// target     =  nickname / server
	// msgtarget  =  msgto *( "," msgto )
	// msgto      =  channel / ( user [ "%" host ] "@" servername )
	// msgto      =/ ( user "%" host ) / targetmask
	// msgto      =/ nickname / ( nickname "!" user "@" host )
	// channel    =  ( "#" / "+" / ( "!" channelid ) / "&" ) chanstring
	//               [ ":" chanstring ]
	// servername =  hostname
	// host       =  hostname / hostaddr
	// hostname   =  shortname *( "." shortname )
	// shortname  =  ( letter / digit ) *( letter / digit / "-" )
	//               *( letter / digit )
	//                 ; as specified in RFC 1123 [HNAME]
	// hostaddr   =  ip4addr / ip6addr
	// ip4addr    =  1*3digit "." 1*3digit "." 1*3digit "." 1*3digit
	// ip6addr    =  1*hexdigit 7( ":" 1*hexdigit )
	// ip6addr    =/ "0:0:0:0:0:" ( "0" / "FFFF" ) ":" ip4addr
	// nickname   =  ( letter / special ) *8( letter / digit / special / "-" )
	// targetmask =  ( "$" / "#" ) mask
	//                 ; see details on allowed masks in section 3.3.1
	// chanstring =  %x01-07 / %x08-09 / %x0B-0C / %x0E-1F / %x21-2B
	// chanstring =/ %x2D-39 / %x3B-FF
	//                 ; any octet except NUL, BELL, CR, LF, " ", "," and ":"
	// channelid  = 5( %x41-5A / digit )   ; 5( A-Z / 0-9 )
	// user       =  1*( %x01-09 / %x0B-0C / %x0E-1F / %x21-3F / %x41-FF )
	//                 ; any octet except NUL, CR, LF, " " and "@"
	// key        =  1*23( %x01-05 / %x07-08 / %x0C / %x0E-1F / %x21-7F )
	//                 ; any 7-bit US_ASCII character,
	//                 ; except NUL, CR, LF, FF, h/v TABs, and " "
	// letter     =  %x41-5A / %x61-7A       ; A-Z / a-z
	// digit      =  %x30-39                 ; 0-9
	// hexdigit   =  digit / "A" / "B" / "C" / "D" / "E" / "F"
	// special    =  %x5B-60 / %x7B-7D
	//                  ; "[", "]", "\", "`", "_", "^", "{", "|", "}"
	//
	// NOTES:
	//     1) The <hostaddr> syntax is given here for the sole purpose of
	//        indicating the format to follow for IP addresses.  This
	//        reflects the fact that the only available implementations of
	//        this protocol uses TCP/IP as underlying network protocol but is
	//        not meant to prevent other protocols to be used.
	//
	//     2) <hostname> has a maximum length of 63 characters.  This is a
	//        limitation of the protocol as internet hostnames (in
	//        particular) can be longer.  Such restriction is necessary
	//        because IRC messages are limited to 512 characters in length.
	//        Clients connecting from a host which name is longer than 63
	//        characters are registered using the host (numeric) address
	//        instead of the host name.
	//
	//     3) Some parameters used in the following sections of this
	//        documents are not defined here as there is nothing specific
	//        about them besides the name that is used for convenience.
	//        These parameters follow the general syntax defined for
	//        <params>.

private:
	static const str nospcrlfcl; // any octet except NUL, CR, LF, " " and ":"
	static const str nospcrlf; // (":" / nospcrlfcl)
	static const str nocrlf; // (" " / ":" / nospcrlfcl)
	static const str nospcrlfat; // any octet except NUL, CR, LF, " " and "@"
	static const str bnf_special; // ?

	siss& gettrailing(siss& is, str& trailing) const;

	siss& getmiddle(siss& is, str& middle) const;

	siss& getparams(siss& is, str_vec& middles, str& trailing) const;

	siss& getparams(siss&& is, str_vec& middles, str& trailing) const
	{
		return getparams(is, middles, trailing);
	}

public:
	// message    =  [ ":" prefix SPACE ] command [ params ] crlf
	str line; // original message line
	std::time_t when; // arrival time

	str prefix;
	str command;
	str params;

	message(): when(0) {}

	void clear()
	{
		line.clear();
		when = 0;
		prefix.clear();
		command.clear();
		params.clear();
	}

	str get_servername() const;

	bool is_nick(const str& nick) const;

	str allow_nick(const str& nick) const;

	str get_nickname() const;

	bool is_user(const str& user) const;

	str allow_user(const str& user) const;

	str get_user() const;

	str get_host() const;

	bool get_params(str_vec& middles, str& trailing) const;

	str_vec get_params() const;

	str_vec get_middles() const;

	str get_trailing() const;

	bool parse(const str& line);

	friend bool parsemsg(const str& line, message& msg)
	{
		return msg.parse(line);
	}


	/**
	 * deserialize
	 */
	friend std::istream& operator>>(std::istream& is, message& m);

	/**
	 * serialize
	 */
	friend std::ostream& operator<<(std::ostream& os, const message& m);


	friend std::ostream& printmsg(std::ostream& os, const message& m);
};

typedef std::set<message> message_set;
typedef message_set::iterator message_set_iter;
typedef message_set::const_iterator message_set_citer;

// TODO: Sort this mess out
//void bug_message_cp(const std::string& K, const std::string& V, const message_cp& msg);
void bug_message(const std::string& K, const std::string& V, const message& msg);
#ifdef DEBUG
#define BUG_MSG(m,M) do{ bug_message(#M, M, m); }while(false)
#else
#define BUG_MSG(m,M)
#endif
}} // skivvy::ircbot

#endif // _SKIVVY_MESSAGE_H__
