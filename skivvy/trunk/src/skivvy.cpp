/*
 * ircbot.cpp
 *
 *  Created on: 29 Jul 2011
 *      Author: oasookee@googlemail.com
 */

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

#include <skivvy/stl.h>
#include <skivvy/types.h>
#include <skivvy/ircbot.h>
#include <skivvy/logrep.h>

#include <fstream>
#include <cstring>
#include <cstdlib>
#include <dirent.h>

using namespace skivvy;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::ircbot;

int main(int argc, char* argv[])
{
	IrcBot bot;
	log(bot.get_name() + " v" + bot.get_version());
	bot.init(argc > 1 ? argv[1] : "");
	if(bot.restart)
		std::exit(6);
	std::exit(0);
}
