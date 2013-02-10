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
#include <queue>
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
#include <skivvy/irc-constants.h>

//using namespace sookee;

using namespace skivvy;
using namespace skivvy::irc;
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

typedef std::unique_lock<std::mutex> unique_lock;
typedef std::vector<message> message_vec;
typedef std::list<message> message_lst;
typedef std::queue<message> message_que;
typedef std::function<bool(const message&)> request_func;

class irc_server
{
private:
	std::istream& is;
	std::ostream& os;

	std::mutex mtx;
	std::future<void> fut;
	std::condition_variable cv;

	std::mutex traffic_mtx;
	message_que traffic;
	std::condition_variable traffic_cv;

	// TODO: Can ALL these be associated with their requests?
	std::mutex responses_mtx;
	message_lst responses;
	std::condition_variable responses_cv;

	// TODO: These can not be associated with their requests
	// therefore send to traffic for logging?
//	std::mutex errors_mtx;
//	message_lst errors;
//	std::condition_variable errors_cnd;

	bool done = false;
	void async_queue()
	{
		bug_func();
		str line;
		message msg;
		while(!done && sgl(is, line))
		{
			bug_var(line);
			std::this_thread::yield();
			if(!parsemsg(line, msg))
			{
				responses_cv.notify_all();
				traffic_cv.notify_all();
				continue;
			}
//			if(msg.command >= "001" && msg.command <= "099")
//			{
//				// 5.1 Command responses
//				//
//				//    Numerics in the range from 001 to 099 are used for client-server
//				//    connections only and should never travel between servers.
//				std::unique_lock<std::mutex> lock(traffic_mtx);
//				while(!done && traffic.size() > 100)
//					traffic_cnd.wait(lock);
//				traffic.push(msg);
//				traffic_cnd.notify_one();
//			}
			if(response_mode && msg.command >= "200" && msg.command <= "399")
			{
				// 5.1 Command responses
				//
				//    Replies generated in the response to commands are found in the
				//    range from 200 to 399.
				std::unique_lock<std::mutex> lock(responses_mtx);
				if(!done)
				{
					responses.push_back(msg);
					responses_cv.notify_all();
				}
			}
			else
			{
				// 5.1 Command responses
				//
				//    Numerics in the range from 001 to 099 are used for client-server
				//    connections only and should never travel between servers.
				std::unique_lock<std::mutex> lock(traffic_mtx);
				if(!done)
				{
					traffic.push(msg);
					traffic_cv.notify_all();
				}
			}

//			else if(msg.command >= "400" && msg.command <= "599")
//			{
				// Error replies are found in the range from 400 to 599.
//				std::unique_lock<std::mutex> lock(errors_mtx);
//				while(!done && errors.size() > 100)
//					errors_cnd.wait(lock);
//				errors.push_back(msg);
//				errors_cnd.notify_one(); // gives up lock ??
//			}
		}
	}

protected:
	bool logged_in = false;
	bool response_mode = false;

public:
	irc_server(std::istream& is, std::ostream& os)
	: is(is), os(os)
//	, fut(std::async(std::launch::async, [this]{ async_queue(); }))
	{
	}

	virtual ~irc_server() {}

	void start()
	{
		fut = std::async(std::launch::async, [this]{ async_queue(); });
	}

	void set_response_mode(bool state = true)
	{
		response_mode = state;
	}

	bool async_get_traffic(message& msg, siz timeout = 60)
	{
		bug_func();
		if(done)
			return false;

		st_time_point end_point = st_clk::now() + std::chrono::seconds(timeout);

		unique_lock lock(traffic_mtx);
		while(!done && traffic.empty())
			traffic_cv.wait_until(lock, end_point);

		if(done || st_clk::now() > end_point)
		{
			traffic_cv.notify_one();
			return false;
		}

		msg = traffic.front();
		traffic.pop();
		traffic_cv.notify_one();

		if(msg.command == "001")
			logged_in = true;

		return true;
	}

	bool async_send(const str& m)
	{
		bug_func();
		os << m << "\r\n" << std::flush;
		return os;
	}

	const str_set whois_responses =
	{
		RPL_WHOISUSER, RPL_WHOISCHANNELS, RPL_WHOISSERVER, RPL_ENDOFWHOIS
	};

	bool async_request(const str& req, request_func is_mine, request_func is_last, message_vec& res, siz timeout = 60)
	{
		if(!async_send(req))
			return false;

		st_time_point end_point = st_clk::now() + std::chrono::seconds(timeout);

		res.clear();
		bool complete = false;
		while(!done && !complete)
		{
			unique_lock lock(responses_mtx);
			while(!done && responses.empty())
				responses_cv.wait_until(lock, end_point);

			if(done || st_clk::now() > end_point)
			{
				responses_cv.notify_one();
				return false;
			}

			for(message_lst::iterator i = responses.begin(); i != responses.end();)
			{
				if(is_mine(*i))
				{
					res.push_back(*i);
					if(is_last(*i))
						complete = true;
					i = responses.erase(i);
				}
				else
					++i;
			}
			responses_cv.notify_one();
		}

		return complete;
	}

	bool async_whois(const str& caller_nick, const str& nick, message_vec& res, siz timeout = 60)
	{
		request_func is_mine = [&](const message& msg)
		{
			if(std::find(whois_responses.cbegin(), whois_responses.cend(), msg.command) == whois_responses.cend())
				return false;
			const str_vec params = msg.get_params();
			if(params.size() < 2)
				return false; // bad response
			if(params[0] != caller_nick || params[1] != nick)
				return false;
			return true;
		};
		request_func is_last = [&](const message& msg)
		{
			return msg.command == RPL_ENDOFWHOIS;
		};
		return async_request("WHOIS " + nick, is_mine, is_last, res, timeout);
	}
};

class remote_irc_server
: public irc_server
{
	net::socketstream ss;
public:
	remote_irc_server(): irc_server(ss, ss) {}

	bool connect(const str& host, const siz port)
	{
		if(!ss.open(host, port))
			return false;
		return true;
	}
};

int main()
{
	remote_irc_server irc;

	if(!irc.connect("irc.quakenet.org", 6667))
		return 1;

	irc.start();

	bool done = false;

	done |= !irc.async_send("PASS none");
	done |= !irc.async_send("NICK Squig");
	done |= !irc.async_send("USER Squig 0 * :Squig");

	if(done)
	{
		// error
		return 1;
	}

	message msg;
	while(!done && irc.async_get_traffic(msg, 60))
	{
//		printmsg(std::cout, msg);
		if(msg.command == RPL_ENDOFMOTD)
		{
			irc.set_response_mode();

			irc.async_send("JOIN #skivvy");

			message_vec res;
			if(!irc.async_whois("Squig", "SooKee", res))
			{
				if(res.empty())
				{
					log("ERROR: calling whois()");
					continue;
				}
				log("WARNING: Partial response from whois()");
			}
			for(const message& msg: res)
				bug("WHOIS RESPONSE: " << msg.line);
		}
		else if(msg.command == "PING")
		{
			irc.async_send("PONG " + msg.get_trailing());
		}
	}
}
