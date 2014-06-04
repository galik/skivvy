/*
 * ircbot-oastats.cpp
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

#include <skivvy/plugin-oastats.h>

#include <array>
#include <ctime>
#include <thread>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include <sookee/types.h>
#include <skivvy/logrep.h>
#include <skivvy/irc.h>
#include <skivvy/ios.h>
#include <skivvy/cal.h>
#include <skivvy/stl.h>
#include <skivvy/str.h>
#include <skivvy/network.h>
#include <skivvy/openarena.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(OAStatsIrcBotPlugin);
PLUGIN_INFO("oastats", "OA Stats Reporter", "0.1");

using namespace skivvy;
using namespace skivvy::oa;
using namespace skivvy::irc;
using namespace sookee::types;
using namespace skivvy::utils;
using namespace skivvy::string;

const str MAX_RESULTS = "oastats.max_results";
const siz MAX_RESULTS_DEFAULT = 10;

OAStatsIrcBotPlugin::OAStatsIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
, timer([&](const void* u){ regular_stats(*reinterpret_cast<const str*>(u)); })
{
}

OAStatsIrcBotPlugin::~OAStatsIrcBotPlugin()
{
}

static str get_url(const str& type = "n", cal::year_t year = cal::get_year(), cal::month_t month = cal::get_month())
{
	static const str base = "http://sookee.dyndns.org/oatab/";
	return base + std::to_string(year) + "-" + cal::get_month_str(month) + "-effect_" + type + ".html";
}

enum STAT
{
	S_RANK
	, S_NAME

	, S_KD
	, S_CK
	, S_FK
	, S_RK
	, S_PW
	, S_TO // Total Opponents
};

static str_siz_map type_map =
{
	{"kd", S_KD}, {"ck", S_CK}, {"fk", S_FK}, {"rk", S_RK}, {"pw", S_PW}
};

static str_map name_map =
{
	{"kd", "Kills/Death"}
	, {"ck", "Caps/Death"}
	, {"fk", "Flag Kills/Death"}
	, {"rk", "Flag Returns/Death"}
	, {"pw", "Won/Played"}
};

bool OAStatsIrcBotPlugin::rpc_lookup_players(const str& search, str_vec& handles)
{
	bool ret = false;
	IrcBotRPCService* s = bot.get_rpc_service("OA Player Finder");
	if(s)
	{
		IrcBotRPCService::call_ptr c = s->create_call();
		//local_call* cp = dynamic_cast<local_call*>(&(*c));
		c->set_func("lookup_players");
		c->add_param(search);
		c->add_param(handles);
		s->rpc(*c);
		ret = c->get_return_value<bool>();
		handles.clear();
		c->get_return_param(handles);
	}
	return ret;
}

bool OAStatsIrcBotPlugin::rpc_match_player(bool subst, const str& name1, const str& name2)
{
	IrcBotRPCService* s = bot.get_rpc_service("OA Player Finder");
	if(s)
	{
		IrcBotRPCService::call_ptr c = s->create_call();
		//rpc::local_call* cp = dynamic_cast<rpc::local_call*>(&(*c));
		c->set_func("match_player");
		c->add_param(subst);
		c->add_param(name1);
		c->add_param(name2);
		s->rpc(*c);
		return c->get_return_value<bool>();
	}
	return false;
}

str OAStatsIrcBotPlugin::rpc_html_handle_to_irc(str html)
{
	IrcBotRPCService* s = bot.get_rpc_service("OA Player Finder");
	if(s)
	{
		IrcBotRPCService::call_ptr c = s->create_call();
		c->set_func("html_handle_to_irc");
		c->add_param(html);
		s->rpc(*c);
		return c->get_return_value<str>();
	}
	return "";
}

bool extract_y_and_m(const str& params, cal::year_t& y, cal::month_t& m)
{
	bug("params: " << params);
	cal::year_t year = y;
	cal::month_t month = m;

	str param;
	std::istringstream iss(params);

	while(ios::getstring(iss, param))
	{
		// param can not be empty() from valid ios:getstring(
		assert(!param.empty());
		bug("param : " << param);
		if(param[0] == '@')
		{
			if(param.size() > 3) // year (@2011)
			{
				bug("@ found setting year to: " << param.substr(1));
				if(!(std::istringstream(param.substr(1)) >> year)
				|| year < 1000 || year > 9999)
				{
					return false;
				}
				bug("year : " << year);
			}
			else if(param.size() > 1) // month (@11)
			{
				bug("@ found setting month to: " << param.substr(1));
				if(!(std::istringstream(param.substr(1)) >> month)
				|| month < 1 || month > 12)
				{
					return false;
				}
				--month;
				bug("month : " << month);
			}
		}
	}

	if(month > cal::get_month() && year == cal::get_year())
		--year;

	y = year;
	m = month;

	return true;
}

void OAStatsIrcBotPlugin::oa1v1(const message& msg)
{
	BUG_COMMAND(msg);

	struct sub
	{
		str handle;
		str_vec matches;
		bool subst;

		bool operator==(const sub& s) { return handle == s.handle; }
		bool operator!=(const sub& s) { return !((*this) == s); }
	};

	str type;

	typedef std::vector<sub> sub_vec;
	typedef sub_vec::iterator sub_vec_iter;
	typedef sub_vec::const_iterator sub_vec_citer;

	sub_vec subs;
	cal::year_t year = cal::get_year();
	cal::month_t month = cal::get_month();
	std::istringstream iss(msg.get_user_params_cp());
	bug("msg.get_user_params(): " << msg.get_user_params_cp());

	extract_y_and_m(msg.get_user_params_cp(), year, month);

	sub s;
	str param;
	while(ios::getstring(iss, param))// && !param.empty())
	{
		if(param[0] != '@')
		{
			s.handle = param;
			subs.push_back(s);
		}
	}

	for(sub& s: subs)
		s.subst = rpc_lookup_players(s.handle, s.matches);

	// Sort all
	for(sub& s: subs)
		stl::sort(s.matches);

	struct candidate
	{
		bool subst;
		str match;

		candidate(bool subst, const str& match): subst(subst), match(match) {}
	};

	typedef std::pair<candidate, candidate> candidate_pair;

	std::vector<candidate_pair> candidates;

	// Get all permutations
	if(subs.size() > 1)
		for(const str& m1: subs[0].matches)
			for(sub& s: subs)
				if(subs[0].handle != s.handle)
					for(const str& m2: s.matches)
						candidates.push_back({ candidate(subs[0].subst, m1), candidate(s.subst, m2) });

//	for(const candidate_pair& cp: candidates)
//		bug("c: " << (cp.first.subst?"exact ":"substr ") << cp.first.match
//			<< ", " << (cp.second.subst?"exact ":"substr ") << cp.second.match);

	const str url = "http://sookee.dyndns.org/oatab/"
		+ std::to_string(year) + "-"
		+ cal::get_month_str(month) + "-scores.html";

	bug("url: " << url);

	// basic HTTP GET
	net::socketstream ss;
	ss.open("sookee.dyndns.org", 80);
	ss << "GET " << url;
	ss << "\r\n" << std::flush;

	std::ostringstream tr_oss;

	str match1;
	str match2;
	size_t max_len1 = 0;
	siz_vec len1;
	siz max_len2 = 0;
	siz_vec len2;
	std::vector<str_vec> results;
	str tmp;

	while(net::read_tag(ss, tr_oss, "tr"))
	{
		std::istringstream tr_iss(tr_oss.str());
		std::ostringstream td_oss;
		str_vec td;
		for(size_t i = 0; i < 10 && net::read_tag(tr_iss, td_oss, "td"); ++i)
		{
			if(i == 0 || i == 2 || i == 3 || i == 9)
			{
				tmp = td_oss.str();
				td.push_back(trim(tmp));
			}
			td_oss.clear();
			td_oss.str("");
		}
		if(td.size() == 4)
		{
			match1 = net::html_to_text(td[0]);
			if(match1[match1.size() - 1] == '*')
				match1.resize(match1.size() - 1);

			match2 = net::html_to_text(td[1]);
			if(match2[match2.size() - 1] == '*')
				match2.resize(match2.size() - 1);

			trim(match1);
			trim(match2);

			for(const candidate_pair& cp: candidates)
			{
				if(rpc_match_player(cp.first.subst, match1, cp.first.match)
				&& rpc_match_player(cp.second.subst, match2, cp.second.match))
				{
					results.push_back(td);
					len1.push_back(match1.length());
					len2.push_back(match2.length());
					max_len1 = std::max(max_len1, match1.length());
					max_len2 = std::max(max_len2, match2.length());
				}
			}
		}

		tr_oss.clear();
		tr_oss.str("");
	}

	const siz max_matches = 30;
	siz matches = 0;
	str name1;
	str name2;
	siz val1, val2;
	siz pos1, pos2;

	bug("max_len1: " << max_len1);
	bug("max_len2: " << max_len2);

	size_t i = 0;
	for(const str_vec& td: results)
	{
		if(matches < max_matches)
		{
			name1 = rpc_html_handle_to_irc(td[0]);
			if((pos1 = name1.find("<a href='")) != str::npos)
				if((pos2 = name1.find("'>*</a>")) != str::npos)
					name1 = name1.substr(0, pos1) + name1.substr(pos2 + 7);

			name2 = rpc_html_handle_to_irc(td[1]);
			if((pos1 = name2.find("<a href='")) != str::npos)
				if((pos2 = name2.find("'>*</a>")) != str::npos)
					name2 = name2.substr(0, pos1) + name2.substr(pos2 + 7);

			std::istringstream(net::html_to_text(td[2])) >> val1;
			std::istringstream(net::html_to_text(td[3])) >> val2;

			double ratio = double(val1)/val2;
			int diff = val1 - val2;

			name1 = net::html_to_text(name1);
			name2 = net::html_to_text(name2);
			bug("name1.length(): " << name1.length());
			bug("name2.length(): " << name2.length());

			std::ostringstream oss;
			oss.fill(' ');
			oss.precision(2);
			oss << std::right;
			oss << "1v1: " << name1 << IRC_COLOR << IRC_Black << ","  << IRC_Black << str(max_len1 - len1[i], ' ') << IRC_NORMAL;
			oss << std::setw(4) << val1;
			oss << " vs " << name2 << IRC_COLOR << IRC_Black << ","  << IRC_Black << str(max_len2 - len2[i], ' ') << IRC_NORMAL;
			oss << std::setw(4) << val2;
			oss << ": [" << ratio;
			oss << "] (" << diff;
			oss << ")";
			bot.fc_reply(msg, oss.str());
			++matches;
			++i;
		}
	}
	if(!matches)
		bot.fc_reply(msg, "!oa1v1 did not find any matches.");
	else if(matches >= max_matches)
		bot.fc_reply(msg, "*** Flood Control ***");
}

void OAStatsIrcBotPlugin::oastat(const message& msg)
{
	BUG_COMMAND(msg);

	str type, handle;
	cal::year_t year = cal::get_year();
	cal::month_t month = cal::get_month();

	extract_y_and_m(msg.get_user_params_cp(), year, month);

	str param;
	std::istringstream iss(msg.get_user_params_cp());

	while(iss >> param && !param.empty())
	{
		static const std::vector<str> types = {"kd", "ck", "fk", "rk", "pw"};
		if(std::find(types.cbegin(), types.cend(), param) != types.cend())
		{
			type = param;
		}
		else if(param[0] != '@')
		{
			handle = param;
		}
	}

	bug("======:---------------");
	bug("month : " << month);
	bug("type  : " << type);
	bug("handle: " << handle);

	//std::vector<str> handles;
	str_vec handles;

	// NOTE: If handle is empty rpc_lookup_players() will
	// put the empty handle in handles. This will
	// later match everything, hence producing the
	// top scores for the given type.

	// hence max_matched = 10 (for top 10) if no handle was given
	size_t max_matches = handle.empty() ? 10 : bot.get(MAX_RESULTS, MAX_RESULTS_DEFAULT);

	bool subst = rpc_lookup_players(handle, handles);

	if(type.empty())
	{
		bot.fc_reply(msg, help(msg.get_user_cmd_cp()));
		return;
	}

	const str url = get_url(type, year, month);
	bug("url: " << url);

	// basic HTTP GET
	net::socketstream ss;
	ss.open("sookee.dyndns.org", 80);
	ss << "GET " << url;
	ss << "\r\n" << std::flush;

	struct match
	{
		size_t rank;
		str line;
		bool operator<(const match& m) const { return rank > m.rank; }
	};

	std::multiset<match> match_mset;

	bug("handles.size(): " << handles.size());

	size_t matches = 0;
	std::ostringstream tr_oss;
	while(net::read_tag(ss, tr_oss, "tr"))
	{
		std::vector<str> stats;
		std::istringstream iss(tr_oss.str());
		std::ostringstream oss;

		for(size_t i = 0; net::read_tag(iss, oss, "td"); ++i)
		{
			stats.push_back(oss.str());
			oss.clear();
			oss.str("");
		}
		for(const str& h: handles)
		{
			if(rpc_match_player(subst, net::html_to_text(stats[S_NAME]), h))
			{
				str rank = net::html_to_text(stats[S_RANK]);
				if(rank.size() == 1)
					rank += " ";
				match m;
				m.line = ""
					+ IRC_NORMAL + "#"
					+ IRC_COLOR + IRC_Red + rank
					+ " " + IRC_COLOR + IRC_Navy_Blue + net::html_to_text(stats[type_map[type]])
					+ " " + rpc_html_handle_to_irc(stats[S_NAME])
					+ IRC_NORMAL;
				std::istringstream(stats[S_RANK]) >> m.rank;
				match_mset.insert(m);
			}
		}

		tr_oss.clear();
		tr_oss.str("");
	}

	if(match_mset.empty() && !handle.empty())
	{
		str delim;
		std::ostringstream oss;
		for(const str& h: handles)
		{
			oss << delim << "'" << h << "'";
			delim = " or ";
		}
		bot.fc_reply(msg, "Player name containing " + oss.str() + " not found.");
	}
	else if(match_mset.size() > max_matches)
	{
		std::ostringstream oss;
		oss << "Found " << matches << " matches, listing only the top " << max_matches << ".";
		bot.fc_reply(msg, oss.str());
	}

	// Print Title
	bot.fc_reply(msg, IRC_BOLD + name_map[type] + IRC_NORMAL);

	size_t count = max_matches;
	for(const match& m: match_mset)
	{
		bot.fc_reply(msg, m.line);
		if(!(--count)) break;
	}
}

bool OAStatsIrcBotPlugin::rpc(rpc::call& c)
{
	bug_func();
	str func = c.get_func();
	bug("func: " << func);
	if(func == "get_oatop")
	{
		str params = c.get_param<str>();
		stats_vector v = c.get_param<stats_vector>();
		c.set_return_value(get_oatop(params, v));
		c.add_return_param(v);
		return true;
	}
//	else if(func == "match_player")
//	{
//		bool subst = c.get_param<bool>();
//		str name1 = c.get_param<str>();
//		str name2 = c.get_param<str>();
//		c.set_return_value(match_player(subst, name1, name2));
//		return true;
//	}
//	else if(func == "html_handle_to_irc")
//	{
//		str html = c.get_param<str>();
//		c.set_return_value(html_handle_to_irc(html));
//		return true;
//	}
	return false;
}

str html_handle_to_oa(str html)
{
	static std::map<str, str> subs =
	{
		{"<b>", ""}//IRC_BOLD}
		, {"</b>", ""}//IRC_NORMAL}
		, {"</font>", ""}//IRC_NORMAL}
		, {"</span>", ""}//IRC_NORMAL}
		, {"<span style='color: black'>", "^8"}
		, {"<font color=\"#000000\">", "^8"}
		, {"<span style='color: red'>", "^1"}
		, {"<font color=\"#FF0000\">", "^1"}
		, {"<span style='color: lime'>", "^2"}
		, {"<font color=\"#00FF00\">", "^2"}
		, {"<span style='color: yellow'>", "^3"}
		, {"<font color=\"#FFFF00\">", "^3"}
		, {"<span style='color: blue'>", "^4"}
		, {"<font color=\"#0000FF\">", "^4"}
		, {"<span style='color: cyan'>", "^5"}
		, {"<font color=\"#00FFFF\">", "^5"}
		, {"<span style='color: magenta'>", "^6"}
		, {"<font color=\"#FF00FF\">", "^6"}
		, {"<span style='color: white'>", "^7"}
		, {"<font color=\"#FFFFFF\">", "^7"}
		, {"<font color=\"#AAAAAA\">", ""}
		, {"<font color=\"#828282\">", ""}
		, {"<font color=\"\">", ""}
	};

	size_t pos = 0;
	for(std::pair<const str, str>& p: subs)
		while((pos = html.find(p.first)) != str::npos)
			html.replace(pos, p.first.size(), p.second);
	return net::fix_entities(html);
}

str dtos(double d, siz dp = 2)
{
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(dp) << d;
	return oss.str();
}

bool OAStatsIrcBotPlugin::get_oatop(const str& params, stats_vector& v)
{
	bug_func();

	v.clear();

	cal::year_t year = cal::get_year();
	cal::month_t month = cal::get_month();

	extract_y_and_m(params, year, month);

	bug("year  : " << year);
	bug("month : " << month);

	str url = get_url("kd", year, month);

	// basic HTTP GET
	net::socketstream ss;
	ss.open("sookee.dyndns.org", 80);
	ss << "GET " << url;
	ss << "\r\n" << std::flush;

	std::ostringstream tr_oss;
	while(net::read_tag(ss, tr_oss, "tr"))
	{
		std::vector<str> stats;
		std::istringstream iss(tr_oss.str());
		std::ostringstream oss;
		size_t i = 0;
		for(; net::read_tag(iss, oss, "td"); ++i)
		{
			stats.push_back(oss.str());
			oss.clear();
			oss.str("");
		}

		if(i > 0)
		{
			keystats s;
			s.name = html_handle_to_oa(stats[S_NAME]);

			std::istringstream(net::html_to_text(stats[S_KD])) >> s.kd;
			std::istringstream(net::html_to_text(stats[S_CK])) >> s.cd;
			std::istringstream(net::html_to_text(stats[S_PW])) >> s.pw;
			std::istringstream(net::html_to_text(stats[S_TO])) >> s.to;
			s.overall = 0.0;
			v.push_back(s);
		}

		tr_oss.clear();
		tr_oss.str("");
	}

	bug_var(v.size());
	if(year >= 2012 && month >= 5)
	{
		v.erase(std::remove_if(v.begin(), v.end(), [&](const keystats& ks)
		{
			return ks.to < 20;
		}), v.end());
	}
	bug_var(v.size());

	for(keystats& ks: v)
		std::istringstream(dtos((ks.kd * ks.cd * ks.pw)/100, 2)) >> ks.overall;

	std::sort(v.begin(), v.end(), [&](const keystats& ks1, const keystats& ks2)
	{
		return ks1.overall > ks2.overall;
	});

	size_t rank = 0;
	double prev = 0.0;
	siz d = 1;

	for(keystats& s: v)
	{
		if(s.overall != prev)
			{ rank += d; d = 1; }
		else
			++d;
		prev = s.overall;
		s.rank = rank;
	}

	siz i = 0;
	for(const keystats& ks: v)
		bug("ks: " << (++i) << " [" << ks.to << "] "<< ks.name);
	for(i = 0; i < v.size(); ++i)
	{
		bug_var(i);
		bug_var(v[i].rank);
		bug_var(v[i].name);
		bug("");
	}

	return true;
}


void OAStatsIrcBotPlugin::oatop(const message& msg)
{
	BUG_COMMAND(msg);

	stats_vector v;
	get_oatop(msg.get_user_params_cp(), v);

	if(v.empty())
	{
		bot.fc_reply(msg, "There are currently no stats for this month.");
		return;
	}

	size_t min = 1;
	size_t factor = 1;

	str param;
	std::istringstream iss(msg.get_user_params_cp());

	while(iss >> param && !param.empty())
	{
		if(param[0] == '#' && param.size() > 1)
		{
			factor = 10;
			if(!(std::istringstream(param.substr(1)) >> min))
			{
				bot.fc_reply(msg, help(msg.get_user_cmd_cp()));
				return;
			}
		}
	}

	min = ((min - 1) * factor) + 1;
	size_t max = min + 10 - 1;

	const str C_KD =  IRC_COLOR + IRC_Royal_Blue;
	const str C_CD =  IRC_COLOR + IRC_Red;
	const str C_WP =  IRC_COLOR + IRC_Lime_Green;
	const str C_OA =  IRC_COLOR + IRC_Hot_Pink;
	const str OFF =  IRC_NORMAL;

	str title = IRC_BOLD + IRC_UNDERLINE + "Rank "
	+ C_KD + "K/D  "
	+ C_CD + "C/D  "
	+ C_WP + "W/P    "
	+ C_OA + "Overall" + OFF;

	bot.fc_reply(msg, title);

	siz matches = 0;

	for(const keystats& s: v)
	{
		++matches;
		if(matches < min) continue;
		if(matches > max) break;

		std::ostringstream oss;

		oss.precision(2);
		oss << std::fixed;
		oss << "#" << std::left << std::setw(4) << s.rank;
		oss << C_KD << std::left << std::setw(5) << s.kd;
		oss << C_CD << std::left << std::setw(5) << s.cd;
		oss << C_WP << std::right << std::setw(6) << s.pw;
		oss << C_OA << " " << std::right << std::setw(5) << s.overall;
		oss << std::left<< "   " << oa_handle_to_irc(s.name);

		bot.fc_reply(msg, oss.str());
	}
}

const str STATS_DELAY = "oastats.delay";
const delay STATS_DELAY_DEFAULT = 60 * 60 * 6; // 6 hours
const str STATS_DIVIDIONS = "oastats.divisions";
const siz STATS_DIVIDIONS_DEFAULT = 4; // Top 4 divisions

void OAStatsIrcBotPlugin::oastats_on(const message& msg)
{
	bug_func();
	siz delay = bot.get(STATS_DELAY, STATS_DELAY_DEFAULT);
	timer.set_mindelay(delay);
	timer.set_maxdelay(delay);
	lock_guard lock(channels_mtx);
	channels.insert(msg.reply_to_cp());
	if(timer.on(&(*channels.find(msg.reply_to_cp()))))
	{
		std::ostringstream oss;
		oss.precision(2);
		oss << std::fixed;
		if(delay < 60 * 60 * 60)
			oss << (double(delay) / (60 * 60)) << " hours.";
		if(delay < 60 * 60)
			oss << (double(delay) / 60) << " minutes.";
		if(delay < 60)
			oss << (delay) << " seconds.";
		bot.fc_reply(msg, "Regular stats have been turned on for this channel every " + oss.str());
	}
	else
		bot.fc_reply(msg, "Regular stats are already turned on for this channel.");
}

void OAStatsIrcBotPlugin::oastats_off(const message& msg)
{
	bug_func();
	lock_guard lock(channels_mtx);
	channels.insert(msg.reply_to_cp());
	if(timer.off(&(*channels.find(msg.reply_to_cp()))))
		bot.fc_reply(msg, "Regular stats have been disabled for this channel.");
	else
		bot.fc_reply(msg, "Regular stats were not enabled for this channel.");
}

void OAStatsIrcBotPlugin::regular_stats(const str& channel)
{
	// moderate channel
//	irc->mode(channel, "+m");
	// TODO finish this
	message msg;
	msg.from_cp = bot.nick;
	msg.to_cp = channel;

	std::this_thread::get_id();

	siz divisions = bot.get(STATS_DIVIDIONS, STATS_DIVIDIONS_DEFAULT);

	bot.fc_reply(msg, IRC_BOLD + IRC_COLOR + IRC_Navy_Blue
		+ "Stats listing about to commence in 10 seconds, "
		+ IRC_COLOR + IRC_Red + "please stop typing.");
	std::this_thread::sleep_for(std::chrono::seconds(10));
	for(siz i = 1; i < (divisions + 1); ++i)
	{
		msg.text_cp = "!oatop #" + std::to_string(i);
		oatop(msg);
	}

	bot.fc_reply(msg, IRC_BOLD + IRC_COLOR + IRC_Navy_Blue + "Stats listing over.");
}

bool OAStatsIrcBotPlugin::initialize()
{
	if(!bot.has_plugin("pfinder", "0.1"))
	{
		log("DEPENDENCY ERROR: Require plugin 'OA Player Finder'.");
		return false;
	}
	add
	({
		"!oastat"
		, "!oastat (kd|ck|fk|rk|pw) [<player>] [@month (1-12)] [@year (YYYY)] Report OA stats on optional <player>."
		, [&](const message& msg){ oastat(msg); }
	});
	add
	({
		"!oa1v1"
		, "!oa1v1 <player1> <player2> [@month (1-12)] [@year (YYYY)] Report player1 vs player2 kills."
		, [&](const message& msg){ oa1v1(msg); }
	});
	add
	({
		"!oatop"
		, "!oatop [#division (1+)] [@month (1-12)] [@year (YYYY)] List the top OA players [in divisions of 10]."
		, [&](const message& msg){ oatop(msg); }
	});
	add
	({
		"!oastats_on"
		, "!oastats_on Start producing regular stats listings in this channel."
		, [&](const message& msg){ oastats_on(msg); }
	});
	add
	({
		"!oastats_off"
		, "!oastats_off Stop producing regular stats listings in this channel."
		, [&](const message& msg){ oastats_off(msg); }
	});
	bot.add_rpc_service(*this);
	return true;
}

// INTERFACE: IrcBotPlugin

str OAStatsIrcBotPlugin::get_id() const { return ID; }
str OAStatsIrcBotPlugin::get_name() const { return NAME; }
str OAStatsIrcBotPlugin::get_version() const { return VERSION; }

void OAStatsIrcBotPlugin::exit()
{
	timer.turn_off();
}

}} // skivvy::ircbot
