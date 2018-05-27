#pragma once
#ifndef _SOOKEE_SERVER_H
#define _SOOKEE_SERVER_H

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

#include <functional>
#include <thread>
#include <future>
#include <list>
#include <tuple>
#include <exception>
#include <string>
#include <mutex>

namespace skivvy { namespace net {

class server_exception
: public std::exception
{
	const std::string msg;
	const std::string file;
	const size_t line;

public:
	server_exception(const std::string& msg);
	server_exception(const std::string& msg, const char* file, size_t line);
	virtual ~server_exception() throw();
	virtual const char* what() const throw();
};

class server
{
	using proc = std::pair<int, std::future<void>>;
	using proc_list = std::list<proc>;
	using proc_iter = proc_list::iterator;
	using proc_citer = proc_list::const_iterator;

	enum PROC { SOCKET, FUTURE };

	static const size_t max_threads;
	std::function<void(int)> func;
	int ss;
	bool done;

	std::mutex mtx;
	proc_list pool;
//	std::thread clean_thread;
	std::future<void> clean_fut;

	bool bind(long port, const std::string& iface = "0.0.0.0");
	void insert(int cs);
	void clean();

public:
	server();
	~server();

	void listen(long port, const std::string& iface = "0.0.0.0");
	void stop() { done = true; }
	void set_handler(void(*func)(int));
	void set_handler(const std::function<void(int)>& func);
};

}} // sookee::net

#endif // _SOOKEE_SERVER_H
