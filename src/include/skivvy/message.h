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

#include <sookee/socketstream.h>

#include <sookee/stl.h>

#include <skivvy/types.h>


namespace skivvy { namespace ircbot {

using namespace skivvy::types;

/**
 * Relevant components from incoming the messages sent
 * by the IRC server.
 */
struct message_cp
{
	str line_cp; // original message line
	str from_cp; // ":Nick!user@network"
	str cmd_cp; // "PRIVMSG"
	str params_cp; // not same as user_params()
	str to_cp; // "#oa-ictf" | Nick;
	str text_cp; // "evening all";

	void clear()
	{
		from_cp.clear();
		cmd_cp.clear();
		params_cp.clear();
		to_cp.clear();
		text_cp.clear();
	}

	/**
	 * Parse an incoming line from the IRC server and break it down into
	 * its relevant parts.
	 */
	friend std::istream& parsemsg_cp(std::istream& is, message_cp& m);
	friend std::istream& parsemsg_cp(std::istream&& is, message_cp& m);

	/**
	 * Output the message parts for diagnostics.
	 */
	friend std::ostream& printmsg_cp(std::ostream& os, const message_cp& m);

	/**
	 * deserialize
	 */
	friend std::istream& operator>>(std::istream& is, message_cp& m);

	/**
	 * serialize
	 */
	friend std::ostream& operator<<(std::ostream& os, const message_cp& m);

	/**
	 * Extract the nick of the message sender from the surrounding
	 * server qualifier. Eg from "MyNick!~User@server.com it will"
	 * extract "MyNick" as the 'sender'.
	 */
	str get_nick_cp() const; // MyNick
	str get_user_cp() const; // ~User
	str get_host_cp() const; // server.com
	str get_userhost_cp() const; // ~User@server.com

	/**
	 * Extract the user command out from a channel/query message.
	 * That is the first word (command word) of the text line that the bot uses
	 * to test if it recognises it as a command, for example "!help".
	 */
	str get_user_cmd_cp() const;

	/**
	 * Extract the parameters out from a channel/query message.
	 * That is everything after the first word (command word) of the text line that
	 * the bot uses as parameters to a command that it recognises.
	 */
	str get_user_params_cp() const;

	/**
	 * Returns true if this message was sent from a channel
	 * rather than a private message.
	 */
	bool from_channel_cp() const;

	/**
	 * Returns either the command sender, or the sender's channel.
	 * This is the correct person to sent replies to because
	 * it will correctly deal with QUERY sessions.
	 */
	str reply_to_cp() const;
};

class message
: public message_cp
{
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
	static const str bnf_special; // any octet except NUL, CR, LF, " " and "@"

	siss& gettrailing(siss& is, str& trailing)
	{
		// trailing: *(":" / " " / nospcrlfcl)
		// trailing: ("\0"|"\n"|"\r")^*

		char c;
		trailing.clear();
		while(is && nocrlf.find(c) == str::npos && is.get(c))
			trailing.append(1, c);
		if(is.eof())
			is.clear();
		return is;
	}

	siss& getmiddle(siss& is, str& middle)
	{
		// middle: nospcrlfcl *(":" / nospcrlfcl)
		// middle: ("\0"|"\n"|"\r"|" "|":")^ ("\0"|"\n"|"\r"|" ")^*

		if(is && soo::find(nospcrlfcl, is.peek()) != nospcrlfcl.cend())
			is.setstate(std::ios::failbit);
		else
		{
			middle = is.get();
			while(is && soo::find(nospcrlf, is.peek()) == nospcrlf.end())
				middle += is.get();
		}
		return is;
	}

	siss& getparams(siss& is, str_vec& middles, str& trailing)
	{
		// params  :   (SPACE middle){0,14} [SPACE ":" trailing]
		//         : | (SPACE middle){14}   [SPACE [ ":" ] trailing]
		// middle  : nospcrlfcl *( ":" / nospcrlfcl )
		// trailing: *( ":" / " " / nospcrlfcl )

		middles.clear();
		trailing.clear();

		char c;
		siz n = 0;

		while(is && n < 14 && is.peek() == ' ' && is.get(c))
		{
			if(is.peek() == ':' && is.get(c))
				return gettrailing(is, trailing);

			str middle;
			if(getmiddle(is, middle) && ++n)
				middles.push_back(middle);
		}

		if(n == 14 && is.peek() == ' ' && is.get(c))
			if(is.peek() != ':' || is.get(c))
				return gettrailing(is, trailing);
		return is;
	}

	siss& getparams(siss&& is, str_vec& middles, str& trailing)
	{
		return getparams(is, middles, trailing);
	}

public:
	// message    =  [ ":" prefix SPACE ] command [ params ] crlf
	str prefix;
	str command; // TODO: Change this to command to refactor changes throughout
	str params;

	str get_servername()
	{
		// prefix: servername | (nickname [[ "!" user ] "@" host ])
		if(prefix.find('!') != str::npos || prefix.find('@') != str::npos)
			return "";
		return prefix;
	}

	bool is_nick(const str& nick)
	{
		if(nick.empty())
			return false;
		if(!isalpha(nick[0]) && soo::find(bnf_special, nick[0]) == bnf_special.cend())
			return false;
		for(char c: nick)
			if(!isalnum(c) && soo::find(bnf_special, c) == bnf_special.cend())
				return false;
		return true;
	}

	str allow_nick(const str& nick)
	{
		if(!is_nick(nick))
			return "";
		return nick;
	}

	str get_nickname()
	{
		// prefix: servername | (nickname [[ "!" user ] "@" host ])
		// nickname   =  ( letter / special ) *8( letter / digit / special / "-" )
		// letter     =  %x41-5A / %x61-7A       ; A-Z / a-z
		// digit      =  %x30-39                 ; 0-9
		// special    =  %x5B-60 / %x7B-7D
		//                  ; "[", "]", "\", "`", "_", "^", "{", "|", "}"
		str::size_type pos;
		if((pos = prefix.find('!')) != str::npos)
			return allow_nick(prefix.substr(0, pos));
		if((pos = prefix.find('@')) != str::npos)
			return allow_nick(prefix.substr(0, pos));
		return allow_nick(prefix);
	}

	bool is_user(const str& user)
	{
		for(char c: user)
			if(soo::find(nospcrlfat, c) != nospcrlfat.cend())
				return false;
		return true;
	}

	str allow_user(const str& user)
	{
		if(is_user(user))
			return user;
		return "";
	}

	str get_user()
	{
		// prefix: servername | (nickname [[ "!" user ] "@" host ])
		str::size_type beg = 0, end;
		if((end = prefix.find('@')) != str::npos)
			if((beg = prefix.find('!')) < end)
				return allow_user(prefix.substr(beg + 1, end - (beg + 1)));
		return "";
	}

	str get_host()
	{
		// prefix: servername | (nickname [[ "!" user ] "@" host ])
		str::size_type pos;
		if((pos = prefix.find('@')) != str::npos && pos < prefix.size() - 1)
			return prefix.substr(pos + 1);
		return "";
	}

	bool get_params(str_vec& middles, str& trailing)
	{
		return getparams(siss(params), middles, trailing);
	}

	str_vec get_params()
	{
		// treat all params equally (middles and trailing)
		str trailing;
		str_vec middles;
		if(get_params(middles, trailing))
			middles.push_back(trailing);
		return middles;
	}

	str_vec get_middles()
	{
		str trailing;
		str_vec middles;
		get_params(middles, trailing);
		return middles;
	}

	str get_trailing()
	{
		str trailing;
		str_vec middles;
		get_params(middles, trailing);
		return trailing;
	}

	std::istream& parse(std::istream& is)
	{
		// MUST handle both
		// [":" prefix SPACE] command [params] cr
		// [":" prefix SPACE] command [params] crlf
		if(char c = is.peek() == ':') // optional prefix
			is.get(c) >> prefix;
		return std::getline(is >> command, params, '\r') >> std::ws;
	}

	bool parse(std::istream&& is) { return parse(is); }
	bool parse(const str& line) { return parse(std::istringstream(line)); }
	friend std::istream& operator>>(std::istream& is, message& msg) { return msg.parse(is); }
};

// NUL, CR, LF, " " and ":"
typedef std::set<message> message_set;
typedef message_set::iterator message_set_iter;
typedef message_set::const_iterator message_set_citer;

// TODO: Sort this mess out
void bug_message_cp(const std::string& K, const std::string& V, const message_cp& msg);

#define BUG_MSG_CP(m,M) do{ bug_message_cp(#M, M, m); }while(false)

}} // skivvy::ircbot

#endif // _SKIVVY_MESSAGE_H__
