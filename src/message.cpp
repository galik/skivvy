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
#include <skivvy/str.h>
#include <skivvy/store.h>

#define CSTRING(s) s,sizeof(s)
#define STRC(c) str(1, c)

namespace skivvy { namespace ircbot {

using namespace skivvy::types;
using namespace skivvy::string;
using namespace skivvy::utils;

std::istream& parsemsg_cp(std::istream& is, message_cp& m)
{
//	std::getline(is, m.line_cp);
//	trim(m.line_cp);
//	// :SooKee!~SooKee@SooKee.users.quakenet.org MODE #skivvy-admin +o Zim-blackberry
//
//	if(!m.line_cp.empty())
//	{
//		if(m.line_cp[0] == ':')
//		{
//			siss iss(m.line_cp.substr(1));
//			iss >> m.from_cp >> m.cmd_cp;
//			std::getline(iss, m.params_cp, ':');
//			std::getline(iss, m.text_cp);
//			iss.clear();
//			iss.str(m.params_cp);
//			iss >> m.to_cp;
//		}
//		else
//		{
//			siss iss(m.line_cp);
//			iss >> m.cmd_cp;
//			std::getline(iss, m.params_cp, ':');
//			std::getline(iss, m.text_cp);
//			iss.clear();
//			iss.str(m.params_cp);
//			iss >> m.to_cp;
//		}
//
//		trim(m.from_cp);
//		trim(m.cmd_cp);
//		trim(m.params_cp);
//		trim(m.to_cp);
//		trim(m.text_cp);
//	}
//
	return is;
}

std::istream& parsemsg_cp(std::istream&& is, message_cp& m)
{
	return parsemsg_cp(is, m);
}

std::ostream& printmsg_cp(std::ostream& os, const message_cp& m)
{
//	os << "//                  line: " << m.line_cp << '\n';
//	os << "//                  from: " << m.from_cp << '\n';
//	os << "//                   cmd: " << m.cmd_cp << '\n';
//	os << "//                params: " << m.params_cp << '\n';
//	os << "//                    to: " << m.to_cp << '\n';
//	os << "//                  text: " << m.text_cp << '\n';
//	os << "// msg.from_channel()   : " << m.from_channel_cp() << '\n';
//	os << "// msg.get_nick()       : " << m.get_nick_cp() << '\n';
//	os << "// msg.get_user()       : " << m.get_user_cp() << '\n';
//	os << "// msg.get_host()       : " << m.get_host_cp() << '\n';
//	os << "// msg.get_userhost()   : " << m.get_userhost_cp() << '\n';
//	os << "// msg.get_user_cmd()   : " << m.get_user_cmd_cp() << '\n';
//	os << "// msg.get_user_params(): " << m.get_user_params_cp() << '\n';
//	os << "// msg.reply_to()       : " << m.reply_to_cp() << '\n';
	return os << std::flush;
}

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

// MyNick!~User@server.com

//str message_cp::get_nick_cp() const
//{
////	bug_func();
//	return from_cp.substr(0, from_cp.find("!"));
//}
//
//str message_cp::get_user_cp() const
//{
////	bug_func();
//	return from_cp.substr(from_cp.find("!") + 1, from_cp.find("@") - from_cp.find("!") - 1);
//}
//
//str message_cp::get_host_cp() const
//{
////	bug_func();
//	return from_cp.substr(from_cp.find("@") + 1);
//}
//
//str message_cp::get_userhost_cp() const
//{
////	bug_func();
//	return from_cp.substr(from_cp.find("!") + 1);
//}
//
//

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
	// TODO: this must be sensitive t the particular
	// command of this message. Mostly (I think) this will be
	// the first parameter...
	str_vec params = get_params();

	for(siz i = 0; i < chan_params.size(); ++i)
		if(chan_params[i].count(command) && params.size() > i)
			if(!params[i].empty() && soo::find(chan_start, params[i][0]) != chan_start.cend())
				return params[i];
	return "";
}

std::ostream& printmsg(std::ostream& os, const message& m)
{
	printmsg_cp(os, m);
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
	return os << std::flush;
}

// TODO: Sort this mess out
//void bug_message_cp(const std::string& K, const std::string& V, const message_cp& msg)
//{
//	bug("===============================");
//	bug(K << ": " << V);
//	bug("-------------------------------");
//	bug_msg(msg);
//	bug("-------------------------------");
//}

void bug_message(const std::string& K, const std::string& V, const message& msg)
{
	bug("===============================");
	bug(K << ": " << V);
	bug("-------------------------------");
	bug_msg(msg);
	bug("-------------------------------");
}

const str message::nospcrlfcl(CSTRING("\0 \r\n:")); // any octet except NUL, CR, LF, " " and ":"
const str message::nospcrlf(CSTRING("\0 \r\n")); // (":" / nospcrlfcl)
const str message::nocrlf(CSTRING("\0\r\n")); // (":" / nospcrlfcl)
const str message::nospcrlfat(CSTRING("\0 \r\n@")); // any octet except NUL, CR, LF, " " and "@"
const str message::bnf_special(CSTRING("[]\\`_^{|}")); // "[", "]", "\", "`", "_", "^", "{", "|", "}"

}} // skivvy::ircbot
