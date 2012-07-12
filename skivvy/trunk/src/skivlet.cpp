/*
 * skivlet.cpp
 *
 *  Created on: 12 Jul 2012
 *      Author: oasookee@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2012 SooKee oasookee@gmail.com               |
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

#include <skivvy/stl.h>
#include <skivvy/types.h>
#include <skivvy/ircbot.h>
#include <skivvy/logrep.h>
#include <skivvy/socketstream.h>

#include <fstream>
#include <cstring>
#include <cstdlib>

using namespace skivvy;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::ircbot;

/*
 * Wait for a condition to become true else timeout.
 *
 * @param cond - a reference to the bool condition that
 * this function is waiting for.
 * @param secs - The number of seconds to wait.
 * @param res - The pause resolution - how long to wait between
 * testing the condition.
 * @return false - timout reached
 */
bool spin_wait(const bool& cond, siz secs, siz res = 1000)
{
	while(!cond && --secs)
		std::this_thread::sleep_for(std::chrono::milliseconds(res));
	return secs;
}

class Bot
{
	str host;
	int port;
	net::socketstream ss;
	bool connected = false;
	bool done = false;

	str_vec nick = {"SooKee", "Skivlet_2", "Skivlet_3", "Skivlet_4", "Skivlet_5"};
	siz uniq = 0;

	std::future<void> responder_fut;
	std::future<void> connecter_fut;

public:
	Bot(const str& host, int port): host(host), port(port) {}

	void start()
	{
		bug_func();
		std::srand(std::time(0));
		ss.open(host, port);
		connecter_fut = std::async(std::launch::async, [&]{ connecter(); });
		responder_fut = std::async(std::launch::async, [&]{ responder(); });
	}

	// ss << cmd.substr(0, 510) << "\r\n" << std::flush;

	void send(const str& cmd)
	{
		bug("send: " << cmd);
		static std::mutex mtx;
		lock_guard lock(mtx);
		ss << cmd.substr(0, 510) << "\r\n" << std::flush;
	}

	str ping;
	str pong;

	void connecter()
	{
		bug_func();
		send("PASS none");
		send("NICK " + nick[uniq]);
		while(!done)
		{
			if(!connected)
			{
				con("Attempting to connect...");
				send("USER Skivlet 0 * :Skivlet");
				spin_wait(done, 30);
			}
			if(connected)
			{
				bug_var(uniq);
				if(uniq)
					send("NICK " + nick[0]); // try to regain primary nick
				send("PING " + (ping = std::to_string(std::rand())));
				spin_wait(done, 30);
				if(ping != pong)
					connected = false;
			}
		}
	}

	void cli()
	{
		bug_func();
		str cmd;
		str line;
		str channel;
		std::istringstream iss;
		std::cout << "> " << std::flush;
		while(!done)
		{
			if(!std::getline(std::cin, line))
				continue;
			iss.clear();
			iss.str(line);
			if(!(iss >> cmd >> std::ws))
				continue;

			if(cmd == "/exit")
			{
				done = true;
			}
			else if(cmd == "/join")
			{
				if(!channel.empty())
					send("PART " + channel);
				iss >> channel;
				send("JOIN " + channel);
			}
			else if(cmd == "/part")
			{
				if(!channel.empty())
					send("PART " + channel);
			}
			else
			{
				send("PRIVMSG " + channel + " :" + line);
			}
			std::cout << "> " << std::flush;
		}
	}

	void join()
	{
		if(responder_fut.valid())
			responder_fut.get();
		if(connecter_fut.valid())
			connecter_fut.get();
		ss.close();
	}

	void responder()
	{
		bug_func();
		str line;
		message msg;
		while(!done)
		{
			if(!std::getline(ss, line))
				continue;
			bug("recv: " << line);
			std::istringstream iss(line);
			parsemsg(iss, msg);
			bug_msg(msg);
			if(msg.cmd == "001")
				connected = true;
			else if(msg.cmd == "PING")
				send("PONG " + msg.text);
			else if(msg.cmd == "433") // NICK IN USE
			{
				++uniq;
				if(uniq >= nick.size())
					uniq = 0;
			}
			else if(msg.cmd == "PONG")
				pong = msg.text;
			else if(msg.cmd == "NOTICE")
				std::cout << "NOTICE: " << msg.text << std::endl;
			else if(msg.cmd == "PRIVMSG")
				std::cout << msg.to << ": " << msg.text << std::endl;
		}
	}
};

int main()
{
	Bot bot("irc.quakenet.org", 6667);
	bot.start();
	bot.cli();
	bot.join();
}
