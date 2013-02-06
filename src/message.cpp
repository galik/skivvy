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
	std::getline(is, m.line_cp);
	trim(m.line_cp);
	// :SooKee!~SooKee@SooKee.users.quakenet.org MODE #skivvy-admin +o Zim-blackberry

	if(!m.line_cp.empty())
	{
		if(m.line_cp[0] == ':')
		{
			siss iss(m.line_cp.substr(1));
			iss >> m.from_cp >> m.cmd_cp;
			std::getline(iss, m.params_cp, ':');
			std::getline(iss, m.text_cp);
			iss.clear();
			iss.str(m.params_cp);
			iss >> m.to_cp;
		}
		else
		{
			siss iss(m.line_cp);
			iss >> m.cmd_cp;
			std::getline(iss, m.params_cp, ':');
			std::getline(iss, m.text_cp);
			iss.clear();
			iss.str(m.params_cp);
			iss >> m.to_cp;
		}

		trim(m.from_cp);
		trim(m.cmd_cp);
		trim(m.params_cp);
		trim(m.to_cp);
		trim(m.text_cp);
	}

	return is;
}

std::istream& parsemsg_cp(std::istream&& is, message_cp& m)
{
	return parsemsg_cp(is, m);
}

std::ostream& printmsg_cp(std::ostream& os, const message_cp& m)
{
	os << "//                  line: " << m.line_cp << '\n';
	os << "//                  from: " << m.from_cp << '\n';
	os << "//                   cmd: " << m.cmd_cp << '\n';
	os << "//                params: " << m.params_cp << '\n';
	os << "//                    to: " << m.to_cp << '\n';
	os << "//                  text: " << m.text_cp << '\n';
	os << "// msg.from_channel()   : " << m.from_channel_cp() << '\n';
	os << "// msg.get_nick()       : " << m.get_nick_cp() << '\n';
	os << "// msg.get_user()       : " << m.get_user_cp() << '\n';
	os << "// msg.get_host()       : " << m.get_host_cp() << '\n';
	os << "// msg.get_userhost()   : " << m.get_userhost_cp() << '\n';
	os << "// msg.get_user_cmd()   : " << m.get_user_cmd_cp() << '\n';
	os << "// msg.get_user_params(): " << m.get_user_params_cp() << '\n';
	os << "// msg.reply_to()       : " << m.reply_to_cp() << '\n';
	return os << std::flush;
}

std::istream& operator>>(std::istream& is, message_cp& m)
{
	str o;
	if(!getobject(is, o))
		return is;
	if(!parsemsg_cp(siss(unescaped(o)), m))
		is.setstate(std::ios::failbit); // protocol error
	return is;
}

std::ostream& operator<<(std::ostream& os, const message_cp& m)
{
	return os << '{' << escaped(m.line_cp) << '}';
}

// MyNick!~User@server.com

str message_cp::get_nick_cp() const
{
//	bug_func();
	return from_cp.substr(0, from_cp.find("!"));
}

str message_cp::get_user_cp() const
{
//	bug_func();
	return from_cp.substr(from_cp.find("!") + 1, from_cp.find("@") - from_cp.find("!") - 1);
}

str message_cp::get_host_cp() const
{
//	bug_func();
	return from_cp.substr(from_cp.find("@") + 1);
}

str message_cp::get_userhost_cp() const
{
//	bug_func();
	return from_cp.substr(from_cp.find("!") + 1);
}

str message_cp::get_user_cmd_cp() const
{
	str cmd;
	std::istringstream iss(text_cp);
	std::getline(iss, cmd, ' ');
	return trim(cmd);
}

str message_cp::get_user_params_cp() const
{
	str params;
	std::istringstream iss(text_cp);
	std::getline(iss, params, ' ');
	if(!std::getline(iss, params))
		return "";
	return trim(params);
}

bool message_cp::from_channel_cp() const
{
	return !to_cp.empty() && to_cp[0] == '#';
}

str message_cp::reply_to_cp() const
{
	return from_channel_cp() ? to_cp : get_nick_cp();
}

// TODO: Sort this mess out
void bug_message_cp(const std::string& K, const std::string& V, const message_cp& msg)
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
