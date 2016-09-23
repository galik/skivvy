#ifndef SKIVVY_STORE_H
#define SKIVVY_STORE_H
/*
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

#include <hol/small_types.h>

#include <hol/bug.h>
#include <hol/basic_serialization.h>
#include <hol/simple_logger.h>
#include <range/v3/all.hpp>
#include <hol/string_utils.h>
//#include <sookee/ios.h>

#include <memory>
#include <istream>
#include <ostream>
#include <fstream>
#include <sstream>

#include <fnmatch.h>

namespace skivvy {
namespace utils {

using namespace hol::small_types::ios;
using namespace hol::small_types::ios::functions;
using namespace hol::small_types::basic;
using namespace hol::small_types::string_containers;
using namespace hol::simple_logger;

using lock_guard = std::lock_guard<std::mutex>;

// Container serialization

// prototypes

template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::pair<K, V>& p);

// [{pair1}{pair2}]
template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& m);

// {{item1}{item2}}
template<typename K, typename V>
std::istream& operator>>(std::istream& is, std::pair<K, V>& p);

// {pair1pair2}
template<typename K, typename V>
std::istream& operator>>(std::istream& is, std::map<K, V>& m);

template<typename T>
std::istream& operator>>(std::istream& is, std::set<T>& set);

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& set);

template<typename T>
std::istream& operator>>(std::istream& is, std::vector<T>& vec);

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec);

std::istream& getobject(std::istream& is, str& o);

template<typename T>
void escape(T&) {}
void escape(str& s);

template<typename T>
void unescape(T&) {}
void unescape(str& s);

str escaped(const str& cs);
str unescaped(const str& cs);

// {{value_type1}{value_type2}}
template<typename Container>
std::ostream& write_container(std::ostream& os, const Container& container)
{
//	bug_fun();
	os << '{';
	str s;
	for(auto i: container)
	{
		sss ss;
		ss << i;
		sgl(ss, s);
		//bug_var(s);
		escape(s);
		os << '{' << s << '}';
	}
	os << '}';
	return os;
}

template<typename T>
std::istream& extract(std::istream& is, T& t)
{
//	bug_fun();
	return is >> t;
}

template<typename T>
std::istream& extract(std::istream&& is, T& t)
{
//	bug_fun();
	return extract(is, t);
}

std::istream& extract(std::istream& is, str& s);

std::istream& extract(std::istream&& is, str& s);

// {{value_type1}{value_type2}}
template<typename Container>
std::istream& read_container(std::istream& is, Container& container)
{
//	bug_fun();
	container.clear();
	str skip, line;
	if(!getobject(is, line))
		is.clear(is.rdstate() | std::ios::failbit); // protocol error
	else
	{
		// {value_type1}{value_type2}
		siss iss(line);
		while(getobject(iss, line))
		{
			typename Container::value_type t;
			extract(siss(line), t);
			std::insert_iterator<Container>(container, container.end()) = t;
		}
	}
	return is;
}


// {key,val)
template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::pair<K, V>& p)
{
//	bug_fun();
	K key;
	V val;
	key = p.first;
	val = p.second;
	escape(key);
	escape(val);
	return os << "{{" << key << "}{" << val << "}}";
}

// [{pair1}{pair2}]
template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::map<K, V>& m)
{
//	bug_fun();
	os << "{";
	for(const std::pair<K, V>& p: m)
		os << p;
	os << "}";
	return os;
}

// {{item1}{item2}}
template<typename K, typename V>
std::istream& operator>>(std::istream& is, std::pair<K, V>& p)
{
//	bug_fun();
	str skip, line, key, val;
	if(!getobject(is, line))
		is.clear(is.rdstate() | std::ios::failbit); // protocol error
	else
	{
		siss iss(line);
		if(!getobject(getobject(iss, key), val))
			is.clear(is.rdstate() | std::ios::failbit); // protocol error
		else
		{
			iss.clear();
			iss.str(key);
			extract(iss, p.first);
			iss.clear();
			iss.str(val);
			extract(iss, p.second);
		}
	}
	return is;
}

// {pair1pair2}
template<typename K, typename V>
std::istream& operator>>(std::istream& is, std::map<K, V>& m)
{
//	bug_fun();
	str skip, line;
	if(!getobject(is, line))
		is.clear(is.rdstate() | std::ios::failbit); // protocol error
	else
	{
		m.clear();
		//bug_var(line);
		siss iss(line);
		std::pair<K, V> p;
		while(iss >> p)
			m.insert(p);
	}
	return is;
}

template<typename T>
std::istream& operator>>(std::istream& is, std::set<T>& set)
{
	return read_container(is, set);
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& set)
{
	return write_container(os, set);
}

template<typename T>
std::istream& operator>>(std::istream& is, std::vector<T>& vec)
{
	return read_container(is, vec);
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec)
{
	return write_container(os, vec);
}

class ci_cmp_type
{
public:
	ci_cmp_type(bool ci): ci(ci) {}

	bool operator()(str const& a, str const& b) const
	{
		if(ci)
			return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), ci_type());
		return a < b;
	}

private:
	struct ci_type
	{
		bool operator()(char a, char b) const
		{
			return std::tolower(a) < std::tolower(b);
		}
	};

	bool ci;
};

// TODO: replace ci_fix(a) == ci_fix(b) with ci_equals(a, b)
// TODO: replace ci_fix(a) != ci_fix(b) with ci_not_equals(a, b)

class Store
{
public:
	using UPtr = std::unique_ptr<Store>;

protected:

	str ci_fix(str s) const { if(case_insensitive()) return hol::lower_mute(s); return s; }

	bool sreg_match(const str& r, const str& s, bool full = false)
	{
		using namespace std::regex_constants;

		syntax_option_type opts = ECMAScript;

		if(case_insensitive())
			opts |= icase;

		std::regex e(r, opts);

		if(full)
			return std::regex_match(s, e);
		return std::regex_search(s, e);
	}

	bool wild_match(const str& w, const str& s, int flags = 0)
	{
		if(case_insensitive())
			flags |= FNM_CASEFOLD;

		return !fnmatch(w.c_str(), s.c_str(), flags | FNM_EXTMATCH);
	}

public:
	virtual ~Store() {}

	virtual bool case_insensitive() const = 0; // { return ci; }
	virtual void case_insensitive(bool ci) = 0; // { this->ci = ci; }

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
		soss oss;
		oss <<  std::boolalpha << v;
		set_at(k, n, oss.str());
	}

	template<typename T>
	void set(const str& k, const T& v)
	{
		set_at(k, 0, v);
	}


	virtual str_set get_keys_if_sreg(const str& r) = 0;
	virtual str_set get_keys_if_wild(const str& w) = 0;

	/**
	 * Return a set of all keys
	 */
	virtual str_set get_keys() = 0;

	virtual bool has(const str& k) = 0;

	virtual str get_at(const str& k, siz n, const str& dflt = "") = 0;

	virtual str_vec get_vec(const str& k) = 0;

	str_set get_set(const str& k)
	{
		str_vec vec = get_vec(k);
		str_set set(vec.begin(), vec.end());
		return set;
	}

	/**
	 * Return values for key that match std::regex
	 * @param k Key
	 * @param r Regex to match against values
	 * @return str_vec of matching values
	 */
	str_vec get_sreg_vec_with_key(const str& k, const str& r)
	{
		str_vec res;
		for(const auto& v: get_vec(k))
			if(sreg_match(r, v, true))
				res.push_back(v);
		return res;
	}

	/**
	 * Return values for key that match wildcard
	 * @param k Key
	 * @param w Wildcard to match against values
	 * @return str_vec of matching values
	 */
	str_vec get_wild_vec_with_key(const str& k, const str& w)
	{
		str_vec res;
		for(const auto& v: get_vec(k))
			if(wild_match(w, v, true))
				res.push_back(v);
		return res;
	}

	/**
	 * Set each value from iterator [first, last) against key k, clearing any current keys.
	 */
	template<typename InputIter>
	void set_from(const str& k, InputIter first, InputIter last)
	{
		clear(k);
		while(first != last)
			add(k, *first++);
	}

	/**
	 * Set each value in container against key k, clearing any current keys.
	 */
	template<typename Container>
	void set_from(const str& k, const Container& c)
	{
		clear(k);
		for(const auto &v: c)
			add(k, v);
	}

	template<typename Container>
	Container get_to(const str& k)
	{
		Container c;
		typename Container::value_type t;
		str_vec vec = get_vec(k);
		for(const str& s: vec)
		{
			siss(s) >> t;
			typename Container::iterator ci = c.begin();
			std::insert_iterator<Container> insert_it(c, ci);
			insert_it = t;
		}
		return c;
	}

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

private:
//	bool ci = false;
};

using StoreUPtr = std::unique_ptr<Store>;

//using str_vec_map = std::map<str, str_vec>;
using str_vec_map = std::map<str, str_vec, ci_cmp_type>;

class MappedStore
: public Store
{
public:
	MappedStore(): store(ci_cmp_type(false)), ci(false) {}

	bool case_insensitive() const override
	{
		return ci;
	}

	void replace_store(str_vec_map&& store)
	{
		for(auto const& p: this->store)
			store[p.first] = p.second;
		this->store.swap(store);
	}

	void case_insensitive(bool ci) override
	{
		if(this->ci == ci)
			return;

		replace_store(str_vec_map{ci_cmp_type{ci}});

		this->ci = ci;
	}

protected:
	str_vec_map store;

private:
	bool ci;
};

class BackupStore
: public MappedStore
{
private:
	static const str VERSION_KEY;

	std::unique_lock<std::mutex> move_lock;
	mutable std::mutex mtx;
	const str file;

	void load()
	{
//		decltype(store)().swap(store);
		store.clear();
		if(auto ifs = std::ifstream(file))
			for(std::string line; std::getline(ifs, line);)
				if(auto pos = line.find(':') + 1)
					store[line.substr(0, pos - 1)].push_back(
						line.substr(std::min(pos + 1, line.size())));
	}

	void save()
	{
		if(auto ofs = std::ofstream(file))
		{
			for(const auto& p: store)
				for(const str& v: p.second)
					ofs << p.first << ": " << v << '\n';
		}
		else
			throw std::runtime_error("Unable to save store: " + file + " " + std::strerror(errno));
	}

public:
	BackupStore(BackupStore&& bs)
	: move_lock(bs.mtx), file(std::move(bs.file))
	{
		move_lock.unlock();
	}

	BackupStore(const str& file): file(file)
	{
		reload();
		try{save();}catch(...){throw std::runtime_error(std::strerror(errno));}
	}

	void reload()
	{
		lock_guard lock(mtx);
		load();
	}

	str_set get_keys_if_sreg(const str& reg) override
	{
		str_set res;
		lock_guard lock(mtx);
		if(store.empty())
			load();
		for(const auto& p: store)
			if(sreg_match(reg, p.first))
				res.insert(p.first);
		return res;
	}

	str_set get_keys_if_wild(const str& wld) override
	{
		str_set res;
		lock_guard lock(mtx);
		if(store.empty())
			load();
		for(const auto& p: store)
			if(wild_match(wld, p.first))
				res.insert(p.first);
		return res;
	}

	str_set get_keys() override
	{
		str_set res;
		lock_guard lock(mtx);
		if(store.empty())
			load();
		for(const auto& p: store)
			res.insert(p.first);
		return res;
	}

	bool has(const str& k) override
	{
		if(store.empty())
			load();
		return store.find(k) != store.end();
	}

	str get_at(const str& k, siz n, const str& dflt = "") override
	{
		lock_guard lock(mtx);
		if(store.empty())
			load();
		if(store[k].size() < n + 1)
			return dflt;
		return store[k][n];
	}

	str_vec get_vec(const str& k) override
	{
		lock_guard lock(mtx);
		if(store.empty())
			load();
		return store[k];
	}

	/**
	 * Clear entire store.
	 */
	void clear() override
	{
		lock_guard lock(mtx);
		store.clear();
		save();
	}

	/**
	 * Clear entire key.
	 */
	void clear(const str& k) override
	{
		lock_guard lock(mtx);
		if(store.empty())
			load();
		store[k].clear();
		save();
	}

	/**
	 * Add v to end of key vector.
	 */
	void add(const str& k, const str& v) override
	{
		lock_guard lock(mtx);
		if(store.empty())
			load();
		store[k].push_back(v);
		save();
	}

	/**
	 * Set nth element of key vector.
	 */
	void set_at(const str& k, siz n, const str& v) override
	{
		lock_guard lock(mtx);
		if(store.empty())
			load();
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
			store.clear();// = str_vec_map();

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
				if(!k.empty() && ci_fix(k) == ci_fix(key))
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
			if(sgl(siss(line), line, ':') && ci_fix(line) != ci_fix(key))
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

	str_set get_keys_if_sreg(const str& reg) override
	{
		str_set res;
		lock_guard lock(mtx);

		ifs.open(file);
		str line;
		while(sgl(ifs, line))
			if(sgl(siss(line), line, ':') && sreg_match(reg, line))
				res.insert(line);
		ifs.close();

		return res;
	}

	str_set get_keys() override
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

	bool has(const str& k) override
	{
		bool res = false;
		lock_guard lock(mtx);
		ifs.open(file);
		str line;
		while(sgl(ifs, line))
			if(!res && sgl(siss(line), line, ':') && ci_fix(line) == ci_fix(k))
				res = true;
		ifs.close();
		return res;
	}

	str get_at(const str& k, siz n, const str& dflt = "") override
	{
		lock_guard lock(mtx);
		if(store.find(k) == store.end())
			load(k);
		if(store[k].size() < n + 1)
			return dflt;
		return store[k][n];
	}

	str_vec get_vec(const str& k) override
	{
		lock_guard lock(mtx);
		if(store.find(k) == store.end())
			load(k);
		return store[k];
	}

	/**
	 * Clear entire store.
	 */
	void clear() override
	{
		lock_guard lock(mtx);
		store.clear();
		ofs.open(file, std::ios::trunc);
		ofs.close();
	}

	/**
	 * Clear entire key.
	 */
	void clear(const str& k) override
	{
		lock_guard lock(mtx);
		store[k].clear();
		save(k);
	}

	/**
	 * Add v to end of key vector.
	 */
	void add(const str& k, const str& v) override
	{
		lock_guard lock(mtx);
		store[k].push_back(v);
		save(k);
	}

	/**
	 * Set nth element of key vector.
	 */
	void set_at(const str& k, siz n, const str& v) override
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
				if(ci_fix(k) == ci_fix(key))
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
			if(sgl(siss(line), line, ':') && ci_fix(line) != ci_fix(key))
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

	str_set get_keys_if_sreg(const str& reg) override
	{
		str_set res;
		lock_guard lock(mtx);

		ifs.open(file);
		while(sgl(ifs, line))
			if(sgl(siss(line), line, ':') && sreg_match(reg, line))
				res.insert(line);
		ifs.close();

		return res;
	}

	str_set get_keys() override
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

	bool has(const str& k) override
	{
		bool res = false;
		lock_guard lock(mtx);
		ifs.open(file);
		while(sgl(ifs, line))
			if(!res && sgl(siss(line), line, ':') && ci_fix(line) == ci_fix(k))
				res = true;
		ifs.close();
		return res;
	}

	str get_at(const str& k, siz n, const str& dflt = "") override
	{
		lock_guard lock(mtx);
		str_vec store = load(k);
		if(store.size() < n + 1)
			return dflt;
		return store[n];
	}

	str_vec get_vec(const str& k) override
	{
		lock_guard lock(mtx);
		return load(k);
	}

	/**
	 * Clear entire store.
	 */
	void clear() override
	{
		lock_guard lock(mtx);
		ofs.open(file, std::ios::trunc);
		ofs.close();
	}

	/**
	 * Clear entire key.
	 */
	void clear(const str& k) override
	{
		lock_guard lock(mtx);
		str_vec store;
		save(k, store);
	}

	/**
	 * Add v to end of key vector.
	 */
	void add(const str& k, const str& v) override
	{
		lock_guard lock(mtx);
		str_vec store = load(k);
		store.push_back(v);
		save(k, store);
	}

	/**
	 * Set nth element of key vector.
	 */
	void set_at(const str& k, siz n, const str& v) override
	{
		lock_guard lock(mtx);
		str_vec store = load(k);
		if(store.size() < n + 1)
			store.resize(n + 1);
		store[n] = v;
		save(k, store);
	}
};

} // utils
} // skivvy

#endif // SKIVVY_STORE_H
