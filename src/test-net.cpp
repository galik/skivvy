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

#include <map>
#include <set>
#include <deque>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#include <sookee/types.h>
#include <skivvy/logrep.h>
#include <skivvy/network.h>

using namespace skivvy;
using namespace sookee::types;

str_vec files
{
	"test-chunk-data-01.txt"
	, "test-chunk-data-02.txt"
	, "test-chunk-data-03.txt"
	, "test-chunk-data-04.txt"
	, "test-chunk-data-05.txt"
};

int main()
{
	net::header_map headers;

	for(const str& file: files)
	{
		std::cout << "file: " << file << '\n';
		std::ifstream ifs(file);
		std::cout << "open: " << bool(ifs) << '\n';
		headers.clear();
		if(net::read_http_headers(ifs, headers))
		{
			for(const net::header_pair& p: headers)
			{
				std::cout << "header: " << p.first << " = " << p.second << '\n';
			}
			net::header_map::const_iterator found = headers.find("transfer-encoding");
			std::cout << "found: " << found->first << " = " << found->second << '\n';
			if(found != headers.end() && found->second == "chunked")
			{
				str data;
				if(net::read_chunked_encoding(ifs, data))
				{
					std::cout << "DATA\n";
					std::cout << data << '\n';
				}
			}
		}
	}

	return 0;
}
