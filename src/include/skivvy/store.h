#pragma once
#ifndef _SKIVVY_STORE_H_
#define _SKIVVY_STORE_H_
/*
 * store.h
 *
 *  Created on: 04 Jan 2013
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2012 SooKee oaskivvy@gmail.com               |
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

#include <skivvy/types.h>

#include <istream>
#include <ostream>
#include <fstream>
#include <sstream>

#include <pcrecpp.h>

namespace skivvy { namespace utils {

using namespace skivvy::types;

class Store
{
public:
//	typedef std::map<str, str_vec> property_map;
//	typedef std::pair<const str, str_vec> property_pair;

private:
	const str file;
	str_vec_map store;
	std::mutex store_mtx;

	bool preg_match(const str& s, const str& r, bool full = false)
	{
		if(full)
			return pcrecpp::RE(r).FullMatch(s);
		return pcrecpp::RE(r).PartialMatch(s);
	}

public:
	Store(const str& file): file(file) {}

	str_vec find(const str& reg)
	{
		str_vec res;
		for(const str_vec_pair& p: store)
			if(preg_match(p.first, reg))
				res.push_back(p.first);
		return res;
	}

	const str_vec_map& get_map()
	{
		return store;
	}

	void load()
	{
		static str line, k, v;
		static std::ifstream ifs;
		static std::istringstream iss;

		ifs.open(file);

		if(ifs)
		{
			store.clear();
			while(std::getline(ifs, line))
			{
				iss.clear();
				iss.str(line);
				k.clear();
				v.clear();
				std::getline(std::getline(iss, k, ':') >> std::ws, v);
				if(!k.empty())
					store[k].push_back(v);
			}
		}
		ifs.close();
	}

	void save()
	{
		static std::ofstream ofs;

		ofs.open(file);
		if(ofs)
			for(const str_vec_pair& p: store)
				for(const str& v: p.second)
					ofs << p.first << ": " << v << '\n';
		ofs.close();
	}

	bool has(const str& k)
	{
		return store.find(k) != store.end();
	}

	str get_at(const str& k, siz n, const str& dflt = "")
	{
		lock_guard lock(store_mtx);
		if(store.empty())
			load();
		if(store[k].size() < n + 1)
			return dflt;
		return store[k][n];
	}

	str get(const str& k, const str& dflt = "")
	{
		return get_at(k, 0, dflt);
	}

	template<typename T>
	T get_at(const str& k, siz n, const T& dflt = T())
	{
		lock_guard lock(store_mtx);
		if(store.empty())
			load();
		if(store[k].size() < n + 1)
			return dflt;
		T t;
		std::istringstream(store[k][n]) >> std::boolalpha >> t;
		return t;
	}

	template<typename T>
	T get(const str& k, const T& dflt = T())
	{
		return get_at(k, 0, dflt);
	}

	str_vec get_vec(const str& k)
	{
		lock_guard lock(store_mtx);
		if(store.empty())
			load();
		return store[k];
	}

	/**
	 * Clear entire store.
	 */
	void clear()
	{
		lock_guard lock(store_mtx);
		store.clear();
		save();
	}

	/**
	 * Clear entire key.
	 */
	void clear(const str& k)
	{
		lock_guard lock(store_mtx);
		store[k].clear();
		save();
	}

	/**
	 * Add v to end of key vector.
	 */
	void add(const str& k, const str& v)
	{
		lock_guard lock(store_mtx);
		store[k].push_back(v);
		save();
	}

	/**
	 * Set nth element of key vector.
	 */
	void set_at(const str& k, siz n, const str& v)
	{
		lock_guard lock(store_mtx);
		if(store[k].size() < n + 1)
			store[k].resize(n + 1);
		store[k][n] = v;
		save();
	}

	/**
	 * Set first element of key vector.
	 */
	void set(const str& k, const str& v)
	{
		set_at(k, 0, v);
	}

	template<typename T>
	void add(const str& k, const T& v)
	{
		std::ostringstream oss;
		oss << std::boolalpha << v;
		add(k, oss.str());
	}

	template<typename T>
	void set_at(const str& k, siz n, const T& v)
	{
		std::ostringstream oss;
		oss << v;
		set_at(k, n, oss.str());
	}

	template<typename T>
	void set(const str& k, const T& v)
	{
		set_at(k, 0, v);
	}

	void dump()
	{
		std::cout << "== PRINTING STORE: ====" << '\n';
		lock_guard lock(store_mtx);

		static str line, k, v;
		static std::ifstream ifs;
		static std::istringstream iss;

		ifs.open(file);

		if(ifs)
		{
			siz c = 0;
			while(std::getline(ifs, line))
			{
				iss.clear();
				iss.str(line);
				k.clear();
				v.clear();
				std::getline(std::getline(iss, k, ':') >> std::ws, v);
				if(!k.empty())
					std::cout << k << "[" << (c++) << "]: " << v << '\n';
			}
		}
		ifs.close();
		std::cout << "== DONE ===============" << '\n';
	}
};

}} // skivvy::utils

#endif // _SKIVVY_STORE_H_
