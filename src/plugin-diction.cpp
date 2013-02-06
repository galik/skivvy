/*
 * ircbot-diction.cpp
 *
 *  Created on: 21 Nov 2011
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

#include <skivvy/plugin-diction.h>

#include <fstream>
#include <sstream>
#include <iostream>

#include <skivvy/irc.h>
#include <skivvy/logrep.h>
#include <skivvy/network.h>
#include <skivvy/str.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(DictionIrcBotPlugin);
PLUGIN_INFO("diction", "Dictionary Lookup", "0.1");

using namespace skivvy::irc;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

const str HOST = "diction.host";
const str HOST_DEFAULT = "pan.alephnull.com";
const str PORT = "diction.port";
const siz PORT_DEFAULT = 2628U;

DictionIrcBotPlugin::DictionIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
{
}

DictionIrcBotPlugin::~DictionIrcBotPlugin() {}

//const str_map langmap = { {"", ""}, {"", ""} };

const str_map langmap =
{
	{"af", "Afrikaans"}
	, {"sq", "Albanian"}
	, {"ar", "Arabic"}
	, {"be", "Belarusian"}
	, {"bg", "Bulgarian"}
	, {"ca", "Catalan"}
	, {"zh-CN", "Chinese"}
	, {"hr", "Croatian"}
	, {"cs", "Czech"}
	, {"da", "Danish"}
	, {"nl", "Dutch"}
	, {"en", "English"}
	, {"et", "Estonian"}
	, {"tl", "Filipino"}
	, {"fi", "Finnish"}
	, {"fr", "French"}
	, {"gl", "Galician"}
	, {"de", "German"}
	, {"el", "Greek"}
	, {"ht", "Haitian Creole"}
	, {"iw", "Hebrew"}
	, {"hi", "Hindi"}
	, {"hu", "Hungarian"}
	, {"is", "Icelandic"}
	, {"id", "Indonesian"}
	, {"ga", "Irish"}
	, {"it", "Italian"}
	, {"ja", "Japanese"}
	, {"ko", "Korean"}
	, {"lv", "Latvian"}
	, {"lt", "Lithuanian"}
	, {"mk", "Macedonian"}
	, {"ms", "Malay"}
	, {"mt", "Maltese"}
	, {"no", "Norwegian"}
	, {"fa", "Persian"}
	, {"pl", "Polish"}
	, {"pt", "Portuguese"}
	, {"pt-PT", "Portuguese(Portugal)"}
	, {"ro", "Romanian"}
	, {"ru", "Russian"}
	, {"sr", "Serbian"}
	, {"sk", "Slovak"}
	, {"sl", "Slovenian"}
	, {"es", "Spanish"}
	, {"sw", "Swahili"}
	, {"sv", "Swedish"}
	, {"th", "Thai"}
	, {"tr", "Turkish"}
	, {"uk", "Ukrainian"}
	, {"vi", "Vietnamese"}
	, {"cy", "Welsh"}
	, {"yi", "Yiddish"}
};

void DictionIrcBotPlugin::langs(const message& msg)
{
	str lang = lowercase(msg.get_user_params_cp());
	trim(lang);

	str sep;
	std::ostringstream oss;
	bool found = false;
	for(const str_pair& l: langmap)
		if(lang.empty() || lowercase(l.second).find(lang) != str::npos)
		{
			if(oss.str().size() > 200)
			{
				bot.fc_reply(msg, "02langs:  " + oss.str());
				oss.str("");
				found = true;
			}
			oss << sep << l.first << ": " << l.second;
			sep = ", ";
		}

	lang = oss.str();

	if(!found && lang.empty())
	{
		bot.fc_reply(msg, "03langs:  None found.");
		return;
	}

	if(!lang.empty())
		bot.fc_reply(msg, "02langs:  " + lang);
}
void DictionIrcBotPlugin::trans(const message& msg)
{
	BUG_COMMAND(msg);

	std::istringstream iss(msg.get_user_params_cp());

	str from = "en";
	str to = "en";

	if(!(iss >> from >> to))
	{
		bot.fc_reply(msg, "05trans:  !trans <from> <to> <phrase>.");
		return;
	}

	if(langmap.find(from) == langmap.end())
	{
		bot.fc_reply(msg, "05trans:  Base language " + from + " not understood. Check !langs for language codes.");
		return;
	}

	if(langmap.find(to) == langmap.end())
	{
		bot.fc_reply(msg, "05trans:  Target language " + to + " not understood. Check !langs for language codes.");
		return;
	}

	str word;
	std::getline(iss >> std::ws, word);

	bug("from: " << from);
	bug("  to: " << to);
	bug("word: " << word);

	word = net::urlencode(word);

	net::socketstream ss;
	ss.open("translate.reference.com", 80);
	ss << "GET /translate?query=" << word << "&src=" << from << "&dst=" << to << " HTTP/1.1\r\n";
	ss << "Host: translate.reference.com" << "\r\n";
	ss << "User-Agent: Skivvy: " << VERSION << "\r\n";
	ss << "Accept: text/html" << "\r\n";
	ss << "\r\n" << std::flush;

	net::header_map headers;
	if(!net::read_http_headers(ss, headers))
	{
		log("ERROR reading headers.");
		return;
	}

	for(const net::header_pair& hp: headers)
		con(hp.first << ": " << hp.second);

	//cookies.clear();
	if(!net::read_http_cookies(ss, headers, cookies))
	{
		log("ERROR reading cookies.");
		return;
	}

	str html;
	if(!net::read_http_response_data(ss, headers, html))
	{
		log("ERROR reading response data.");
		return;
	}

//	<div class="translateTxt" >
//	or is the rabbit
//	</div>

//	std::ofstream ofs("dump.html");
//	ofs.write(html.c_str(), html.size());
//	ofs.close();

	str div;
	if(extract_delimited_text(html, R"(<div class="translateTxt" >)", R"(</div>)", div, 0) == str::npos)
	{
		bot.fc_reply(msg, "05trans:  No translation found.");
		return;
	}
	bot.fc_reply(msg, "06trans: " + trim(div));
}

void DictionIrcBotPlugin::ddg_dict(const message& msg)
{
	BUG_COMMAND(msg);

	const str word = net::urlencode(msg.get_user_params_cp());

	net::socketstream ss;
	ss.open("duckduckgo.com", 80);
	ss << "GET " << "/?q=define%3A+" << word << " HTTP/1.1\r\n";
	ss << "Host: duckduckgo.com" << "\r\n";
	ss << "User-Agent: Skivvy: " << VERSION << "\r\n";
	//ss << "Referer: http://www.google.com/search?q=define%3A+" << word << "\r\n";
	ss << "Accept: text/html" << "\r\n";
	ss << "\r\n" << std::flush;

	net::header_map headers;
	if(!net::read_http_headers(ss, headers))
	{
		log("ERROR reading headers.");
		return;
	}

	//cookies.clear();
	if(!net::read_http_cookies(ss, headers, cookies))
	{
		log("ERROR reading cookies.");
		return;
	}

	str html;
	if(!net::read_http_response_data(ss, headers, html))
	{
		log("ERROR reading response data.");
		return;
	}

//	std::ofstream ofs("dump.html");
//	ofs.write(html.c_str(), html.size());
//	ofs.close();

	str a;
	if(extract_delimited_text(html, R"(<a class="snippet")", R"(/a>)", a, 0) != str::npos)
	{
		str defn;
		if(extract_delimited_text(a, R"(>)", R"(<)", defn, 0) == str::npos)
		{
			log("DICT: Error parsing definition.");
			return;
		}
		bot.fc_reply(msg, "06dict: " + defn);
	}

	siz n = 0;
	std::istringstream div_iss(html);
	std::ostringstream div_oss;
	while(n < 5 && net::read_tag_by_att(div_iss, div_oss, "div", "class", "links_zero_click_disambig highlight_2"))
	{
		std::istringstream div_iss(div_oss.str());
		div_oss.clear();
		div_oss.str("");
		str option, defn;
		if(net::read_tag(div_iss, div_oss, "a"))
		{
			option = div_oss.str();
			div_oss.clear();
			div_oss.str("");
			if(std::getline(div_iss, defn))
			{
				bot.fc_reply(msg, "06dict: " + std::to_string(1 + n++) + ". (" + option + ") " + net::html_to_text(defn));
			}
		}
	}
}

void DictionIrcBotPlugin::google_dict(const message& msg)
{
	BUG_COMMAND(msg);

	const str word = net::urlencode(msg.get_user_params_cp());

	net::socketstream ss;
	ss.open("www.merriam-webster.com", 80);
	ss << "GET " << "/dictionary/" << word << " HTTP/1.1\r\n";
	ss << "Host: www.merriam-webster.com" << "\r\n";
	ss << "User-Agent: Skivvy: " << VERSION << "\r\n";
	ss << "Accept: text/html" << "\r\n";
	ss << "\r\n" << std::flush;

	net::header_map headers;
	if(!net::read_http_headers(ss, headers))
	{
		log("ERROR reading headers.");
		return;
	}

	//cookies.clear();
	if(!net::read_http_cookies(ss, headers, cookies))
	{
		log("ERROR reading cookies.");
		return;
	}

	str html;
	if(!net::read_http_response_data(ss, headers, html))
	{
		log("ERROR reading response data.");
		return;
	}

//	std::ofstream ofs("dump.html");
//	ofs.write(html.c_str(), html.size());
//	ofs.close();

	str a;
	if(extract_delimited_text(html, R"(<a class="snippet")", R"(/a>)", a, 0) != str::npos)
	{
		str defn;
		if(extract_delimited_text(a, R"(>)", R"(<)", defn, 0) == str::npos)
		{
			log("DICT: Error parsing definition.");
			return;
		}
		bot.fc_reply(msg, "06dict: " + defn);
	}

	siz n = 0;
	std::istringstream div_iss(html);
	std::ostringstream div_oss;
	while(n < 5 && net::read_tag_by_att(div_iss, div_oss, "div", "class", "links_zero_click_disambig highlight_2"))
	{
		std::istringstream div_iss(div_oss.str());
		div_oss.clear();
		div_oss.str("");
		str option, defn;
		if(net::read_tag(div_iss, div_oss, "a"))
		{
			option = div_oss.str();
			div_oss.clear();
			div_oss.str("");
			if(std::getline(div_iss, defn))
			{
				bot.fc_reply(msg, "06dict: " + std::to_string(1 + n++) + ". (" + option + ") " + net::html_to_text(defn));
			}
		}
	}
}

void DictionIrcBotPlugin::dict(const message& msg)
{
	BUG_COMMAND(msg);
	google_dict(msg);
	return;
	std::string word = msg.get_user_params_cp();

	net::socketstream ss;
	//ss.open("miranda.org", 2628);

	str host = bot.get(HOST, HOST_DEFAULT);
	uint16_t port = bot.get(PORT, PORT_DEFAULT);

	ss.open(host, port);
	std::string line;
	if(!std::getline(ss, line))
	{
		log(host + ":" + std::to_string(port) + " did not respond.");
		return;
	}
	bug("line: " << line);

	std::string code;
	std::istringstream iss(line);

	// 220 miranda.org dictd 1.9.15/rf on Linux 2.6.32-5-686 <auth.mime> <42786878.6063.1321914539@miranda.org>
	if(iss >> code && code != "220")
	{
		if(code == "552")
		{
			bot.fc_reply(msg, word + ", was not found.");
			return;
		}
		log("Unexpected response: " << line);
		return;
	}

	ss << "DEFINE ! '" + word + "'" << std::endl;

	std::getline(ss, line);
	iss.clear();
	iss.str(line);

	// 150 6 definitions retrieved
	if(iss >> code && code != "150")
	{
		if(code == "552")
		{
			bot.fc_reply(msg, IRC_COLOR + IRC_Royal_Blue
				+ "No entry for '" + word + "'" + IRC_NORMAL);
			return;
		}
		log("Unexpected response: " << line);
		return;
	}

	size_t defs;
	if(!(iss >> defs))
	{
		log("Unexpected number: " << line);
		return;
	}

	size_t done = 0;

	while(std::getline(ss, line) && line.substr(0, 3) != "151");
	std::getline(ss, line, '[');
	bot.fc_reply(msg, IRC_COLOR + IRC_Royal_Blue + line + IRC_NORMAL);

	while(std::getline(ss, line) && (trim(line).empty() || !std::isdigit(line[0])));

	std::string dfn;
	do
	{
		dfn += " " + line;
	}
	while(std::getline(ss, line) && !trim(line).empty());

	while(std::getline(ss, line))
	{
		bool output = true;
		bug("word: " << line);
		bot.fc_reply(msg, IRC_BOLD + IRC_COLOR + IRC_Royal_Blue + line.substr(4) + IRC_NORMAL);
		while(done < 10 && std::getline(ss, line) && line[0] != '.')
		{
			bug("defn: " << line);

			std::istringstream iss(line);
			std::getline(iss, line, '[');
			trim(line);

			if(!line.empty() && std::isdigit(line[0]))
				output = true;

			if(output)
			{
				bot.fc_reply(msg, IRC_COLOR + IRC_Royal_Blue + line + IRC_NORMAL);
				++done;
			}
			if(iss.get() || line.empty())
				output = false;
		}
	}
	while(done < 10 && std::getline(ss, line) && line.substr(0, 3) == "151");

//	do
//	{
//		bool output = true;
//		bug("word: " << line);
//		bot.fc_reply(msg, IRC_BOLD + IRC_COLOR + IRC_Royal_Blue + line.substr(4) + IRC_NORMAL);
//		while(done < 10 && std::getline(ss, line) && line[0] != '.')
//		{
//			bug("defn: " << line);
//
//			std::istringstream iss(line);
//			std::getline(iss, line, '[');
//			trim(line);
//
//			if(!line.empty() && std::isdigit(line[0]))
//				output = true;
//
//			if(output)
//			{
//				bot.fc_reply(msg, IRC_COLOR + IRC_Royal_Blue + line + IRC_NORMAL);
//				++done;
//			}
//			if(iss.get() || line.empty())
//				output = false;
//		}
//	}
//	while(done < 10 && std::getline(ss, line) && line.substr(0, 3) == "151");
//
	if(done == 10)
		bot.fc_reply(msg, IRC_COLOR + IRC_Royal_Blue + "== Flood Limit ==" + IRC_NORMAL);

	bug("quitting:");
	ss << "QUIT" << std::endl;
	ss.close();
}

// INTERFACE: BasicIrcBotPlugin

bool DictionIrcBotPlugin::initialize()
{
	add
	({
		"!dict"
		, "!dict <word|phrase> Look word up in dictionary."
		, [&](const message& msg){ dict(msg); }
	});
	add
	({
		"!trans"
		, "!trans <from> <to> <word|phrase> Translate <word|phrase> between languages <from> and <to> (see !langs)."
		, [&](const message& msg){ trans(msg); }
	});
	add
	({
		"!langs"
		, "!langs [<lang>] List matching languages and their codes. If no <lang> is given all languages are displayed."
		, [&](const message& msg){ langs(msg); }
	});
	return true;
}

// INTERFACE: IrcBotPlugin

std::string DictionIrcBotPlugin::get_id() const { return ID; }
std::string DictionIrcBotPlugin::get_name() const { return NAME; }
std::string DictionIrcBotPlugin::get_version() const { return VERSION; }

void DictionIrcBotPlugin::exit()
{
//	bug_func();
}

// INTERFACE: IrcBotMonitor

}} // sookee::ircbot

