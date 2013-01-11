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

#include <skivvy/ircbot.h>
#include <skivvy/logrep.h>

using namespace skivvy::ircbot;

#include <cstdio>
#include <execinfo.h>
#include <signal.h>
#include <cstdlib>
#include <cxxabi.h>

#include <memory>

void handler(int sig)
{
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	//backtrace_symbols_fd(array, size, 2);
	char** trace = backtrace_symbols(array, size);

	int status;
	str obj, func;
	for(siz i = 0; i < size; ++i)
	{
		// /home/gareth/dev/cpp/skivvy/build/src/.libs/libskivvy.so.0(_ZN6skivvy6ircbot11RandomTimer5timerEv+0x85)[0x4e2ead]
		sgl(sgl(siss(trace[i]), obj, '('), func, '+');

		cstring_uptr func_name(abi::__cxa_demangle(func.c_str(), 0, 0, &status));
		std::cerr << "function: " << func_name.get() << '\n';
		std::cerr << "info    : " << trace[i] << '\n';
		std::cerr << '\n';
	}
	free(trace);

	exit(1);
}


int main(int argc, char* argv[])
{
	signal(SIGSEGV, handler);   // install our handler

//	try
//	{
		IrcBot bot;
		log(bot.get_name() + " v" + bot.get_version());
		bot.init(argc > 1 ? argv[1] : "");
		if(bot.restart)
			return 6;
//	}
//	catch(std::exception& e)
//	{
//		std::cerr << e.what() << '\n';
//		handler(0);
//	}
	return 0;
}
