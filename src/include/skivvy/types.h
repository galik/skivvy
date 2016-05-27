#pragma once
#ifndef _SKIVVY_TYPES_H__
#define _SKIVVY_TYPES_H__
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
#include <chrono>

#include <sookee/types.h>

namespace skivvy { namespace types {

using namespace sookee::types;

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

#endif /* _SKIVVY_TYPES_H__ */
