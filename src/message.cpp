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

namespace skivvy { namespace ircbot {

using namespace skivvy::types;
using namespace skivvy::string;
using namespace skivvy::utils;

std::istream& parsemsg(std::istream& is, message& m)
{
	std::getline(is, m.line);
	trim(m.line);
	// :SooKee!~SooKee@SooKee.users.quakenet.org MODE #skivvy-admin +o Zim-blackberry

	if(!m.line.empty())
	{
		if(m.line[0] == ':')
		{
			siss iss(m.line.substr(1));
			iss >> m.from >> m.cmd;
			std::getline(iss, m.params, ':');
			std::getline(iss, m.text);
			iss.clear();
			iss.str(m.params);
			iss >> m.to;
		}
		else
		{
			siss iss(m.line);
			iss >> m.cmd;
			std::getline(iss, m.params, ':');
			std::getline(iss, m.text);
			iss.clear();
			iss.str(m.params);
			iss >> m.to;
		}

		trim(m.from);
		trim(m.cmd);
		trim(m.params);
		trim(m.to);
		trim(m.text);
	}

	return is;
}

std::istream& parsemsg(std::istream&& is, message& m)
{
	return parsemsg(is, m);
}

std::ostream& printmsg(std::ostream& os, const message& m)
{
	os << "//                  line: " << m.line << '\n';
	os << "//                  from: " << m.from << '\n';
	os << "//                   cmd: " << m.cmd << '\n';
	os << "//                params: " << m.params << '\n';
	os << "//                    to: " << m.to << '\n';
	os << "//                  text: " << m.text << '\n';
	os << "// msg.from_channel()   : " << m.from_channel() << '\n';
	os << "// msg.get_nick()       : " << m.get_nick() << '\n';
	os << "// msg.get_user()       : " << m.get_user() << '\n';
	os << "// msg.get_host()       : " << m.get_host() << '\n';
	os << "// msg.get_userhost()   : " << m.get_userhost() << '\n';
	os << "// msg.get_user_cmd()   : " << m.get_user_cmd() << '\n';
	os << "// msg.get_user_params(): " << m.get_user_params() << '\n';
	os << "// msg.reply_to()       : " << m.reply_to() << '\n';
	return os << std::flush;
}

std::istream& operator>>(std::istream& is, message& m)
{
	str o;
	if(!getobject(is, o))
		return is;
	if(!parsemsg(siss(unescaped(o)), m))
		is.setstate(std::ios::failbit); // protocol error
	return is;
}

std::ostream& operator<<(std::ostream& os, const message& m)
{
	return os << '{' << escaped(m.line) << '}';
}

// MyNick!~User@server.com

str message::get_nick() const
{
//	bug_func();
	return from.substr(0, from.find("!"));
}

str message::get_user() const
{
//	bug_func();
	return from.substr(from.find("!") + 1, from.find("@") - from.find("!") - 1);
}

str message::get_host() const
{
//	bug_func();
	return from.substr(from.find("@") + 1);
}

str message::get_userhost() const
{
//	bug_func();
	return from.substr(from.find("!") + 1);
}

str message::get_user_cmd() const
{
	str cmd;
	std::istringstream iss(text);
	std::getline(iss, cmd, ' ');
	return trim(cmd);
}

str message::get_user_params() const
{
	str params;
	std::istringstream iss(text);
	std::getline(iss, params, ' ');
	if(!std::getline(iss, params))
		return "";
	return trim(params);
}

bool message::from_channel() const
{
	return !to.empty() && to[0] == '#';
}

str message::reply_to() const
{
	return from_channel() ? to : get_nick();
}

// TODO: Sort this mess out
void bug_message(const std::string& K, const std::string& V, const message& msg)
{
	bug("===============================");
	bug(K << ": " << V);
	bug("-------------------------------");
	bug_msg(msg);
	bug("-------------------------------");
}

}} // skivvy::ircbot
