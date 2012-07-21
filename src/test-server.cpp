#include <skivvy/server.h>
#include <skivvy/socketstream.h>

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

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional>

#include <skivvy/types.h>
#include <skivvy/logrep.h>
#include <skivvy/server.h>
#include <skivvy/socketstream.h>

using namespace skivvy;
using namespace skivvy::types;
using namespace skivvy::utils;
//using namespace skivvy::ircbot;

#define bug_in(i) do{bug(i.peek());}while(false)

namespace irc {

struct message
{
	static const str spcrlfcl;
	static const str ztcrlfspco;
	static const str ztcrlf;
	static const str ztcrlfsp;

	str prefix;
	str cmd; // TODO: Change this to command to refactor changes throughout
	str params;

	// params:   (SPACE middle){0,14} [SPACE ":" trailing]
	//       : | (SPACE middle){14}   [SPACE [ ":" ] trailing]
	str_vec middles;
	str trailing;


	// prefix: servername | (nickname [[ "!" user ] "@" host ])

	str get_servername()
	{
		if(prefix.find('!') != str::npos || prefix.find('@') != str::npos)
			return "";
		return prefix;
	}

	str get_nickname()
	{
		str::size_type pos;
		if((pos = prefix.find('!')) != str::npos)
			return prefix.substr(0, pos);
		if((pos = prefix.find('@')) != str::npos)
			return prefix.substr(0, pos);
		return prefix;
	}

	str get_user()
	{
		str::size_type beg = 0, end;
		if((end = prefix.find('@')) != str::npos)
			if((beg = prefix.find('!')) < end)
				return prefix.substr(beg + 1, end - (beg + 1));
		return "";
	}

	str get_host()
	{
		str::size_type pos;
		if((pos = prefix.find('@')) != str::npos && pos < prefix.size() - 1)
			return prefix.substr(pos + 1);
		return "";
	}

	str line; // working

//	std::istream& getparam(std::istream& is, str_vec& middles, str& trailing)
//	{
//		bug_func();
//		// params:   (SPACE middle){0,14} [SPACE ":" trailing]
//		//       : | (SPACE middle){14}   [SPACE [ ":" ] trailing]
//
//		middles.clear();
//		trailing.clear();
//
//		char c;
//		str middle;
//		siz n;
//
//		bug("do middle");
//		for(n = 0; n < 14 && is; ++n)
//		{
//			bug("n: " << n);
//			if(is.peek() != ' ')
//			{
//				bug("end of middle: " << std::hex << int(is.peek()));
//				if(is.eof())
//					is.clear();
//				return is;
//			}
//
//			is.get(c);
//			bug("c: " << std::hex << int(c));
//
//			if(is.peek() == ':')
//			{
//				bug("only trailing:");
//				// trailing: (":" | " " | nospcrlfcl)*
//				while(is && (is.peek() == ':' || is.peek() == ' ' || spcrlfcl.find(char(is.peek())) == str::npos))
//					if(is.get(c))
//						trailing.append(1, c);
//				if(is.eof())
//					is.clear();
//				return is;
//			}
//
//			bug("middle:" << middle);
//
//			// middle:  nospcrlfcl *( ":" / nospcrlfcl )
//			if(spcrlfcl.find(char(is.peek())) != str::npos)
//			{
//				bug("err: " << std::hex << int(is.peek()));
//				is.setstate(std::ios_base::failbit);
//				return is;
//			}
//
//			bug("init: " << bool(is));
//			if(is.get(c))
//				middle.append(1, c);
//			bug("middle: " << middle);
//
//			bug("repeats:" << bool(is));
//			while(is && (is.peek() == ':' || spcrlfcl.find(char(is.peek())) == str::npos))
//				if(is.get(c))
//				{
//					middle.append(1, c);
//					//bug("middle: " << middle);
//				}
//			if(is.eof())
//				is.clear();
//			if(is)
//				middles.push_back(middle);
//		}
//
//		bug("is      :" << bool(is));
//		if(is.peek() != ' ')
//		{
//			if(is.eof())
//				is.clear();
//			return is;
//		}
//		is.get();
//
//		if(n == 14 && is.peek() == ':')
//			is.get();
//
//		// trailing: (":" | " " | nospcrlfcl)*
//		while(is && (is.peek() == ':' || is.peek() == ' ' || spcrlfcl.find(char(is.peek())) == str::npos))
//		{
//			bug("is.peek(): " << is.peek());
//			if(is.get(c))
//				trailing.append(1, c);
//		}
//		if(is.eof())
//			is.clear();
//		return is;
//	}

	std::istream& gettrailing(std::istream& is, str& trailing)
	{
		// trailing: *(":" / " " / nospcrlfcl)
		// trailing: ("\0"|"\n"|"\r")^*
		char c;
		trailing.clear();
		while(is && ztcrlf.find(c) == str::npos && is.get(c))
//		while(is && ((c = is.peek()) == ':' || c == ' ' || spcrlfcl.find(c) == str::npos) && is.get(c))
			trailing.append(1, c);
		if(is.eof())
			is.clear();
		return is;
	}

	std::istream& getmiddle(std::istream& is, str& middle)
	{
		// middle: nospcrlfcl *(":" / nospcrlfcl)
		// middle: ("\0"|"\n"|"\r"|" "|":")^ ("\0"|"\n"|"\r"|" ")^*
		char c;
		middle.clear();
		if(is && spcrlfcl.find(is.peek()) == str::npos && is.get(c))
			middle.append(1, c);
		while(is && ztcrlfsp.find(is.peek()) == str::npos && is.get(c))
//		while(is && (is.peek() == ':' || spcrlfcl.find(is.peek()) == str::npos) && is.get(c))
			middle.append(1, c);
		if(is.eof())
			is.clear();
		return is;
	}

	std::istream& getparam(std::istream& is, str_vec& middles, str& trailing)
	{
		bug_func();
		// params  :   (SPACE middle){0,14} [SPACE ":" trailing]
		//         : | (SPACE middle){14}   [SPACE [ ":" ] trailing]
		// middle  : nospcrlfcl *( ":" / nospcrlfcl )
		// trailing: *( ":" / " " / nospcrlfcl )

		// abcd :efgh

		middles.clear();
		trailing.clear();

		char c;
		siz n = 0;

		while(is && n < 14 && is.peek() == ' ' && is.get(c))
		{
			if(is.peek() == ':' && is.get(c))
				return gettrailing(is, trailing);

			str middle;
			if(getmiddle(is, middle))
				middles.push_back(middle);
		}

		if(n == 14 && is.peek() == ' ' && is.get(c))
			if(is.peek() != ':' || is.get(c))
				return gettrailing(is, trailing);

		if(is.eof())
			is.clear();
		return is;
	}

	bool getparam(std::istream&& is, str_vec& middles, str& trailing)
	{
		bug_func();
		return getparam(is, middles, trailing);
	}

	bool getparam(str_vec& middles, str& trailing)
	{
		bug_func();
		bug("params: " << params);
		return getparam(std::istringstream(params), middles, trailing);
	}

	// MUST handle both
	// [":" prefix SPACE] command [params] cr
	// [":" prefix SPACE] command [params] crlf
	std::istream& parse(std::istream& is)
	{
		if(char c = is.peek() == ':') // optional prefix
			is.get(c) >> prefix;
		return std::getline(is >> cmd, params, '\r') >> std::ws;
	}

	bool parse(std::istream&& is) { return parse(is); }
	bool parse(const str& line) { return parse(std::istringstream(line)); }
	friend std::istream& operator>>(std::istream& is, message& msg) { return msg.parse(is); }
};

const str message::spcrlfcl = "\0\n\r :";
const str message::ztcrlfspco = "\0\n\r :";
const str message::ztcrlf = "\0\n\r :";
const str message::ztcrlfsp = "\0\n\r :";

}

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

	irc::message msg;
	while(!done && std::getline(ss, line) && std::istringstream(line) >> msg)
	{
		if(msg.cmd == "QUIT")
		{
		//	ss << msg("ERROR Client terminated.") << std::endl;
			ss << ":" << server_name << " ERROR Client terminated." << crlf;
			done = true;
		}
		else if(msg.cmd == "PASS")
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

	irc::message msg;

	//std::istringstream iss(":WiZ!jto@tolsun.oulu.fi NICK Kilroy");
	std::ifstream ifs("test-messages.txt");
	std::ofstream ofs("test-messages-out.txt");

	botlog(&ofs);

	str line;
//	while(ifs >> msg)
	while(std::getline(ifs, line) && std::istringstream(line) >> msg)
	{
		log("====================================================");
		log(line);
		log("----------------------------------------------------");
		log("prefix        : " << msg.prefix);
		log("cmd           : " << msg.cmd);
		log("params        : " << msg.params);
		log("get_servername: " << msg.get_servername());
		log("get_nickname  : " << msg.get_nickname());
		log("get_user      : " << msg.get_user());
		log("get_host      : " << msg.get_host());

		str_vec middles;
		str trailing;
		msg.getparam(middles, trailing);
		for(const str& middle: middles)
			log("middle        : " << middle);
		log("trailing      : " << trailing);
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
