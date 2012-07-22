/*
 * network.cpp
 *
 *  Created on: 12 Aug 2011
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
#include <skivvy/network.h>

#include <string>
#include <sstream>
#include <ctime>
#include <iomanip>

#include <skivvy/str.h>
#include <skivvy/logrep.h>

namespace skivvy { namespace net {

using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

cookie::cookie()
: secure(false)
, httponly(false)
{
}

cookie::cookie(const str& data)
: secure(false)
, httponly(false)
{
	std::istringstream iss(data);
	std::getline(iss, key, '=');
	std::getline(iss, val, ';');
	iss >> std::ws;

	str att;
	while(std::getline(iss, att, '='))
	{
		if(att == "Domain")
			std::getline(iss, domain, ';');
		else if(att == "Path")
			std::getline(iss, path, ';');
		else if(att == "Expires")
		{
			std::getline(iss, att, ';');
			// Wdy, DD-Mon-YYYY HH:MM:SS GMT
			std::istringstream iss(att);
			std::tm t;
			t.tm_gmtoff = 0;
			t.tm_zone = "GMT";
			t.tm_isdst = 0;
			std::getline(iss, att, ',');
			iss >> t.tm_mday;
			iss.ignore();
			iss >> t.tm_mon;
			iss.ignore();
			iss >> t.tm_year;
			t.tm_year -= 1900;
			iss >> t.tm_hour;
			iss.ignore();
			iss >> t.tm_min;
			iss.ignore();
			iss >> t.tm_sec;
			expires = std::mktime(&t);
		}
		else if(att == "Secure")
			secure = true;
		else if(att == "HttpOnly")
			httponly = true;
		iss >> std::ws;
	}
}

bool cookie::is_expired()
{
	return !expires > time(0);
}

str urlencode(const str& url)
{
	static const str plain = "";
	std::ostringstream oss;
	str::const_iterator i = url.begin();
	for(; i != url.end(); ++i)
	{
		if(isalnum(*i) || plain.find(*i) != str::npos) { oss << *i; }
		else { oss << "%" << std::hex << int(*i); }
	}
	return oss.str();
}

// TODO: implement domain checking
std::string get_cookie_header(const cookie_jar& cookies, const str& /*domain*/)
{
//	bug_func();
	std::ostringstream oss;
	if(!cookies.empty())
	{
		oss << "Cookie:";
		str sep = " ";
		for(const cookie_pair& c: cookies)
		{
			oss << sep << c.second.key << "=" << c.second.val;
			sep = "; ";
		}
	}
	return oss.str();
}

std::istream& read_http_headers(std::istream&is, header_map& headers)
{
//	bug_func();
	str line;

	if(!std::getline(is, line))
	{
		log("STREAM ERROR: " << line);
		log("          is: " << is);
		return is;
	}

	if(line.find("200") == str::npos || line.find("OK") == str::npos)
	{
		log("BAD RESPONSE: " << line);
	}

	str key, val;
	std::istringstream iss;

	while(is && std::getline(is, line) && !trim(line).empty())
	{
		iss.clear();
		iss.str(line);
		if(iss >> key && std::getline(iss, val))
		{
//			bug("header: " << key << " " << val);
			headers.insert(std::make_pair(lowercase(trim(key, " \t\r\n:")), trim(val)));
		}
	}
	return is;
}


std::istream& read_chunked_encoding(std::istream& is, str& data)
{
//	bug_func();
	data.clear();
	size_t len = 0;
	str line;
	std::ostringstream oss;
	while(is >> std::hex >> len && len)
	{
//		bug("len: " << len);
		std::getline(is, line); // move to data start
		oss.clear();
		oss.str("");
		for(char c; len && is.get(c); --len) oss.put(c);
		data.append(oss.str());
	}
	return is;
}

std::istream& read_http_response_data(std::istream&is, const header_map& headers, str& data)
{
//	bug_func();
	for(const net::header& h: headers)
	{
//		bug("header: " << h.first << ": " << h.second);
		if(h.first == "transfer-encoding" && h.second == "chunked")
			return read_chunked_encoding(is, data);
		else if(h.first == "content-length")
		{
			size_t len;
			if(std::istringstream(h.second) >> len)
			{
				std::ostringstream oss;
				for(char c; len && is.get(c); --len) oss.put(c);
				data = oss.str();
				return is;
			}
			else
				log("BAD content-length.");
		}
	}
	log("Unknown data response method.");
	return is;
}

std::istream& read_http_cookies(std::istream& is, const header_map& headers, cookie_jar& cookies)
{
	//bug_func();
	for(const header& h: headers)
	{
		if(h.first == "set-cookie")
		{
			cookie c(h.second);
			cookies[c.domain + "/" + c.path] = c;
		}
	}
	return is;
}

// HTML

static const str_map ents =
{
	{"&lt;", "<"}
	, {"&#60;", ">"}
	, {"&gt;", ">"}
	, {"&#61;", ">"}
	, {"&nbsp;", " "}
	, {"&#160;", " "}
	, {"&amp;", "&"}
	, {"&#38;", "&"}
	, {"&cent;", "¢"}
	, {"&#162;", "¢"}
	, {"&pound;", "£"}
	, {"&#163;", "£"}
	, {"&euro;", "€"}
	, {"&#8364;", "€"}
	, {"&sect;", "§"}
	, {"&#167;", "§"}
	, {"&copy;", "©"}
	, {"&#169;", "©"}
	, {"&reg;", "®"}
	, {"&#174;", "®"}
	, {"&trade;", "™"}
	, {"&#8482;", "™"}
	, {"&quot;", "\""}
};

str fix_entities(std::string s)
{
//	bug_func();
	str::size_type p;
	for(const str_pair& e: ents)
		while((p = s.find(e.first)) != str::npos)
			s.replace(p, e.first.size(), e.second);
	return s;
}

str urldecode(std::string s)
{
//	bug_func();
	static str_map urlism;
	if(urlism.empty())
	{
		for(size_t i = 0; i < 256; ++i)
		{
			std::ostringstream oss;
			oss.fill('0');
			oss << "%" << std::setw(2) << std::hex << i;
			//std::cout << "adding: " << oss.str() << '\n';

			urlism[lowercase(oss.str())] = str(1, char(i));
			urlism[uppercase(oss.str())] = str(1, char(i));
		}
	}
	str::size_type p;
	for(const str_pair& e: urlism)
	{
		while((p = s.find(e.first)) != str::npos)
		{
			//std::cout << "found: " << e.first << '\n';
			s.replace(p, e.first.size(), e.second);
		}
	}
	return s;
}

std::istream& read_tag(std::istream& is, std::ostream& os, const str& tag)
{
	bool done = false;
	str s;
	while(!done && std::getline(is, s, '<'))
	{
		if(std::getline(is, s, '>') && s.find(tag) == 0)
		{
			if(std::getline(is, s, '<')) os << s;

			while(std::getline(is, s, '>') && s.find("/" + tag) == str::npos)
			{
				os << '<' << s << '>';
				if(std::getline(is, s, '<')) os << s;
			}
			done = true;
		}
	}
	return is;
}

std::istream& read_tag_by_att(std::istream& is, std::ostream& os, const str& tag, const str& att, const str& val)
{
	bool done = false;
	str s;
	const str att1 = att + "='" + val + "'";
	const str att2 = att + "=\"" + val + "\"";

	while(!done && std::getline(is, s, '<'))
	{
		if(std::getline(is, s, '>') && s.find(tag) == 0 && (s.find(att1) != str::npos || s.find(att2) != str::npos))
		{
			if(std::getline(is, s, '<')) os << s;

			while(std::getline(is, s, '>') && s.find("/" + tag) == str::npos)
			{
				os << '<' << s << '>';
				if(std::getline(is, s, '<')) os << s;
			}
			done = true;
		}
	}
	return is;
}

void html_to_text(std::istream& i, std::ostream& o)
{
	for(std::string s; std::getline(i, s, '<'); std::getline(i, s, '>'))
		o << urldecode(fix_entities(s));
}

str html_to_text(const str& html)
{
	std::istringstream i(html);
	std::ostringstream o;
	html_to_text(i, o);
	return o.str();
}

}} // sookee::net