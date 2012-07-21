#pragma once
#ifndef _SOOKEE_RPC_TCC_
#define _SOOKEE_RPC_TCC_
/*
 * rpc.tcc
 *
 *  Created on: 10 Aug 2011
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

#include <skivvy/logrep.h>

#include <map>
#include <list>
#include <array>
#include <vector>

namespace skivvy { namespace rpc {

template<typename T>
void call::add_param(const T& t)
{
	i << t << '\n';
}

template<typename T>
T call::get_param()
{
	T t;
	i >> t;
	std::string line;
	std::getline(i, line);
	return t;
}

template<>
std::string call::get_param<std::string>();

template<typename T>
void call::set_return_value(const T& t)
{
	o << t << '\n';
}

template<typename T>
void call::add_return_param(const T& t)
{
	o << t << '\n';
}

template<typename T>
T call::get_return_value()
{
	T t;
	o >> t;
	std::string line;
	std::getline(o, line);
	return t;
}

template<>
std::string call::get_return_value<std::string>();

template<>
void call::get_return_value<void>();

template<typename T>
void call::get_return_param(T& t)
{
	o >> t;
	std::string line;
	std::getline(o, line);
}

template<>
void call::get_return_param<std::string>(std::string& t);

template<typename T>
std::istream& inputter(std::istream& is, T& t)
{
	is >> t;
	std::string line;
	std::getline(is, line);
	return is;
}

template<>
std::istream& inputter<std::string>(std::istream& is, std::string& t);

template<typename C>
std::istream& container_is(std::istream& is, C& c)
{
	size_t sz;
	is >> sz;
	std::string line;
	std::getline(is, line);
	c.clear();
	typename C::value_type t;
	std::insert_iterator<C> ii(c, c.end());
	for(size_t i = 0; i < sz; ++i)
	{
		if(inputter(is, t))
			ii = t;
	}
	return is;
}

template<typename C>
std::ostream& container_os(std::ostream& os, const C& c)
{
	os << c.size() << '\n';
	for(const typename C::value_type& t: c)
	{
		os << t << '\n';
	}
	return os;
}

//template<typename T>
//std::istream& operator>>(std::istream& is, std::array<T>& a)
//{
//	return container_is(is, a);
//}
//
//template<typename T>
//std::ostream& operator<<(std::ostream& os, const std::array<T>& a)
//{
//	return container_os(os, a);
//}

template<typename T>
std::istream& operator>>(std::istream& is, std::vector<T>& v)
{
	return container_is(is, v);
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
	return container_os(os, v);
}

template<typename T>
std::istream& operator>>(std::istream& is, std::list<T>& l)
{
	return container_is(is, l);
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::list<T>& l)
{
	return container_os(os, l);
}

template<typename T>
std::istream& operator>>(std::istream& is, std::set<T>& s)
{
	return container_is(is, s);
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& a)
{
	return container_os(os, a);
}

template<typename K, typename V>
std::istream& operator>>(std::istream& is, std::map<K, V>& m)
{
	return container_is(is, m);
}

template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& m)
{
	return container_os(os, m);
}

}} // sookee::rpc

#endif // _SOOKEE_RPC_TCC_
