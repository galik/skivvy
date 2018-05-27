/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oaskivvy@gmail.com               |
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

#include <skivvy/server.h>

#include <skivvy/logrep.h>
#include <hol/small_types.h>
#include <skivvy/socketstream.h>

namespace skivvy { namespace net {

using namespace skivvy::utils;
using namespace sookee::types;

class test_irc_server
: public server
{
private:
	bool done = false;

	void serve(int cs)
	{
		log("Client session starting: " << cs);
		str line;
		socketstream ss(cs);

		while(!done && sgl(ss, line))
		{
			//bug_var(line);
		}
		log("Client session terminated: " << cs);
	}

public:
	void start()
	{
		set_handler([](int cs){ serve(cs); });
		listen(6667);
	}

};

}} // skivvy::net
