/*
 * rcon.h
 *
 *  Created on: 01 June 2012
 *      Author: oasookee@googlemail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2012 SooKee oasookee@googlemail.com               |
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

#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <thread>
#include <chrono>

#include <sstream>

#include <skivvy/logrep.h>
#include <skivvy/rcon.h>
#include <skivvy/types.h>

namespace skivvy { namespace net {

using namespace skivvy;
using namespace skivvy::utils;
using namespace skivvy::types;

bool plog(const str& msg, bool error)
{
	log(msg);
	return error;
}

bool rcon(const str& cmd, str& res, const str& host, int port)
{
	//bug_func();
	//bug(" cmd: " << cmd);
	//bug("host: " << host);
	//bug("port: " << port);

	int cs = socket(PF_INET, SOCK_DGRAM, 0);

	if(cs <= 0)
		return plog(strerror(errno), false);

	sockaddr_in sin;
	hostent *he = gethostbyname(host.c_str());
	std::copy(he->h_addr, he->h_addr + he->h_length
		, reinterpret_cast<char*>(&sin.sin_addr.s_addr));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	if(connect(cs, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)) < 0)
		return plog(strerror(errno), false);

	fcntl(cs, F_SETFL, O_NONBLOCK);

	const str msg = "\xFF\xFF\xFF\xFF" + cmd;
	if(send(cs, msg.c_str(), msg.size(), 0) < 1)
		return plog(strerror(errno), false);

	res.clear();

	const std::chrono::milliseconds ds(100);
//	const std::chrono::milliseconds cs(10);
//	const std::chrono::milliseconds ms(1);

	int len;
	char buf[1024];
	for(siz i = 0; i < 10; ++i, std::this_thread::sleep_for(ds))
	{
		for(; (len = read(cs, buf, 1024)) > 9; i = 0)
			res.append(buf + 10, len - 10);
		if(len > 0)
			res.append(buf, len);
		else if(len == 0)
			break;
	}

	close(cs);

	return true;
}

}} // skivvy::net

