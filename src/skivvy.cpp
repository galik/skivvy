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

#include <hol/bug.h>
#include <hol/small_types.h>
#include <hol/simple_logger.h>
#include <hol/experimental/term_utils.h>

#include <skivvy/ircbot.h>

using namespace skivvy::ircbot;
using namespace skivvy::utils;
using namespace hol::simple_logger;
using namespace hol::small_types::ios;
using namespace hol::small_types::basic;

#include <cstdio>
#include <execinfo.h>
#include <signal.h>
#include <cstdlib>
#include <cxxabi.h>

#include <memory>

inline
void set_default_log_colors()
{
	using namespace hol::simple_logger;
	using namespace hol::term_utils::ansi;
	using namespace hol::term_utils::ansi::color;

	auto border = ansi_esc({FAINT_ON}) + "│ " + ansi_esc({FAINT_OFF});
	auto BOLD = ansi_esc({BOLD_ON});

	log_out::format_time(FG::magenta + "%F"   + norm + " " + FG::cyan + "%T " + norm);
	log_out::level_name(LOG::D, FG::white     + "D" + norm + border);
	log_out::level_name(LOG::I, FG::blue      + "I" + norm + border);
	log_out::level_name(LOG::A, FG::cyan      + "A" + norm + border);
	log_out::level_name(LOG::W, FG::green     + "W" + norm + border);
	log_out::level_name(LOG::E, FG::red       + "E" + norm + border);
	log_out::level_name(LOG::X, fgcol256(226) + "X" + norm + border);
	log_out::level_parens(LOG::A,  BOLD, norm);
	log_out::level_prefix(LOG::X,  "<({[ " + bgcol256(235));
	log_out::level_suffix(LOG::X,  norm + " ]})>");
}

inline
void set_html_logging()
{
	using namespace hol::simple_logger;
	using namespace hol::term_utils::ansi;
	using namespace hol::term_utils::ansi::color;

//	auto border = ansi_esc({FAINT_ON}) + "│ " + ansi_esc({FAINT_OFF});
//	auto BOLD = ansi_esc({BOLD_ON});

	log_out::format_time("<td class='log-row>'><span class='log-date'>%F</span> <span class='log-time'>%T</span>");
	log_out::level_name(LOG::D, R"(<span class="log-D">D</span>)");
	log_out::level_name(LOG::I, R"(<span class="log-I">I</span>)");
	log_out::level_name(LOG::A, R"(<span class="log-A">A</span>)");
	log_out::level_name(LOG::W, R"(<span class="log-W">W</span>)");
	log_out::level_name(LOG::E, R"(<span class="log-E">E</span>)");
	log_out::level_name(LOG::X, R"(<span class="log-X">X</span>)");
	log_out::filter([](std::string s)
	{
		return R"(<span class="log-entry">)" + s + R"(</span></td>)";;
	});


}

auto txt = []
{
	using namespace hol::term_utils;
	bool active = false;
	if((active = hol::term_utils::term_supports_color()))
		set_default_log_colors();

//	set_html_logging();

	return ansi::ansi_fsm {active};
}();

//class ansi_filter
//: public hol::simple_logger::log_filter
//{
//public:
//	std::string operator()(std::string s) const
//	{
//		return txt(s);
//	}
//
//private:
//	ansi::ansi_fsm txt;
//};

int main(int argc, char* argv[])
{
	bug_fun();
	try
	{
		str_vec args(argv + 1, argv + argc);

		IrcBot bot;

//		log_out::synchronize_output();
//		log_out::filter([](std::string s){return txt(s);});

		LOG::A << bot.get_name() << " /v" << bot.get_version() + "/";

		bot.init(args.empty() ? "" :  args[0]);
		bot.exit();

		if(bot.restart)
			return 6;
		return 0;
	}
	catch(std::exception const& e)
	{
		LOG::X << e.what();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
