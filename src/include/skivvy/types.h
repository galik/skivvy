#pragma once
#ifndef _SOOKEE_TYPES_H__
#define _SOOKEE_TYPES_H__
/*
 * tyoes.h
 *
 *  Created on: 9 Jan 2012
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

#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <memory>
#include <cstdlib>

#include <map>
#include <set>
#include <deque>
#include <mutex>
#include <stack>
#include <vector>
#include <random>
#include <chrono>
#include <istream>
#include <utility>

namespace skivvy { namespace types {

typedef std::size_t siz;

typedef std::string str;
typedef str::iterator str_iter;
typedef str::const_iterator str_citer;

typedef std::vector<int> int_vec;
typedef std::vector<siz> siz_vec;

typedef std::vector<str> str_vec;
typedef str_vec::iterator str_vec_itr;
typedef str_vec::const_iterator str_vec_citr;

typedef std::set<str> str_set;
typedef str_set::const_iterator str_set_citer;

typedef std::map<str, time_t> str_time_map;
typedef std::pair<const str, time_t> str_time_pair;


typedef std::multiset<str> str_mset;
typedef std::map<str, str> str_map;
typedef std::multimap<str, str> str_mmap;
typedef std::pair<const str, str> str_pair;
typedef std::deque<str> str_deq;

typedef std::map<str, siz> str_siz_map;
typedef std::pair<const str, siz> str_siz_pair;

typedef std::map<str, str_set> str_set_map;
typedef std::pair<const str, str_set> str_set_pair;

typedef std::map<const str, str_vec> str_vec_map;
typedef std::pair<const str, str_vec> str_vec_pair;

typedef std::lock_guard<std::mutex> lock_guard;
//typedef std::chrono::steady_clock steady_clock;
//typedef steady_clock::period period;
//typedef steady_clock::time_point time_point;

// time
typedef std::chrono::steady_clock st_clk;
typedef st_clk::period st_period;
typedef st_clk::time_point st_time_point;

typedef std::chrono::high_resolution_clock hr_clk;
typedef hr_clk::period hr_period;
typedef hr_clk::time_point hr_time_point;

typedef std::stringstream sss;
typedef std::istringstream siss;
typedef std::ostringstream soss;

typedef std::fstream sfs;
typedef std::ifstream sifs;
typedef std::ofstream sofs;

typedef std::stringstream sss;

inline
std::istream& sgl(std::istream& is, str& s, char d = '\n')
{
	return std::getline(is, s, d);
}

inline
std::istream& sgl(std::istream&& is, str& s, char d = '\n')
{
	return sgl(is, s, d);
}


struct delay
{
	siz d;
	delay(siz d = 0): d(d) {}
	delay(const str& s) : d(0) { parse(s); }
//	delay(const delay& d): d(d.d) {}
	std::istream& parse(std::istream& is);

	bool parse(const str& s);

	operator siz() const { return d; }
	operator int() const { return int(d); }

	bool operator<(const delay& d) const { return this->d < d.d; }
	bool operator<(int i) const { return (int)this->d < i; }
	bool operator<(siz i) const { return this->d < i; }

	friend std::ostream& operator<<(std::ostream& os, const delay& d)
	{
//		case 's': d *= 1; break;
//		case 'm': d *= 60; break;
//		case 'h': d *= 60 * 60; break;
//		case 'd': d *= 60 * 60 * 24; break;
//		case 'w': d *= 60 * 60 * 24 * 7; break;

		if(double(d.d / (60 * 60 * 24 * 7)) == double(d.d) / (60 * 60 * 24 * 7))
			os << (d.d / (60 * 60 * 24 * 7)) << "w";
		else if(double(d.d / (60 * 60 * 24)) == double(d.d) / (60 * 60 * 24))
			os << (d.d / (60 * 60 * 24)) << "d";
		else if(double(d.d / (60 * 60)) == double(d.d) / (60 * 60))
			os << (d.d / (60 * 60)) << "h";
		else if(double(d.d / (60)) == double(d.d) / (60))
			os << (d.d / (60)) << "m";
		else
			os << d.d << "s";

		return os;
	}

	friend std::istream& operator>>(std::istream& is, delay& d)
	{
		return d.parse(is);
	}
};

struct malloc_deleter
{
	template <class T>
	void operator()(T* p) { std::free(p); }
};

typedef std::unique_ptr<char, malloc_deleter> cstring_uptr;

}} // sookee::types

#endif /* using namespace skivvy::types;
 */
