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

#include <map>
#include <set>
#include <deque>
#include <vector>
#include <string>
#include <iostream>

#include <skivvy/rpc.h>
#include <skivvy/logrep.h>

template<typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::pair<K, V>& p)
{
	os << p.first << '\n';
	os << p.second << '\n';
	return os;
}
template<typename K, typename V>
std::istream& operator>>(std::istream& is, std::pair<K, V>& p)
{
	inputter(is, p.first);
	inputter(is, p.first);
	return is;
}


template<typename Param>
Param copy_container(const Param& p, Param& rp)
{
	bug_func();
	Param ret = p;
	rp = p;
	return ret;
}

typedef std::vector<int> int_vector;
typedef std::vector<std::string> str_vector;

template<typename Param>
bool rpc(call& c)
{
	bug_func();
	std::string func = c.get_func();
	if(func == "copy_container")
	{
		Param p = c.get_param<Param>();
		Param rp = c.get_param<Param>();
		Param ret = copy_container<Param>(p, rp);
		c.set_return_value(ret);
		c.add_return_param(rp);
		return true;
	}
	return false;
}

template<typename Param>
Param rpc_copy_container(const Param& p, Param& rp)
{
	bug_func();
	local_call c;
	c.set_func("copy_container");
	c.add_param(p);
	c.add_param(rp);
	rpc<Param>(c);
	Param ret = c.get_return_value<Param>();
	c.get_return_param(rp);
	return ret;
}

typedef std::map<int, std::string> int_str_map;
typedef std::pair<int, std::string> int_str_pair;
typedef std::map<std::string, int> str_int_map;
typedef std::pair<std::string, int> str_int_pair;

int main()
{



//	int_vector v1 {1, 2, 3}, rv1, retv1;
//	retv1 = rpc_copy_container(v1, rv1);
//	bug("v1 == rv1: " << (v1 == rv1));
//	bug("v1 == retv1: " << (v1 == retv1));
//
//	str_vector v2 {"1", "2", "3"}, rv2, retv2;
//	retv2 = rpc_copy_container(v2, rv2);
//	bug("v2 == rv2: " << (v2 == rv2));
//	bug("v2 == retv2: " << (v2 == retv2));

//	int_str_map m1 {int_str_pair{1, "1"}, int_str_pair{3, "3"}, int_str_pair{3, "3"}}, rm1, retm1;
//	retm1 = rpc_copy_container(m1, rm1);
//	bug("m1 == rm1: " << (m1 == rm1));
//	bug("m1 == retm1: " << (m1 == retm1));

//	str_int_map m2 {str_int_pair{"1", 1}, str_int_pair{"3", 3}, str_int_pair{"3", 3}}, rm2, retm2;
//	retm1 = rpc_copy_container(m2, rm2);
//	bug("m2 == rm2: " << (m2 == rm2));
//	bug("m2 == retm2: " << (m2 == retm2));

	int_str_pair p1{4, "9"}, p2{2, "7"};
	std::cout << p1 << '\n';
	std::cout << p2 << '\n';

	int_str_map m {p1, p2};
	std::cout << m << '\n';

	rpc_copy_container(m, m);

	return 0;
}
