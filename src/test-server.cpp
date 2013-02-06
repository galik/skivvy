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

struct message
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
const str message::nospcrlfcl(CSTRING("\0 \r\n:")); // any octet except NUL, CR, LF, " " and ":"
const str message::nospcrlf(CSTRING("\0 \r\n")); // (":" / nospcrlfcl)
const str message::nocrlf(CSTRING("\0\r\n")); // (":" / nospcrlfcl)
const str message::nospcrlfat(CSTRING("\0 \r\n@")); // any octet except NUL, CR, LF, " " and "@"
const str message::bnf_special(CSTRING("[]\\`_^{|}")); // "[", "]", "\", "`", "_", "^", "{", "|", "}"

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

	irc::message msg;

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

	botlog(&ofs);

	str line;
//	while(ifs >> msg)
	while(sgl(ifs, line))
	{
		log(str(68, '=')); //"====================================================");
		log(line);
		log(str(68, '-')); // "----------------------------------------------------");
		if(!(siss(line) >> msg))
		{
			log("ERROR: parsing line: " << line);
			continue;
		}
		log("prefix        : " << msg.prefix);
		log("cmd           : " << msg.command);
		log("params        : " << msg.params);
		log("get_servername: " << msg.get_servername());
		log("get_nickname  : " << msg.get_nickname());
		log("get_user      : " << msg.get_user());
		log("get_host      : " << msg.get_host());

//		str_vec middles;
//		str trailing;
//		msg.getparams(middles, trailing);
		for(const str& middle: msg.get_middles())
			log("middle        : " << middle);
		log("trailing      : " << msg.get_trailing());
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
