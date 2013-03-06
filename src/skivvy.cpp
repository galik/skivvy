/*
 * skivvy.cpp
 *
 *  Created on: 29 Jul 2011
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

#include <sookee/bug.h>
#include <skivvy/ircbot.h>

using namespace skivvy::ircbot;
using namespace skivvy::utils;
using namespace sookee::bug;
using namespace sookee::log;

#include <cstdio>
#include <execinfo.h>
#include <signal.h>
#include <cstdlib>
#include <cxxabi.h>

#include <memory>

int main(int argc, char* argv[])
{
	bug_func();

	ADD_STACK_HANDLER();

	IrcBot bot;

	log(bot.get_name() + " v" + bot.get_version());

	bot.init(argc > 1 ? argv[1] : "");
	bot.exit();

	if(bot.restart)
		return 6;
	return 0;
}
