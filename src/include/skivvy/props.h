#pragma once
#ifndef _SKIVVY_PROPERTIES_H_
#define _SKIVVY_PROPERTIES_H_
/*
 * properties.h
 *
 *  Created on: 15 Jun 2012
 *      Author: grafterman@googlemail.com
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

#include <hol/string_utils.h>
#include <hol/small_types.h>
#include <skivvy/logrep.h>

#include <thread>
#include <mutex>

namespace skivvy { namespace props {

using namespace hol::small_types::basic;
using namespace skivvy::utils;

using lock_guard = std::lock_guard<std::mutex>;

typedef std::map<const str, str> property_map;
typedef std::pair<const str, str> property_pair;
//typedef property_map::iterator property_iter;
//typedef property_map::const_iterator property_citer;

template<typename T>
struct serialize
{
	str operator()(const T& t)
	{
		std::ostringstream oss;
		oss << t;
		return oss.str();
	}
};

class transaction;

class properties
{
protected:

//	typedef std::map<const str, str> property_map;
//	typedef std::pair<const str, str> property_pair;
//	typedef property_map::iterator property_iter;
//	typedef property_map::const_iterator property_citer;

private:
	mutable std::mutex mtx;
	property_map m; // complete
	property_map t; // transaction

public:

	virtual transaction get_transaction();

	virtual str get(const str& s, const str& dflt = "") const
	{
		bug_fun();

		decltype(t.cbegin()) ci;

		lock_guard lock(mtx);

		if((ci = t.find(s)) != t.cend())
			return ci->second;
		if((ci = m.find(s)) != m.cend())
			return ci->second;
		return dflt;
	}

	virtual void set(const str& s, const str& v)
	{
		bug_fun();
		lock_guard lock(mtx);
		t[s] = v;
	}

private:
	friend class transaction;

	virtual void commit()
	{
		lock_guard lock(mtx);
		for(const property_pair& p: t)
			m[p.first] = p.second;
		t.clear();
	}

	virtual void rollback()
	{
		lock_guard lock(mtx);
		t.clear();
	}
};

class transaction
{
	properties& props;

public:
	transaction(const transaction& tx): props(tx.props) {}
	transaction(properties& props): props(props) {}
	~transaction() { props.commit(); }

	template<typename T>
	T get(const str& s, const T& dflt = T()) const
	{
		bug_fun();
		T t;
		if(std::istringstream(props.get(s)) >> std::boolalpha >> t)
			return t;
		return dflt;
	}

	template<typename T>
	bool convert(const str& s, T& t) const
	{
		return std::istringstream(s) >> t;
	}

	template<typename Container>
	Container get_c(const str& s, const Container& dflt = Container()) const
	{
		bug_fun();
		str line;
		Container c;
		typename Container::value_type t;
		std::insert_iterator<Container> ii(c, c.end());
		std::istringstream iss(props.get(s));
		while(std::getline(iss, line))
			if(convert(line, t))
				ii = t;
		return c;
	}

	str get(const str& s, const str& dflt = "") const { return props.get(s, dflt); }

	template<typename T>
	void set(const str& s, const T& t)
	{
		bug_fun();
		std::ostringstream oss;
		oss << t;
		props.set(s, oss.str());
	}

	void set(const str& s, const str& v) { props.set(s, v); }
};

transaction properties::get_transaction() { return transaction(*this); }

//class transaction
//{
//	properties& props;
//	std::thread::id id;
//
//public:
//	transaction(properties& props): props(props), id(std::this_thread::get_id()) {}
//	//~transaction() { props.unroll(id); }
//	void commit()
//	{
//		props.commit(id);
//	}
//};


//class persistant_properties
//: public properties
//{
//	const str filename;
//	std::mutex mtx;
//
//public:
//	persistant_properties(const str& filename): filename(filename) {}
//
//	virtual str get(const str& s, const str& dflt = "") const
//	{
//		str line, key, val;
//		lock_guard lock(mtx);
//		std::ifstream ifs(filename);
//		while(std::getline(ifs, line))
//			if(std::getline(std::getline(std::istringstream(line), key, ':') >> std::ws, val) && key == s)
//				return val;
//		return dflt;
//	}
//};

}} // skivvy::prop

#endif // _SKIVVY_PROPERTIES_H_
