/*
 * ircbot-grabber.cpp
 *
 *  Created on: 30 Jan 2012
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

#include <skivvy/plugin-compile.h>

#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>

#include <skivvy/logrep.h>
#include <skivvy/str.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(CompileIrcBotPlugin);
PLUGIN_INFO("compile", "Compile Code", "0.1");

using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

const str HEADERS =
	"#include <map>\n"
	"#include <set>\n"
	"#include <list>\n"
// 	"#include <array>\n" c++11
	"#include <cmath>\n"
	"#include <ctime>\n"
	"#include <deque>\n"
	"#include <queue>\n"
	"#include <stack>\n"
	"#include <bitset>\n"
	"#include <cstdio>\n"
	"#include <vector>\n"
	"#include <string>\n"
//	"#include <future>\n" c++11
//	"#include <thread>\n" c++11
	"#include <cstdlib>\n"
	"#include <cstring>\n"
	"#include <fstream>\n"
	"#include <sstream>\n"
	"#include <iostream>\n"
	"#include <iterator>\n"
	"#include <algorithm>\n"
	"#include <exception>\n"
	"#include <stdexcept>\n"
	"#include <functional>\n"
	"\n";

const str PREAMBLE =
	"using namespace std;"
	"\n";

const str MAIN = "int main()\n";

CompileIrcBotPlugin::CompileIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
{
}

CompileIrcBotPlugin::~CompileIrcBotPlugin() {}

void CompileIrcBotPlugin::cpp(const message& msg, bool cpp11)
{
	BUG_COMMAND(msg);
	str text = msg.get_user_params();
	if(!text.empty() && text[0] == '{')
	{
		lock_guard lock(mtx);
		system("rm -f compile");
		system("rm -f compile.cpp");
		system("rm -f compile.logfile.txt");
		system("rm -f compile.errors.txt");
		system("rm -f compile.output.txt");

		str code = HEADERS + PREAMBLE + MAIN + text;
		std::ofstream ofs("compile.cpp");
		ofs.write(code.c_str(), code.size());
		ofs.close();

		const str CXX = cpp11 ? "/extra/gcc-trunk-4.7/bin/g++ -std=gnu++11" : "g++";
		const str LD_LIBRARY_PATH =  cpp11 ? "/extra/gcc-trunk-4.7/lib" : "";
		const str COMPILE_ARGS = " -o compile compile.cpp > compile.logfile.txt 2> compile.errors.txt";
		const str RUN_ARGS = " timeout 15s ./compile > compile.output.txt 2>&1";

		if(-1 == std::system((CXX + COMPILE_ARGS).c_str()))
		{
			bot.fc_reply(msg, "g++ failed to run.");
			return;
		}

		str line;
		std::ifstream ifs;

		// check for program file
		if(!std::ifstream("compile"))
		{
			bot.fc_reply(msg, "Compile failed.");
			// report errors
			ifs.open("compile.errors.txt");
			// TODO: Parse these to extract main (first) error
			str sep;
			std::ostringstream oss;
			for(siz n = 0;std::getline(ifs, line) && n < 10;)
			{
				bug("line: " << line);
				siz pos;
				if(!line.empty() && (pos = line.find("error: ")) != str::npos)
				{
					oss << sep << line.substr(pos + 7);
					sep = ", ";
					++n;
				}
			}
			bot.fc_reply(msg, oss.str());
			return;
		}

		// Run program
		if(-1 == std::system(("LD_LIBRARY_PATH=" + LD_LIBRARY_PATH + RUN_ARGS).c_str()))
		{
			bot.fc_reply(msg, "Program failed to run.");
			return;
		}

		ifs.open("compile.output.txt");
		if(!ifs)
		{
			bot.fc_reply(msg, "Output not found.");
			return;
		}

		siz n = 0;
		// TODO: Parse these to extract useful text
		while(std::getline(ifs, line) && n < 5)
			if(!line.empty())
			{
				if(line.size() > 100)
					line.erase(97) += "...";
				bot.fc_reply(msg, line);
				++n;
			}
		if(n >= 5)
			bot.fc_reply(msg, "... <--- flood control prevents further output.");
	}
}

// INTERFACE: BasicIrcBotPlugin

bool CompileIrcBotPlugin::initialize()
{
	add
	({
		"!c++"
		, "!c++ { cout << \"Hello IRC\\n\"; } - Compile & run c++ code."
		, [&](const message& msg){ cpp(msg, false); }
	});
	add
	({
		"!c++11"
		, "!c++11 { cout << \"Hello IRC\\n\"; } - Compile & run c++11 code."
		, [&](const message& msg){ cpp(msg, true); }
	});
//	bot.add_monitor(*this);
	return true;
}

// INTERFACE: IrcBotPlugin

str CompileIrcBotPlugin::get_id() const { return ID; }
str CompileIrcBotPlugin::get_name() const { return NAME; }
str CompileIrcBotPlugin::get_version() const { return VERSION; }

void CompileIrcBotPlugin::exit()
{
//	bug_func();
}

// INTERFACE: IrcBotMonitor

void FactoidIrcBotPlugin::event(const message& msg)
{
	if(msg.cmd == "PRIVMSG")
	{
	}
}

}} // sookee::ircbot
