/*
 * ircbot-rserver.cpp
 *
 *  Created on: 10 Dec 2011
 *      Author: oasookee@googlemail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oasookee@googlemail.com               |
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

#include <skivvy/plugin-rserver.h>

#include <ctime>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>

#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <skivvy/str.h>
#include <skivvy/logrep.h>
#include <skivvy/socketstream.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(RServerIrcBotPlugin);
PLUGIN_INFO("Remote Server", "0.1");

using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

RServerIrcBotPlugin::RServerIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot), ss(::socket(PF_INET, SOCK_STREAM, 0))
{
	fcntl(ss, F_SETFL, fcntl(ss, F_GETFL, 0) | O_NONBLOCK);
}

RServerIrcBotPlugin::~RServerIrcBotPlugin() {}

bool RServerIrcBotPlugin::bind(port p, const std::string& iface)
{
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(p);
	addr.sin_addr.s_addr = inet_addr(iface.c_str());
	return ::bind(ss, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != -1;
}

bool RServerIrcBotPlugin::listen(port p)
{
	if(!bind(p))
	{
		log("ERROR: " << std::strerror(errno));
		return false;
	}
	if(::listen(ss, 10) == -1)
	{
		log("ERROR: " << std::strerror(errno));
		return false;
	}
	while(!done)
	{
		sockaddr connectee;
		socklen_t connectee_len = sizeof(sockaddr);
		socket cs;

		while(!done)
		{
			while(!done && (cs = ::accept4(ss, &connectee, &connectee_len, SOCK_NONBLOCK)) == -1)
			{
				if(!done)
				{
					if(errno ==  EAGAIN || errno == EWOULDBLOCK)
						std::this_thread::sleep_for(std::chrono::seconds(1));
					else
					{
						log("ERROR: " << strerror(errno));
						return false;
					}
				}
			}

			if(!done)
				std::async(std::launch::async, [&]{ process(cs); });
		}
	}
	return true;
}

void RServerIrcBotPlugin::process(socket cs)
{
	net::socketstream ss(cs);
	// receive null terminated string
	std::string cmd;
	char c;
	while(ss.get(c) && c) cmd.append(1, c);
	bug("cmd: " << cmd);
	if(!trim(cmd).empty())
	{
		std::ostringstream oss;
		bot.exec(cmd, &oss);
		ss << oss.str() << '\0' << std::flush;
	}
}

void RServerIrcBotPlugin::on(const message& msg)
{
	bug("-----------------------------------------------------");
	bug(get_name() << ": " << "on()");
	bug("-----------------------------------------------------");
	bug_msg(msg);
}

void RServerIrcBotPlugin::off(const message& msg)
{
	bug("-----------------------------------------------------");
	bug(get_name() << ": " << "off()");
	bug("-----------------------------------------------------");
	bug_msg(msg);
}

bool RServerIrcBotPlugin::initialize()
{
	con = std::async(std::launch::async, [&]{ listen(7334); });
	return true;
}

// INTERFACE: IrcBotPlugin

std::string RServerIrcBotPlugin::get_name() const { return NAME; }
std::string RServerIrcBotPlugin::get_version() const { return VERSION; }

void RServerIrcBotPlugin::exit()
{
	done = true;
	close(ss);
	// TODO: find a way to get past block...
	//if(con.valid()) con.get();
}

}} // sookee::ircbot