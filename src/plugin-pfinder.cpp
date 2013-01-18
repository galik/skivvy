/*
 * ircbot-pfinder.cpp
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

#include <skivvy/plugin-pfinder.h>
#include <skivvy/plugin-pfinder-oacom.h>

#include <iomanip>
#include <array>
#include <cmath>
#include <future>
#include <thread>
#include <fstream>
#include <sstream>
#include <functional>
#include <mutex>
#include <ctime>

#include <skivvy/ios.h>
#include <skivvy/irc.h>
#include <skivvy/stl.h>
#include <skivvy/str.h>
#include <skivvy/ircbot.h>
#include <skivvy/logrep.h>
#include <skivvy/network.h>
//#include <skivvy/rcon.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(PFinderIrcBotPlugin);
PLUGIN_INFO("OA Player Finder", "0.1");

using namespace skivvy;
using namespace skivvy::irc;
using namespace skivvy::oacom;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

const str MAX_MATCHES = "pfinder.max_matches";
const siz MAX_MATCHES_DEFAULT = 8;
const str CVAR_FILE = "pfinder.cvar_file";
const str CVAR_FILE_DEFAULT = "pfinder-cvars.txt";
const str CVAR_MAX_RESULTS = "pfinder.cvar_max_results";
const siz CVAR_MAX_RESULTS_DEFAULT = 8;
const str LINKS_FILE = "pfinder.links_file";
const str LINKS_FILE_DEFAULT = "pfinder-links.txt";
const str TELL_FILE = "pfinder.tell_file";
const str TELL_FILE_DEFAULT = "pfinder-tells.txt";
const str SERVER_UID_FILE = "pfinder.server_uid_file";
const str SERVER_UID_FILE_DEFAULT = "pfinder-servers.txt";

const str EDIV = R"(</div>)";

PFinderIrcBotPlugin::PFinderIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
//, automsg_timer([&](const void*){ check_automsgs(); })
{
}
PFinderIrcBotPlugin::~PFinderIrcBotPlugin() {}

str IRC_BOLD = "\x02";
str IRC_COLOR = "\x03";
str IRC_NORMAL = "\x0F";
str IRC_UNDERLINE = "\x1F";

static std::mutex links_file_mtx;

struct cvar_t
{
	str name;
	str desc;
	str dflt;
	str type;

	bool operator<(const cvar_t& c) const { return name < c.name; }

	friend std::istream& operator>>(std::istream& is, cvar_t& cvar)
	{
		std::getline(is, cvar.name);
		std::getline(is, cvar.desc);
		std::getline(is, cvar.dflt);
		std::getline(is, cvar.type);
		return is;
	}

	friend std::ostream& operator<<(std::ostream& os, const cvar_t& cvar)
	{
		os << cvar.name << '\n';
		os << cvar.desc << '\n';
		os << cvar.dflt << '\n';
		os << cvar.type;
		return os;
	}
};

typedef std::set<cvar_t> cvar_set;
typedef cvar_set::const_iterator cvar_set_citer;

static const str::size_type npos = str::npos;

static str BACK = "," + IRC_Black;
//static str BACK = "," + IRC_Navy_Blue;

// &lt;--NIK--&gt;
static std::map<str, str> subs =
{
	{"<b>", ""}//IRC_BOLD}
	, {"</b>", ""}//IRC_NORMAL}
	, {"</font>", ""}//IRC_NORMAL}
	, {"</span>", ""}//IRC_NORMAL}
	, {"<span style='color: black'>", IRC_COLOR + IRC_Black + BACK}
	, {"<font color=\"#000000\">", IRC_COLOR + IRC_Black + BACK}
	, {"<span style='color: red'>", IRC_COLOR + IRC_Red + BACK}
	, {"<font color=\"#FF0000\">", IRC_COLOR + IRC_Red + BACK}
	, {"<span style='color: lime'>", IRC_COLOR + IRC_Lime_Green + BACK}
	, {"<font color=\"#00FF00\">", IRC_COLOR + IRC_Lime_Green + BACK}
	, {"<span style='color: yellow'>", IRC_COLOR + IRC_Yellow + BACK}
	, {"<font color=\"#FFFF00\">", IRC_COLOR + IRC_Yellow + BACK}
	, {"<span style='color: blue'>", IRC_COLOR + IRC_Navy_Blue + BACK}
	, {"<font color=\"#0000FF\">", IRC_COLOR + IRC_Navy_Blue + BACK}
	, {"<span style='color: cyan'>", IRC_COLOR + IRC_Royal_Blue + BACK}
	, {"<font color=\"#00FFFF\">", IRC_COLOR + IRC_Royal_Blue + BACK}
	, {"<span style='color: magenta'>", IRC_COLOR + IRC_Purple + BACK}
	, {"<font color=\"#FF00FF\">", IRC_COLOR + IRC_Purple + BACK}
	, {"<span style='color: white'>", IRC_COLOR + IRC_White + BACK}
	, {"<font color=\"#FFFFFF\">", IRC_COLOR + IRC_White + BACK}
	, {"<font color=\"#AAAAAA\">", IRC_COLOR + IRC_Light_Gray + BACK}
	, {"<font color=\"#828282\">", IRC_COLOR + IRC_Dark_Gray + BACK}
	, {"<font color=\"\">", IRC_COLOR + IRC_Black + BACK}
};

str html_handle_to_irc(str html)
{
	size_t pos = 0;
	for(std::pair<const str, str>& p: subs)
		while((pos = html.find(p.first)) != str::npos)
			html.replace(pos, p.first.size(), p.second);
	return IRC_BOLD + net::fix_entities(html) + IRC_NORMAL;
}

#define ColorIndex(c)	( ( (c) - '0' ) & 7 )
#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && isalnum(*((p)+1)) ) // ^[0-9a-zA-Z]
#define Q_IsSpecialChar(c) ((c) && ((c) < 32))

str oatoirc(const str& str)
{
	std::ostringstream oss;

	static int  q3ToAnsi[ 8 ] =
	{
		30, // COLOR_BLACK
		31, // COLOR_RED
		32, // COLOR_GREEN
		33, // COLOR_YELLOW
		34, // COLOR_BLUE
		36, // COLOR_CYAN
		35, // COLOR_MAGENTA
		0   // COLOR_WHITE
	};

	const char* msg = str.c_str();

	while( *msg )
	{
		if( Q_IsColorString( msg ) || *msg == '\n' )
		{
			if( *msg == '\n' )
			{
				oss << "\033[0m\n";
				msg++;
			}
			else
			{
				oss << "\033[" << q3ToAnsi[ ColorIndex( *( msg + 1 ) ) ] << "m";
				msg += 2;
			}
		}
		else if(Q_IsSpecialChar(*msg))
		{
				// Print the special char
			oss << "#";
			msg++;
		}
		else
		{
			oss << *msg;
			msg++;
		}
	}
	return oss.str();
}

// TODO: make handles a std::set<str>
// Requires RPC to be working for std::set
// TODO: Make player lookup channel specific
bool PFinderIrcBotPlugin::lookup_players(const str& search, std::vector<str>& handles)
{
	bug_func();
	bug("search: " << search);

	// Use set to make values unique.
	// TODO: change parameter to set<>
	std::set<str> players;

	// First check our database of aliases to see if this is
	// a standard lookup (eg. IRC nick)
	std::ifstream ifs(bot.getf(LINKS_FILE, LINKS_FILE_DEFAULT));

	str key, val;
	//while(ifs >> key >> val >> std::ws)
	while(std::getline(ifs >> key, val))
	{
		trim(val);
		if(!val.empty() && (lowercase(search) == "all" || lowercase(key) == lowercase(search)))
			players.insert(val);
	}
	handles.assign(players.begin(), players.end());

	// If our parameter was not an alias, assume it is a player
	bool replaced = !handles.empty();
	if(!replaced)
		handles.push_back(search);

	return replaced;
}

bool PFinderIrcBotPlugin::match_player(bool subst, const str& name1, const str& name2)
{
	if(subst) return lowercase(name1) == lowercase(name2);
	return lowercase(name1).find(lowercase(name2)) != str::npos;
}

std::mutex uidfile_mtx;

bool PFinderIrcBotPlugin::server_uidfile_is_old() const
{
	lock_guard lock(uidfile_mtx);
	std::ifstream ifs(bot.getf(SERVER_UID_FILE, SERVER_UID_FILE_DEFAULT));
	std::time_t last;
	if(!ifs || ((ifs >> last) && std::time(0) - last > 24 * 60 * 60)) // daily
		return true;
	return false;
}

void PFinderIrcBotPlugin::write_server_uidfile(std::map<str, scache>& servers, siz uid) const
{
	lock_guard lock(uidfile_mtx);
	std::ofstream ofs(bot.getf(SERVER_UID_FILE, SERVER_UID_FILE_DEFAULT));
	if(!ofs)
	{
		log("Unable to open server uid file: " << bot.getf(SERVER_UID_FILE, SERVER_UID_FILE_DEFAULT));
		return;
	}

	// header
	ofs << time(0) << '\n';
	ofs << uid << '\n';

//	stl::sort(servers);
//	size_t i = 1;
	for(const std::pair<const str, scache>& p: servers)
		ofs << p.second << '\n';
}

str::size_type PFinderIrcBotPlugin::extract_server(const str& line, server& s, str::size_type pos) const
{
	server n;
	pos = extract_delimited_text(line, R"(<div id="address">)", EDIV, n.address, pos);
	pos = extract_delimited_text(line, R"(<div id="players">)", EDIV, n.players, pos);
	pos = extract_delimited_text(line, R"(<div id="map">)", EDIV, n.map, pos);
	pos = extract_delimited_text(line, R"(<div id="gametype">)", EDIV, n.gametype, pos);
	pos = extract_delimited_text(line, R"(<div id="name">)", EDIV, n.name, pos);
	if(pos != str::npos)
		s = n;
	return pos;
}

str::size_type PFinderIrcBotPlugin::extract_player(const str& line, player& p, str::size_type pos) const
{
	player n;
	pos = extract_delimited_text(line, R"(<div id="ping">)", EDIV, n.ping, pos);
	pos = extract_delimited_text(line, R"(<div id="frags">)", EDIV, n.frags, pos);
	pos = extract_delimited_text(line, R"(<div id="handle">)", EDIV, n.handle, pos);
	if(pos != str::npos)
		p = n;
	return pos;
}

std::vector<str> PFinderIrcBotPlugin::oafind(const str handle)
{
	bug_func();
	bug("handle: " << handle);

	std::vector<str> found;

	// The supplied parameter may resolve into a list of handles
	// or just one
	str_set handles;
	bool subst = false;
	{
		str_vec tmp_handles;
		subst = lookup_players(handle, tmp_handles);
		handles.insert(tmp_handles.begin(), tmp_handles.end());
	}
	str line;

	oa_server_vec oa_servers;
	if(!getservers(oa_servers))
	{
		log("No servers found.");
		return found;
	}

	bug_var(oa_servers.size());


	// Process response
	server s;
	player p;
	size_t matches = 0;
	std::ostringstream oss;

	//std::vector<server> servers; // rebuild server uid file
	const str uidfile = bot.getf(SERVER_UID_FILE, SERVER_UID_FILE_DEFAULT);
	siz uid = 0;
	std::map<str, scache> servers;
	{
		lock_guard lock(uidfile_mtx);
		std::ifstream ifs(uidfile);

		scache s;
		std::time_t time;
		// skip time
		ifs >> time >> uid >> std::ws;
		while(ifs >> s)
			servers[s.name] = s;
	}


	str status;
	for(siz i = 0; i < servers.size() && i < 10; ++i)
	{
		bug_var(oa_servers[i].host);
		if(getstatus(oa_servers[i].host, oa_servers[i].port, status))
		{
			str_vec lines;
			str info, line;
			siss iss(status);
			sgl(iss, info);
			while(sgl(iss, line))
				lines.push_back(line);

			server srv;
			str key, val;
			siss iss_s(info);
			srv.address = oa_servers[i].host + ":" + std::to_string(oa_servers[i].port);
			srv.players = std::to_string(lines.size());
			while(sgl(sgl(iss_s, key, '\\'), val, '\\'))
			{
				if(key == "sv_hostname")
					srv.name = val;
				else if(key == "mapname")
					srv.map = val;
				else if(key == "g_gametype")
					srv.gametype = val;
			}

			s = srv;
			if(servers.count(s.name))
				s.uid = servers[s.name].uid;
			else
				s.uid = ++uid;
			servers[s.name] = s;

			// players
			for(const str& line: lines)
			{
				if(sgl(siss(line) >> p.frags >> p.ping >> std::ws, p.handle))
				{
					bug("Found player: " << p.handle);
					for(str h: handles)
					{
						bug_var(h);
						bug_var(p.handle);
						if(match_player(subst, net::html_to_text(p.handle), h))
						{
							oss.clear();
							oss.str("");
							oss << IRC_BOLD << net::html_to_text(s.name) << IRC_NORMAL;
							oss << ": " << s.map;
							oss << " (" << IRC_BOLD << html_handle_to_irc(p.handle) << IRC_NORMAL
								<< " " << p.frags << ")";
							oss << "-> " << IRC_BOLD << s.address << IRC_NORMAL;
							oss << " [" << IRC_COLOR << IRC_Red << s.players << IRC_NORMAL << "]";
							found.push_back(oss.str());
							++matches;
						}
					}
				}
			}
		}
	}

	// refresh server UID file
	write_server_uidfile(servers, uid);


	return found;
}

std::vector<str> PFinderIrcBotPlugin::xoafind(const str handle)
{
	bug_func();
	bug("handle: " << handle);

	std::vector<str> found;

	// The supplied parameter may resolve into a list of handles
	// or just one
//	bool subst = false;
//	str_set handles;

	static std::map<str, str_set> handle_map;
	static std::map<str, bool> subst_map;

	if(links_changed)
		handle_map[handle].clear();

	if(subst_map.find(handle) == subst_map.end())
	{
		str_vec tmp_handles;
		subst_map[handle] = lookup_players(handle, tmp_handles);
		handle_map[handle].clear();
		handle_map[handle].insert(tmp_handles.begin(), tmp_handles.end());
		links_changed = false;
	}

	str_set& handles = handle_map[handle];
	bool& subst = subst_map[handle];

	str line;

	// basic HTTP GET
	net::socketstream ss;
	ss.open("dpmaster.deathmask.net", 80);
	ss << "GET " << "http://dpmaster.deathmask.net/?game=openarena&hide=empty";
	ss << "\r\n" << std::flush;

	// Process response
	server s;
	player p;
	size_t matches = 0;
	bool do_players = false;
	std::ostringstream oss;

	//std::vector<server> servers; // rebuild server uid file
	const str uidfile = bot.getf(SERVER_UID_FILE, SERVER_UID_FILE_DEFAULT);
	siz uid = 0;
	std::map<str, scache> servers;
	{
		lock_guard lock(uidfile_mtx);
		std::ifstream ifs(uidfile);

		scache s;
		std::time_t time;
		// skip time
		ifs >> time >> uid >> std::ws;
		while(ifs >> s)
			servers[s.name] = s;
	}

	str_set matched_set; // definite no-matches on player
	while(std::getline(ss, line))
	{
		if(do_players)
		{
			if(extract_player(line, p) == str::npos)
				do_players = false;

			if(do_players && matched_set.find(p.handle) == matched_set.end())
			{
				bool matched = false;
				for(str h: handles)
				{
					if(match_player(subst, net::html_to_text(p.handle), h))
					{
						matched = true;
						oss.clear();
						oss.str("");
						oss << IRC_BOLD << net::html_to_text(s.name) << IRC_NORMAL;
						oss << ": " << s.map;
						oss << " (" << IRC_BOLD << html_handle_to_irc(p.handle) << IRC_NORMAL
							<< " " << p.frags << ")";
						oss << "-> " << IRC_BOLD << s.address << IRC_NORMAL;
						oss << " [" << IRC_COLOR << IRC_Red << s.players << IRC_NORMAL << "]";
						found.push_back(oss.str());
						++matches;
					}
				}
				if(!matched) // ditch p.handle
					matched_set.insert(p.handle);
			}
		}
		else
		{
			// Record the last found server in s
			server srv;
			if(extract_server(line, srv) != str::npos)
			{
				s = srv;
				if(servers.count(s.name))
					s.uid = servers[s.name].uid;
				else
					s.uid = ++uid;
				servers[s.name] = s;
			}
//			else if(line.find(R"(<div id="ping">PING</div>)") != npos
//			&& line.find(R"(<div id="frags">FRAGS</div>)") != npos
//			&& line.find(R"(<div id="handle">NAME</div>)") != npos)
//				do_players = true;
			else if(line.find("PING") != npos
			&& line.find("FRAGS") != npos
			&& line.find("NAME") != npos)
				do_players = true;
		}
	}

	// refresh server UID file
	write_server_uidfile(servers, uid);


	return found;
}

// Biox1de's spreadheet
// https://docs.google.com/spreadsheet/ccc?key=0ApzKQFss715hdEFSQjYtemlXaE0yTlNxcGU5c3NsT3c#gid=0
void PFinderIrcBotPlugin::cvar(const message& msg)
{
	BUG_COMMAND(msg);

	str var = lowercase(msg.get_user_params());

	std::ifstream ifs(bot.getf(CVAR_FILE, CVAR_FILE_DEFAULT));

	cvar_set cvars;

	cvar_t cvar;
	while(ifs >> cvar)
		if(bot.wild_match("*" + var + "*", lowercase(cvar.name)))
			cvars.insert(cvar);

	siz max_results = bot.get(CVAR_MAX_RESULTS, CVAR_MAX_RESULTS_DEFAULT);

	if(cvars.empty())
		bot.fc_reply(msg, msg.get_user_params() + " did not match any cvar");
	else
	{
		siz i = 0;
		for(const cvar_t& cvar: cvars)
		{
			bot.fc_reply(msg, cvar.name + ": " + cvar.desc);
			if(++i == max_results)
				break;
		}

		if(cvars.size() > i)
		{
			str n = std::to_string(cvars.size() - i - 1);
			bot.fc_reply(msg, var + " also matches "  + n + " other cvars, you may want to be more specific.");
		}
	}
}

// list server
// !oaserver <subs> // list server matches + their UID
// !oaserver UID // list server's players

void PFinderIrcBotPlugin::oaserver(const message& msg)
{
	BUG_COMMAND(msg);
	bot.fc_reply(msg, "!oaserver function is officially off-line until some idiot fixed it.");
	return;

	str param = msg.get_user_params();

	const str EDIV = R"(</div>)";
	str uidfile = bot.getf(SERVER_UID_FILE, SERVER_UID_FILE_DEFAULT);

	scache s;
	siz uid = 0;
	str line;
	std::time_t time;

	if(server_uidfile_is_old())
	{
		bot.fc_reply(msg, "Building server list.");

		std::map<str, scache> servers;
		{
			lock_guard lock(uidfile_mtx);
			std::ifstream ifs(uidfile);

			// skip time
			ifs >> time >> uid >> std::ws;
			while(ifs >> s)
			{
				bug("reloading: " << s.name);
				servers[s.name] = s;
			}
		}

		// basic HTTP GET
		net::socketstream ss;
		ss.open("dpmaster.deathmask.net", 80);
		ss << "GET " << "http://dpmaster.deathmask.net/?game=openarena";
		ss << "\r\n" << std::flush;

//		std::ofstream xx("dpmaster.html");

		//std::vector<server> servers;
		while(std::getline(ss, line))
		{
//			xx << line << '\n';
			server srv;
			if(extract_server(line, srv) != str::npos)
			{
				s = srv;
				if(servers.count(s.name))
					s.uid = servers[s.name].uid;
				else
					s.uid = ++uid;
				servers[s.name] = s;
			}
		}
		write_server_uidfile(servers, uid);
	}

	std::ifstream ifs(uidfile);
	if(!ifs)
	{
		bot.fc_reply(msg, "Unable to open server file.");
		return;
	}

	str_vec found;

	siz match_uid = 0;
	std::istringstream(param) >> match_uid;
	bug("match_uid: " << match_uid);

	// header
	ifs >> time >> std::ws;//std::getline(ifs, time); // skip time stamp
	ifs >> uid >> std::ws;//std::getline(ifs, uid); // skip uid

	bug_var(time);
	bug_var(uid);
	bug_var(ifs);

	siz n = 0;
	while(ifs >> s)
	{
		if(!match_uid)
		{
			// NOTE: Add proper batching with #<batch number>
			bug("matching: " << s.match);
			if(bot.wild_match("*" + lowercase(param) + "*", lowercase(s.match)))
			{
				if(++n < 8)
				{
					std::ostringstream oss;
					oss << s.uid << ' ' << s.print << ' ' << s.gametype;
					bot.fc_reply(msg, oss.str());
				}
			}
		}
		else if(s.uid == match_uid)
		{
			bot.fc_reply(msg, IRC_BOLD + s.print + IRC_NORMAL);

			// basic HTTP GET
			net::socketstream ss;
			ss.open("dpmaster.deathmask.net", 80);
			ss << "GET " << "http://dpmaster.deathmask.net/?game=openarena&server=" << s.address;
			ss << "\r\n" << std::flush;

			bool ignore = true;
			player p;
			siz ping = 0;
			while(sgl(ss, line))
				if(extract_player(line, p) != str::npos)
				{
					ping = 0;
					siss(p.ping) >> ping;
					if(!ignore && ping) // ignore heading & bots (zero ping)
						bot.fc_reply(msg, IRC_BOLD + html_handle_to_irc(p.handle) + IRC_NORMAL
							+ " " + p.frags + " frags");
					ignore = false;
				}
			return;
		}
	}
}

void PFinderIrcBotPlugin::oafind(const message& msg)
{
	BUG_COMMAND(msg);

	time_t start = std::time(0);

	str handle = msg.get_user_params();
	if(trim(handle).empty())
	{
		bot.fc_reply(msg, help(msg.get_user_cmd()));
		return;
	}

	std::vector<str> found = xoafind(handle);
	const size_t max_matches = bot.get<size_t>(MAX_MATCHES, MAX_MATCHES_DEFAULT);

	if(found.empty())
		bot.fc_reply(msg, "No players found for " + handle + ".");

	else if(found.size() > max_matches)
	{
		found.resize(max_matches);
		std::ostringstream oss;
		oss << "Found " << found.size() << " matches, listing only the first " << max_matches << ".";
		found.push_back(oss.str());
	}

	for(const str& s: found)
		bot.fc_reply(msg, s);

	bot.fc_reply(msg, "Query took: " + std::to_string(std::time(0) - start) + " seconds.");
}

void PFinderIrcBotPlugin::read_links_file(str_set_map& links)
{
	links.clear();
	str entry;
	std::ifstream ifs(bot.getf(LINKS_FILE, LINKS_FILE_DEFAULT));
	while(std::getline(ifs, entry))
	{
		str key, val;
		std::istringstream iss(entry);
		if(iss >> key && std::getline(iss >> std::ws, val))
			links[key].insert(val);
	}
}

void PFinderIrcBotPlugin::write_links_file(const str_set_map& links)
{
	std::ofstream ofs(bot.getf(LINKS_FILE, LINKS_FILE_DEFAULT));
	for(const str_set_pair& link: links)
		for(const str& item: link.second)
			ofs << link.first << ' ' << item << '\n';
	links_changed = true;
}

void PFinderIrcBotPlugin::oalink(const message& msg)
{
	BUG_COMMAND(msg);

	str group, handle;
	std::istringstream iss(msg.get_user_params());

	if(!(iss >> group))
	{
		bot.fc_reply(msg, help(msg.get_user_cmd()));
		return;
	}

	str_set_map links;

	lock_guard lock(links_file_mtx);
	read_links_file(links);

	bool changed = false;
	while(ios::getstring(iss, handle))
	{
		if(links[group].count(handle))
		{
			bot.fc_reply(msg, "Link for: '" + handle + "' is already present.");
		}
		else if(!trim(handle).empty())
		{
			links[group].insert(handle);
			bot.fc_reply(msg, "Link established for: '" + handle + "'.");
			changed = true;
		}
	}

	if(changed)
		write_links_file(links);
}

void PFinderIrcBotPlugin::oalist(const message& msg)
{
	BUG_COMMAND(msg);

	str group = msg.get_user_params();
	if(!trim(group).empty())
	{
		str_set_map links;

		{
			lock_guard lock(links_file_mtx);
			read_links_file(links);
		}

		if(links.empty())
		{
			bot.fc_reply(msg, "Nothing to list.");
			return;
		}

		siz n = 0;
		str delim;
		std::ostringstream oss;

		if(lowercase(group) == "all")
		{
			// list groups
			for(const str_set_pair& link: links)
			{
				oss << delim << IRC_BOLD << "[" << n << "]" << IRC_NORMAL << " '" + link.first + "'";
				if(oss.str().size() < 400)
				{
					delim = ", ";
				}
				else
				{
					bot.fc_reply(msg, "all -> " + oss.str());
					delim = "";
					oss.str("");
				}
				++n;
			}
			if(!oss.str().empty())
				bot.fc_reply(msg, group + " -> " + oss.str());
		}
		else
		{
			// TODO: allow listing of group number <#>
			for(const str& val: links[group])
			{
				oss << delim << IRC_BOLD << "[" << n << "]" << IRC_NORMAL << " '" + val + "'";
				if(oss.str().size() < 400)
				{
					delim = ", ";
				}
				else
				{
					bot.fc_reply(msg, group + " -> " + oss.str());
					delim = "";
					oss.str("");
				}
				++n;
			}
			if(!oss.str().empty())
				bot.fc_reply(msg, group + " -> " + oss.str());
		}
	}
}

void PFinderIrcBotPlugin::oaunlink(const message& msg)
{
	BUG_COMMAND(msg);

	std::istringstream params(msg.get_user_params());

	str group;
	siz_vec items;

	if(!(params >> group))
	{
		log("Missing group");
		return;
	}

//	if(params >> group)
//		while(params >> n)
//			items.push_back(n);

	str range;
	while(sgl(params, range, ','))
	{
		// "1-4", "7", "9-11 ", " 15 - 16" ...
		siss iss(range);

		str lb, ub;
		sgl(sgl(iss, lb, '-'), ub);
		if(lb.empty())
		{
			log("Bad range: " << range);
			continue;
		}

		siz l, u;

		if(!(siss(lb) >> l))
		{
			log("Bad range (unrecognized number): " << range);
			continue;
		}

		if(ub.empty() || !(siss(ub) >> u))
			u = l;

		if(u < l)
		{
			log("Bad range (higher to lower): " << range);
			continue;
		}

		for(siz i = l; i <= u; ++i)
			items.push_back(i);
	}

	if(trim(group).empty() || items.empty())
	{
		bot.fc_reply(msg, help(msg.get_user_cmd()));
		return;
	}

	for(siz item: items)
		bug("item: " << item);

	str_set_map links;

	lock_guard lock(links_file_mtx);
	read_links_file(links);

	if(links.empty())
	{
		bot.fc_reply(msg, "Nothing to unlink.");
		return;
	}


	bool changed = false;
	if(group == "all")
	{
		// TODO: Make this unlink a whole group
		bot.fc_reply(msg, "Unlinking from 'all' is not possible.");
	}
	else
	{
		const str_vec entries(links[group].begin(), links[group].end());
		for(siz i: items)
			if(i < entries.size())
				changed |= links[group].erase(entries[i]);
	}

	if(changed)
		write_links_file(links);

	bot.fc_reply(msg, "Unlinking complete.");
}

std::istream& getmsg(std::istream& is, message& msg)
{
	std::getline(is, msg.from);
	std::getline(is, msg.cmd);
	std::getline(is, msg.to);
	std::getline(is, msg.text);
	return is;
}

std::ostream& putmsg(std::ostream& os, const message& msg)
{
	os << msg.from << '\n';
	os << msg.cmd << '\n';
	os << msg.to << '\n';
	os << msg.text;
	return os;
}

struct tell
{
	time_t from;
	message msg;
	str nick;
	size_t minutes;
};

std::istream& gettell(std::istream& is, tell& t)
{
	is >> t.from >> std::ws;
	getmsg(is, t.msg);
	std::getline(is, t.nick);
	is >> t.minutes >> std::ws;
	return is;
}

std::ostream& puttell(std::ostream& os, const tell& t)
{
	os << t.from << '\n';
	putmsg(os, t.msg) << '\n';
	os << t.nick << '\n';
	os << t.minutes;
	return os;
}

static bool done = false;

void tell_runner(std::function<void()> func)
{
	time_t t = time(0);
	while(!done)
	{
		while(!done && (time(0) - t) < 60)
			std::this_thread::sleep_for(std::chrono::seconds(1));
		if(!done)
			func();
		t = time(0);
	}
}

void PFinderIrcBotPlugin::check_tell()
{
	//bug_func();
	std::ifstream ifs(bot.getf(TELL_FILE, TELL_FILE_DEFAULT));
	std::ostringstream oss; // next list
	if(ifs)
	{
		tell t;
		while(gettell(ifs, t))
		{
			size_t diff = (time(0) - t.from) / 60;
			if(diff < t.minutes)
			{
				std::vector<str> found = oafind(t.nick);
				if(found.empty())
					puttell(oss, t) << '\n';

				else
				{
					const size_t max_matches = bot.get<size_t>("maxlines", 6);
					if(found.size() > max_matches)
					{
						found.resize(max_matches);
						std::ostringstream oss;
						oss << "Found " << found.size() << " matches, listing only the first " << max_matches << ".";
						found.push_back(oss.str());
					}

					for(const str& s: found)
						bot.fc_reply_pm(t.msg, s);
				}
			}
			else
			{
				bot.fc_reply_pm(t.msg, IRC_COLOR + IRC_Red + "tell: " + oatoirc(t.nick) + " has expired.");
			}
		}
	}
	ifs.close();
	std::ofstream ofs(bot.getf(TELL_FILE, TELL_FILE_DEFAULT), std::ios::trunc);
	if(ofs) ofs << oss.str();
}

void PFinderIrcBotPlugin::oatell(const message& msg)
{
	BUG_COMMAND(msg);

	std::istringstream iss(msg.get_user_params());
	str nick;
	size_t minutes = 60;
	iss >> nick >> minutes;

	if(!trim(nick).empty())
	{
		std::ofstream ofs(bot.getf(TELL_FILE, TELL_FILE_DEFAULT), std::ios::app);
		if(ofs)
		{
			ofs << std::time(0) << '\n';
			putmsg(ofs, msg) << '\n';
			ofs << nick << '\n';
			ofs << minutes << '\n';
		}

		bot.fc_reply(msg, nick + " added to tell list.");
	}
}

static std::future<void> fut;

bool PFinderIrcBotPlugin::initialize()
{
//	read_automsgs();
	// TODO: oawhois (GUID database)
	add
	({
		"!frags"
		, "!frags - Message the frag roster to see who wants to play."
		, [&](const message& msg){ oafind(msg); }
	});
	add
	({
		"!oafind"
		, "!oafind <player|nick> Try to find players on OA servers matching the supplied <player>."
		, [&](const message& msg){ oafind(msg); }
	});
	add
	({
		"!oalink"
		, "!oalink <nick|group> <player>* Link either an IRC <nick> or a group name <group> with one or more (*) OA players <player>*"
			" in order to search for players on OA servers using either their IRC <nick> or a group name <group>."
		, [&](const message& msg){ oalink(msg); }
	});
	add
	({
		"!oaunlink"
		, "!oaunlink <nick|group> 1-3, 5, 7-8 Unlink either a list of numbered items or ranges from an IRC <nick> or a <group> added through !oalink."
		, [&](const message& msg){ oaunlink(msg); }
	});
	add
	({
		"!oalist"
		, "!oalist <nick> List which players are linked to a given IRC <nick>."
		, [&](const message& msg){ oalist(msg); }
	});
	add
	({
		"!oatell"
		, "!oatell <player> [<minutes>:60] Notify when player joins a server."
		, [&](const message& msg){ oatell(msg); }
	});
	add
	({
		"!oacvar"
		, "!oacvar DEPRECATED, please use !cvar."
		, [&](const message& msg){ cvar(msg); }
	});
	add
	({
		"!cvar"
		, "!cvar <cvar> Match <cvar> as part of an OpenArena cvar, providing info."
		, [&](const message& msg){ cvar(msg); }
	});
	add
	({
		"!oaname"
		, "!oaname <name> Convert OA name colours."
		, [&](const message& msg){ bot.fc_reply(msg, oa_to_IRC(msg.get_user_params().c_str())); }
	});
	add
	({
		"!oaserver"
		, "!oaserver <name>|<uid> List servers matching <name> or players on server <uid>."
		, [&](const message& msg){ oaserver(msg); }
	});
	bot.add_rpc_service(*this);
	fut = std::async(std::launch::async, [&]{ tell_runner([&]{check_tell();}); });
	return true;
}

// INTERFACE: IrcBotPlugin

str PFinderIrcBotPlugin::get_name() const { return NAME; }
str PFinderIrcBotPlugin::get_version() const { return VERSION; }

void PFinderIrcBotPlugin::exit()
{
//	bug_func();
	done = true;
	if(fut.valid())
		if(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			fut.get();
//	if(fut.valid())
//		fut.get();
}

//bool lookup_players(const str& search, std::vector<str>& handles);
//bool match_player(bool substitution, const str& name1, const str& name2);

bool PFinderIrcBotPlugin::rpc(rpc::call& c)
{
	str func = c.get_func();
	if(func == "lookup_players")
	{
		str search = c.get_param<str>();
		std::vector<str> handles = c.get_param<std::vector<str>>();
		c.set_return_value(lookup_players(search, handles));
		c.add_return_param(handles);
		return true;
	}
	else if(func == "match_player")
	{
		bool subst = c.get_param<bool>();
		str name1 = c.get_param<str>();
		str name2 = c.get_param<str>();
		c.set_return_value(match_player(subst, name1, name2));
		return true;
	}
	else if(func == "html_handle_to_irc")
	{
		str html = c.get_param<str>();
		c.set_return_value(html_handle_to_irc(html));
		return true;
	}
	return false;
}

}} // sookee::ircbot
