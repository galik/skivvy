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
#include <fnmatch.h>

#include <skivvy/stl.h>

namespace skivvy { namespace utils {

using namespace skivvy::types;

class Store
{
protected:
//	str_vec_map store;

	bool pcre_match(const str& r, const str& s, bool full = false)
	{
		if(full)
			return pcrecpp::RE(r).FullMatch(s);
		return pcrecpp::RE(r).PartialMatch(s);
	}

	bool wild_match(const str& w, const str& s, int flags = 0)
	{
		return !fnmatch(w.c_str(), s.c_str(), flags | FNM_EXTMATCH);
	}

public:
//	const str_vec_map& get_map()
//	{
//		return store;
//	}
//
	str get(const str& k, const str& dflt = "")
	{
		return get_at(k, 0, dflt);
	}

	template<typename T>
	T get_at(const str& k, siz n, const T& dflt = T())
	{
		T t;
		sss s;
		s << dflt;
		std::istringstream(get_at(k, n, s.str())) >> std::boolalpha >> t;
		return t;
	}

	template<typename T>
	T get(const str& k, const T& dflt = T())
	{
		return get_at(k, 0, dflt);
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
		soss oss;
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

	virtual str_set get_keys_if_pcre(const str& r) = 0;
	virtual str_set get_keys_if_wild(const str& w) = 0;

	/**
	 * Return a set of all keys
	 */
	virtual str_set get_keys() = 0;

	virtual bool has(const str& k) = 0;

	virtual str get_at(const str& k, siz n, const str& dflt = "") = 0;

	virtual str_vec get_vec(const str& k) = 0;
//	str_set get_set(const str& k)
//	{
//		str_set set;
//		return stl::copy(get_vec(k), std::inserter(set, set.begin()));
//		return set;
//	}

	/**
	 * Clear entire store.
	 */
	virtual void clear() = 0;

	/**
	 * Clear entire key.
	 */
	virtual void clear(const str& k) = 0;

	/**
	 * Add v to end of key vector.
	 */
	virtual void add(const str& k, const str& v) = 0;

	/**
	 * Set nth element of key vector.
	 */
	virtual void set_at(const str& k, siz n, const str& v) = 0;
};

class MappedStore
: public Store
{
protected:
	str_vec_map store;

};

class BackupStore
: public MappedStore
{
private:
	std::mutex mtx;
	const str file;

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

public:
	BackupStore(const str& file): file(file) { reload(); }

	void reload()
	{
		lock_guard lock(mtx);
		load();
	}

	str_set get_keys_if_pcre(const str& reg)
	{
		str_set res;
		lock_guard lock(mtx);
		for(const str_vec_pair& p: store)
			if(pcre_match(reg, p.first))
				res.insert(p.first);
		return res;
	}

	str_set get_keys_if_wild(const str& wld)
	{
		str_set res;
		lock_guard lock(mtx);
		for(const str_vec_pair& p: store)
			if(wild_match(p.first, wld))
				res.insert(p.first);
		return res;
	}

	str_set get_keys()
	{
		str_set res;
		lock_guard lock(mtx);
		for(const str_vec_pair& p: store)
			res.insert(p.first);
		return res;
	}

	bool has(const str& k)
	{
		return store.find(k) != store.end();
	}

	str get_at(const str& k, siz n, const str& dflt = "")
	{
		lock_guard lock(mtx);
		if(store.empty())
			load();
		if(store[k].size() < n + 1)
			return dflt;
		return store[k][n];
	}

	str_vec get_vec(const str& k)
	{
		lock_guard lock(mtx);
		if(store.empty())
			load();
		return store[k];
	}

	/**
	 * Clear entire store.
	 */
	void clear()
	{
		lock_guard lock(mtx);
		store.clear();
		save();
	}

	/**
	 * Clear entire key.
	 */
	void clear(const str& k)
	{
		lock_guard lock(mtx);
		store[k].clear();
		save();
	}

	/**
	 * Add v to end of key vector.
	 */
	void add(const str& k, const str& v)
	{
		lock_guard lock(mtx);
		store[k].push_back(v);
		save();
	}

	/**
	 * Set nth element of key vector.
	 */
	void set_at(const str& k, siz n, const str& v)
	{
		lock_guard lock(mtx);
		if(store[k].size() < n + 1)
			store[k].resize(n + 1);
		store[k][n] = v;
		save();
	}
};

class CacheStore
: public MappedStore
{
private:
	std::mutex mtx;
	const str file;
	const siz max;

	static sifs ifs;
	static sofs ofs;

	void load(const str& key)
	{
		static str line, k, v;
		static siss iss;

		if(store.size() > max)
			store = str_vec_map();

		ifs.open(file);

		if(ifs)
		{
			store[k].clear();
			while(sgl(ifs, line))
			{
				iss.clear();
				iss.str(line);
				k.clear();
				v.clear();
				sgl(sgl(iss, k, ':') >> std::ws, v);
				if(!k.empty() && k == key)
					store[k].push_back(v);
			}
		}
		ifs.close();
	}

	void save(const str& key)
	{
		static sss ss;

		ifs.open(file);

		str line;
		while(sgl(ifs, line))
			if(sgl(siss(line), line, ':') && line != key)
				ss << line << '\n';

		ifs.close();
		ofs.open(file);

		while(sgl(ss, line))
			ofs << line << '\n';

		for(const str& line: store[key])
			ofs << key << ": " << line << '\n';

		ofs.close();
	}


public:
	CacheStore(const str& file, siz max = 0):file(file), max(max) {}

	str_set get_keys_if_pcre(const str& reg)
	{
		str_set res;
		lock_guard lock(mtx);

		ifs.open(file);
		str line;
		while(sgl(ifs, line))
			if(sgl(siss(line), line, ':') && pcre_match(reg, line))
				res.insert(line);
		ifs.close();

		return res;
	}

	str_set get_keys()
	{
		str_set res;
		lock_guard lock(mtx);

		ifs.open(file);
		str line;
		while(sgl(ifs, line))
			if(sgl(siss(line), line, ':'))
				res.insert(line);
		ifs.close();

		return res;
	}

	bool has(const str& k)
	{
		bool res = false;
		lock_guard lock(mtx);
		ifs.open(file);
		str line;
		while(sgl(ifs, line))
			if(!res && sgl(siss(line), line, ':') && line == k)
				res = true;
		ifs.close();
		return res;
	}

	str get_at(const str& k, siz n, const str& dflt = "")
	{
		lock_guard lock(mtx);
		if(store.find(k) == store.end())
			load(k);
		if(store[k].size() < n + 1)
			return dflt;
		return store[k][n];
	}

	str_vec get_vec(const str& k)
	{
		lock_guard lock(mtx);
		if(store.find(k) == store.end())
			load(k);
		return store[k];
	}

	/**
	 * Clear entire store.
	 */
	void clear()
	{
		lock_guard lock(mtx);
		store.clear();
		ofs.open(file, std::ios::trunc);
		ofs.close();
	}

	/**
	 * Clear entire key.
	 */
	void clear(const str& k)
	{
		lock_guard lock(mtx);
		store[k].clear();
		save(k);
	}

	/**
	 * Add v to end of key vector.
	 */
	void add(const str& k, const str& v)
	{
		lock_guard lock(mtx);
		store[k].push_back(v);
		save(k);
	}

	/**
	 * Set nth element of key vector.
	 */
	void set_at(const str& k, siz n, const str& v)
	{
		lock_guard lock(mtx);
		if(store[k].size() < n + 1)
			store[k].resize(n + 1);
		store[k][n] = v;
		save(k);
	}
};


class FileStore
: public Store
{
private:
	std::mutex mtx;
	const str file;

	static sifs ifs;
	static sofs ofs;
	static sss ss;
	static str line, k, v;

	str_vec load(const str& key)
	{
		str_vec store;

		ifs.open(file);

		while(sgl(ifs, line))
		{
			ss.clear();
			ss.str(line);
			k.clear();
			v.clear();
			if(sgl(sgl(ss, k, ':') >> std::ws, v))
				if(k == key)
					store.push_back(v);
		}
		ifs.close();
		return store;
	}

	void save(const str& key, const str_vec& store)
	{
		ifs.open(file);

		ss.clear();
		ss.str("");

		str line;
		while(sgl(ifs, line))
			if(sgl(siss(line), line, ':') && line != key)
				ss << line << '\n';

		ifs.close();
		ofs.open(file);

		while(sgl(ss, line))
			ofs << line << '\n';

		for(const str& line: store)
			ofs << key << ": " << line << '\n';

		ofs.close();
	}


public:
	FileStore(const str& file):file(file) {}

	str_set get_keys_if_pcre(const str& reg)
	{
		str_set res;
		lock_guard lock(mtx);

		ifs.open(file);
		while(sgl(ifs, line))
			if(sgl(siss(line), line, ':') && pcre_match(reg, line))
				res.insert(line);
		ifs.close();

		return res;
	}

	str_set get_keys()
	{
		str_set res;
		lock_guard lock(mtx);

		ifs.open(file);
		str line;
		while(sgl(ifs, line))
			if(sgl(siss(line), line, ':'))
				res.insert(line);
		ifs.close();

		return res;
	}

	bool has(const str& k)
	{
		bool res = false;
		lock_guard lock(mtx);
		ifs.open(file);
		while(sgl(ifs, line))
			if(!res && sgl(siss(line), line, ':') && line == k)
				res = true;
		ifs.close();
		return res;
	}

	str get_at(const str& k, siz n, const str& dflt = "")
	{
		lock_guard lock(mtx);
		str_vec store = load(k);
		if(store.size() < n + 1)
			return dflt;
		return store[n];
	}

	str_vec get_vec(const str& k)
	{
		lock_guard lock(mtx);
		return load(k);
	}

	/**
	 * Clear entire store.
	 */
	void clear()
	{
		lock_guard lock(mtx);
		ofs.open(file, std::ios::trunc);
		ofs.close();
	}

	/**
	 * Clear entire key.
	 */
	void clear(const str& k)
	{
		lock_guard lock(mtx);
		str_vec store;
		save(k, store);
	}

	/**
	 * Add v to end of key vector.
	 */
	void add(const str& k, const str& v)
	{
		lock_guard lock(mtx);
		str_vec store = load(k);
		store.push_back(v);
		save(k, store);
	}

	/**
	 * Set nth element of key vector.
	 */
	void set_at(const str& k, siz n, const str& v)
	{
		lock_guard lock(mtx);
		str_vec store = load(k);
		if(store.size() < n + 1)
			store.resize(n + 1);
		store[n] = v;
		save(k, store);
	}
};
}} // skivvy::utils

#endif // _SKIVVY_STORE_H_
