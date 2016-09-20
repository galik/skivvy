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

#include <skivvy/server.h>

#include <hol/bug.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <streambuf>
#include <sstream>
#include <iostream>
#include <map>
#include <functional>
#include <set>
#include <thread>
#include <vector>
#include <future>
#include <queue>
#include <list>
#include <tuple>

#include <hol/small_types.h>

namespace skivvy { namespace net {

using namespace hol::small_types::basic;

using lock_guard = std::lock_guard<std::mutex>;

#define throw_server_exception(msg) throw server_exception(msg, __FILE__, __LINE__)

server_exception::server_exception(const std::string& msg):msg(msg), file(""), line(0) {}
server_exception::server_exception(const std::string& msg, const char* file, size_t line)
:msg(msg)
, file(file)
, line(line)
{
}
server_exception::~server_exception() throw() {}

const char* server_exception::what() const throw()
{
	std::ostringstream oss;
	oss << msg;
	if(line)
	{
		 oss << "\n error: " << file << " in line " << line << std::endl;
	}
	return oss.str().c_str();
}

const size_t server::max_threads = 10;

server::server()
: ss(::socket(PF_INET, SOCK_STREAM, 0))
, done(false)
//, clean_thread([=]{ this->clean(); })
, clean_fut(std::async(std::launch::async, [&]{ clean(); }))
{
}

server::~server()
{
	done = true;
	if(clean_fut.valid())
		if(clean_fut.wait_for(std::chrono::seconds(3)) == std::future_status::ready)
			clean_fut.get();
}

void server::insert(int cs)
{
	bug_fun();
	lock_guard lock(mtx);
	bug("adding...");
	pool.push_back(proc{cs, std::async(std::launch::async, func, cs)});
}

const std::chrono::seconds NOTHING(0);
const std::chrono::seconds ONE_SECOND(1);

//void server::clean()
//{
//	while(!done || !pool.empty())
//	{
//		std::this_thread::sleep_for(ONE_SECOND);
//		if(pool.empty()) continue;
//		bug("cleaning...");
//		mtx.lock();
//		for(proc_iter p = pool.begin(); p != pool.end();)
//		{
//			const std::future<void>& f = p->second;//std::get<PROC::FUTURE>(*p);
//			if(!f.wait_for(NOTHING)) { ++p; continue; }
//			bug("erasing : " << p->first);
//			close(p->first);//std::get<PROC::SOCKET>(*p));
//			p = pool.erase(p);
//		}
//		mtx.unlock();
//	}
//}

void server::clean()
{
	bug_fun();
	while(!done || !pool.empty())
	{
		bug("waiting...");
		std::this_thread::sleep_for(ONE_SECOND);
		bug("done");

		if(pool.empty())
			continue;

		bug("locking...");
		lock_guard lock(mtx);

		bug("cleaning...");
		for(proc_iter p = pool.begin(); p != pool.end();)
		{
			if(p->second.wait_for(NOTHING) == std::future_status::ready)
			{
				bug("erasing : " << p->first);
				close(p->first);
				p = pool.erase(p);
				continue;
			}
			bug("skipping : " << p->first);
			++p;
		}
	}
}

bool server::bind(long port, const std::string& iface)
{
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(iface.c_str());
	return ::bind(ss, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != -1;
}

void server::listen(long port, const std::string& iface)
{
	if(!bind(port, iface)) throw_server_exception(strerror(errno));
	if(::listen(ss, 10) == -1) throw_server_exception(strerror(errno));

	while(!done)
	{
		bug(":");
		bug("| ready:");
		sockaddr connectee;
		socklen_t connectee_len = sizeof(sockaddr);
		int cs;
		if((cs = ::accept(ss, &connectee, &connectee_len)) == -1)
			throw_server_exception(strerror(errno));

		bug("|");
		bug("|==> accept socket = " << cs);
		bug("|");

		insert(cs);
	}
}

void server::set_handler(void(*func)(int)) { this->func = func; }
void server::set_handler(const std::function<void(int)>& func) { this->func = func; }

}} // skivvy::net
