#pragma once
#ifndef _SOOKEE_RPC_H_
#define _SOOKEE_RPC_H_
/*
 * rpc.h
 *
 *  Created on: 10 Aug 2011
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

#include <skivvy/socketstream.h>

#include <map>
#include <set>
#include <string>
#include <vector>
#include <istream>
#include <ostream>
#include <sstream>

namespace skivvy { namespace rpc {

class call
{
	std::iostream& i;
	std::iostream& o;

public:
	// Pre call
	call(std::iostream& i, std::iostream& o): i(i), o(o) {}
	virtual ~call() {}

	bool call_error() { return i; }
	bool return_error() { return o; }
	std::string get_error() { std::string e; std::getline(o, e); return e; }

	/**
	 * Clear the iostreams for reuse in a subsequent call
	 */
	virtual void clear()
	{
		i.clear();
		i.flush();
		i.sync();
		o.clear();
		o.flush();
		o.sync();
	}

	void set_func(const std::string& func)
	{
		i << func << '\n';
	}

	template<typename T>
	void add_param(const T& t);

	// Call

	std::string get_func()
	{
		std::string func;
		std::getline(i, func);
		return func;
	}

	template<typename T>
	T get_param();

	void set_return_value()
	{
		o << '\n';
	}

	template<typename T>
	void set_return_value(const T& t);


	template<typename T>
	void add_return_param(const T& t);

	// Post call

	template<typename T>
	T get_return_value();

	template<typename T>
	void get_return_param(T& t);
};

struct local_call
: public call
{
	std::stringstream i;
	std::stringstream o;

	local_call(): call(i, o) {}
	virtual ~local_call() {}

	virtual void clear()
	{
		call::clear();
		i.str("");
		o.str("");
	}
};

struct remote_call
: public call
{
	net::socketstream i;
	net::socketstream o;

	remote_call(): call(i, o) {}
	virtual ~remote_call() {}
};

/**
 * Remote Procedure Call interface
 */
class service
{
public:
	virtual ~service() {}
	virtual bool rpc(call& c) = 0;
};

template<typename T>
std::istream& operator>>(std::istream& is, std::vector<T>& v);

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v);

}} // sookee::rpc

#include "bits/rpc.tcc"

#endif // _SOOKEE_RPC_H_
