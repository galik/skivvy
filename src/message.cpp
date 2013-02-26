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

#include <skivvy/types.h>
#include <sookee/str.h>
#include <skivvy/store.h>
#include <skivvy/logrep.h>

#define CSTRING(s) s,sizeof(s)
#define STRC(c) str(1, c)

namespace skivvy { namespace ircbot {

using namespace skivvy::types;
using namespace sookee::string;
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
	return trim(cmd);
}

str message::get_user_params() const
{
	str params;
	siss iss(get_trailing());
	sgl(iss, params, ' ');
	if(!sgl(iss, params))
		return "";
	return trim(params);
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
	for(const str& param: m.get_params())
		os << "// param                : " << param << '\n';
	for(const str& middle: m.get_middles())
		os << "// middle               : " << middle << '\n';
	os << "// trailing             : " << m.get_trailing() << '\n';
	os << "// get_nick()           : " << m.get_nick() << '\n';
	os << "// get_chan()           : " << m.get_chan() << '\n';
	os << "// get_user_cmd()       : " << m.get_user_cmd() << '\n';
	os << "// get_user_params()    : " << m.get_user_params() << '\n';
	return os << std::flush;
}

void bug_message(const std::string& K, const std::string& V, const message& msg)
{
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

}} // skivvy::ircbot
