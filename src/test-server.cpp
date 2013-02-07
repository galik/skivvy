//#undef DEBUG

#include <skivvy/server.h>
#include <skivvy/socketstream.h>

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

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>

#include <sookee/stl.h>
#include <sookee/str.h>

#include <skivvy/types.h>
#include <skivvy/logrep.h>
#include <skivvy/server.h>
#include <skivvy/socketstream.h>
#include <skivvy/message.h>

//using namespace sookee;

using namespace skivvy;
using namespace skivvy::types;
using namespace skivvy::utils;
//using namespace skivvy::ircbot;
#define bug_in(i) do{bug(i.peek());}while(false)
#define CSTRING(s) s,sizeof(s)
#define STRC(c) str(1, c)


namespace irc {

str hex(char c)
{
	soss oss;
	oss << "\\x" << std::hex << siz(c);
	return oss.str();
}

str bugify(const str& s)
{
	str r = s;
	soo::replace(r, STRC('\0'), "\\0");
	soo::replace(r, STRC('\n'), "\\n");
	soo::replace(r, STRC('\r'), "\\r");
	soo::replace(r, STRC('\t'), "\\t");
	return r;
}

}

using namespace skivvy;
using namespace skivvy::ircbot;

void echo_service(int cs)
{
	log("Echo service starting.");

	str line;
	net::socketstream ss(cs);

	while(std::getline(ss, line) && line != "END")
	{
		ss << "ECHO: " << line << std::endl;
	}
	ss << "END:" << std::endl;
	log("Echo service closed.");
}

std::string msg(const std::string& text)
{
	return ":skivvy_irc_test_server " + text;
}

void irc_service(int cs)
{
	log("IRC service starting.");

	static const str server_name = "skivvy_irc_test_server";
	static const str crlf = "\r\n";

	bool done = false;
	str line;
	net::socketstream ss(cs);

	str pass;
	str user;
	str nick;

	message msg;
	while(!done && std::getline(ss, line) && std::istringstream(line) >> msg)
	{
		if(msg.command == "QUIT")
		{
		//	ss << msg("ERROR Client terminated.") << std::endl;
			ss << ":" << server_name << " ERROR Client terminated." << crlf;
			done = true;
		}
		else if(msg.command == "PASS")
		{
		//	pass = msg.get_user_cmd();
		}
	}

	ss << std::flush;


	log("IRC service closed.");
}

int main(int argc, char* argv[])
{
//	std::cout << int('\n');

	message msg;

	//std::istringstream iss(":WiZ!jto@tolsun.oulu.fi NICK Kilroy");
	std::ifstream ifs("test/messages.txt");
	if(!ifs)
	{
		log("Could not open input file.");
		return 1;
	}


	std::ofstream ofs("test/messages-out.txt");
	if(!ofs)
	{
		log("Could not open output file.");
		return 1;
	}

	str_siz_map done;

	siz pos = 0;
	str line;
	while(sgl(ifs, line))
	{
		++pos;
		message msg;
		if(!parsemsg(line, msg))
		{
			log("ERROR: Parsing failed: " << pos << ": " << line);
			continue;
		}

		if(++done[msg.command] > 3)
			continue;
		log(pos << ": " << line);

		ofs << "===============================================\n";
		printmsg(ofs, msg);
//		ofs << "-----------------------------------------------\n";
	}

	return 0;

	bool server = false;
	size_t port = 39006;

	for(int a = 1; a < argc; ++a)
	{
		std::string arg = argv[a];

		if(arg == "-h" || arg == "--help")
		{
			std::cout << "Usage: test-server [<port>] [server]\n";
			return 0;
		}

		if(arg == "server")
			server = true;

		size_t p;
		if(std::istringstream(arg) >> p)
			port = p;
	}

	if(server)
	{
		log("Running server on port: " << port);
		try
		{
			net::server s;
			s.set_handler(&echo_service);
			s.listen(port);
		}
		catch(std::exception& e)
		{
			std::cout << e.what() << '\n';
			return 2;
		}
		return 0;
	}

	log("Client connecting on port: " << port);

	net::socketstream ss;
	ss.open("localhost", port);

	//std::string line;
	while(ss and std::getline(std::cin, line))
	{
		ss << line << std::endl;
		if(std::getline(ss, line))
		{
			std::cout << line << '\n';
		}
	}

	return 0;
}
