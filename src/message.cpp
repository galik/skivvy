/*
 * ircbot.cpp
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

#include <skivvy/message.h>

#include <hol/small_types.h>
#include <hol/string_utils.h>
#include <skivvy/store.h>
#include <skivvy/logrep.h>

#define CSTRING(s) s,sizeof(s)
#define STRC(c) str(1, c)

namespace skivvy { namespace ircbot {

using namespace hol::small_types::ios;
using namespace hol::small_types::basic;
using namespace hol::small_types::string_containers;
using namespace skivvy::utils;

std::istream& operator>>(std::istream& is, message& m)
{
	str o;
	if(!getobject(is, o))
		return is;
	if(!parsemsg(unescaped(o), m))
		is.setstate(std::ios::failbit); // protocol error
	return is;
}

std::ostream& operator<<(std::ostream& os, const message& m)
{
	return os << '{' << escaped(m.line) << '}';
}

bool message::from_channel() const
{
	str_vec params = get_params();
	return !params.empty() && !params[0].empty() && params[0][0] == '#';
}

str message::get_to() const
{
	str_vec params = get_params();
	if(!params.empty())
		return params[0];
	return "";
}

str message::reply_to() const
{
	if(!from_channel())
		return get_nickname();
	return get_to();
}

str message::get_user_cmd() const
{
	str cmd;
	sgl(siss(get_trailing()), cmd, ' ');
	return hol::trim_mute(cmd);
}

str message::get_user_params() const
{
	str params;
	siss iss(get_trailing());
	sgl(iss, params, ' ');
	if(!sgl(iss, params))
		return "";
	return hol::trim_mute(params);
}

str message::get_nick() const
{
	return get_nickname(); // alias
}

str message::get_userhost() const
{
	return prefix.substr(prefix.find("!") + 1);
}

static const std::vector<str_set> chan_params =
{
	{"PRIVMSG", "JOIN", "MODE", "KICK", "PART", "TOPIC"}
	, {"332", "333", "366", "404", "474", "482", "INVITE"}
	, {"353", "441"}
};

static const str chan_start = "#&+!";

str message::get_chan() const
{
	str_vec params = get_params();

	for(siz i = 0; i < chan_params.size(); ++i)
		if(chan_params[i].count(command) && params.size() > i)
			if(!params[i].empty() && std::count(chan_start.cbegin(), chan_start.cend(), params[i][0]))
//			if(!params[i].empty() && soo::find(chan_start, params[i][0]) != chan_start.cend())
				return params[i];
	return "";
}

std::ostream& printmsg(std::ostream& os, const message& m)
{
	os << "//                  line: " << m.line << '\n';
	os << "//                prefix: " << m.prefix << '\n';
	os << "//               command: " << m.command << '\n';
	os << "//                params: " << m.params << '\n';
	os << "// get_servername()     : " << m.get_servername() << '\n';
	os << "// get_nickname()       : " << m.get_nickname() << '\n';
	os << "// get_user()           : " << m.get_user() << '\n';
	os << "// get_host()           : " << m.get_host() << '\n';
	os << "// get_userhost()       : " << m.get_userhost() << '\n';
	os << "// get_to()             : " << m.get_to() << '\n';
	for(const str& param: m.get_params())
		os << "// param                : " << param << '\n';
	for(const str& middle: m.get_middles())
		os << "// middle               : " << middle << '\n';
	os << "// get_trailing()       : " << m.get_trailing() << '\n';
	os << "// get_nick()           : " << m.get_nick() << '\n';
	os << "// get_chan()           : " << m.get_chan() << '\n';
	os << "// get_user_cmd()       : " << m.get_user_cmd() << '\n';
	os << "// get_user_params()    : " << m.get_user_params() << '\n';
	return os << std::flush;
}

void bug_message(const std::string& K, const std::string& V, const message& msg)
{
	(void) K;
	(void) V;
	(void) msg;
	bug("//================================");
	bug("// " << K << ": " << V);
	bug("//--------------------------------");
	bug_msg(msg);
	bug("//--------------------------------");
}

const str message::nospcrlfcl(CSTRING("\0 \r\n:")); // any octet except NUL, CR, LF, " " and ":"
const str message::nospcrlf(CSTRING("\0 \r\n")); // (":" / nospcrlfcl)
const str message::nocrlf(CSTRING("\0\r\n")); // (":" / nospcrlfcl)
const str message::nospcrlfat(CSTRING("\0 \r\n@")); // any octet except NUL, CR, LF, " " and "@"
const str message::bnf_special(CSTRING("[]\\`_^{|}")); // "[", "]", "\", "`", "_", "^", "{", "|", "}"

siss& message::gettrailing(siss& is, str& trailing) const
{
	// trailing: *(":" / " " / nospcrlfcl)
	// trailing: ("\0"|"\n"|"\r")^*

	char c;
	trailing.clear();
	while(is && nocrlf.find(is.peek()) == str::npos && is.get(c))
		trailing.append(1, c);
	if(is.eof())
		is.clear();
	return is;
}

siss& message::getmiddle(siss& is, str& middle) const
{
	// middle: nospcrlfcl *(":" / nospcrlfcl)
	// middle: ("\0"|"\n"|"\r"|" "|":")^ ("\0"|"\n"|"\r"|" ")^*

	if(is && std::count(nospcrlfcl.cbegin(), nospcrlfcl.cend(), is.peek()))
		is.setstate(std::ios::failbit);
	else
	{
		middle = is.get();
		while(is && is.peek() != EOF && !std::count(nospcrlf.cbegin(), nospcrlf.cend(), is.peek()))
			middle += is.get();
	}
	return is;
}

siss& message::getparams(siss& is, str_vec& middles, str& trailing) const
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

str message::get_servername() const
{
	// prefix: servername | (nickname [[ "!" user ] "@" host ])
	if(prefix.find('!') != str::npos || prefix.find('@') != str::npos)
		return "";
	return prefix;
}

bool message::is_nick(const str& nick) const
{
	if(nick.empty())
		return false;
	if(!isalpha(nick[0]) && !std::count(bnf_special.cbegin(), bnf_special.cend(), nick[0]))
		return false;
	for(char c: nick)
		if(!isalnum(c) && !std::count(bnf_special.cbegin(), bnf_special.cend(), c) && c != '-')
			return false;
	return true;
}

str message::allow_nick(const str& nick) const
{
	if(!is_nick(nick))
		return "";
	return nick;
}

str message::get_nickname() const
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

bool message::is_user(const str& user) const
{
	for(char c: user)
		if(std::count(nospcrlfat.cbegin(), nospcrlfat.cend(), c))
			return false;
	return true;
}

str message::allow_user(const str& user) const
{
	if(is_user(user))
		return user;
	return "";
}

str message::get_user() const
{
	// prefix: servername | (nickname [[ "!" user ] "@" host ])
	str::size_type beg = 0, end;
	if((end = prefix.find('@')) != str::npos)
		if((beg = prefix.find('!')) < end)
			return allow_user(prefix.substr(beg + 1, end - (beg + 1)));
	return "";
}

str message::get_host() const
{
	// prefix: servername | (nickname [[ "!" user ] "@" host ])
	str::size_type pos;
	if((pos = prefix.find('@')) != str::npos && pos < prefix.size() - 1)
		return prefix.substr(pos + 1);
	return "";
}

bool message::get_params(str_vec& middles, str& trailing) const
{
	return (bool)getparams(siss(params), middles, trailing);
}

str_vec message::get_params() const
{
	// treat all params equally (middles and trailing)
	str trailing;
	str_vec middles;
	if(get_params(middles, trailing))
		middles.push_back(trailing);
	return middles;
}

str_vec message::get_middles() const
{
	str trailing;
	str_vec middles;
	get_params(middles, trailing);
	return middles;
}

str message::get_trailing() const
{
	str trailing;
	str_vec middles;
	get_params(middles, trailing);
	return trailing;
}

bool message::parse(const str& line)
{
	if(line.empty())
		return false;
	// MUST handle both
	// [":" prefix SPACE] command [params] cr
	// [":" prefix SPACE] command [params] crlf

	this->line = line;
	this->when = std::time(0); // now
	siss iss(hol::trim_right_mute(this->line)); // solve cr crlf endings
	if(iss.peek() == ':') // optional prefix
		iss.ignore() >> prefix;
	return (bool)sgl(iss >> command, params);
}

}} // skivvy::ircbot
