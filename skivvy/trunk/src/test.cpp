#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <skivvy/stl.h>
#include <skivvy/str.h>
#include <skivvy/rcon.h>
#include <skivvy/types.h>
#include <skivvy/logrep.h>
#include <skivvy/network.h>
#include <skivvy/socketstream.h>

using namespace skivvy;
using namespace skivvy::types;
using namespace skivvy::utils;

// http://www.ip-adress.com/reverse_ip/83.100.193.172

const str VERSION = "1.0";

int main(int argc, char* argv[])
{
	str_vec args;

	if(argc < 2)
	{
		str ip;
		while(std::cin >> ip)
			args.push_back(ip);
	}
	else
	{
		args.assign(argv + 1, argv + argc);
	}

	for(const str& ip: args)
	{
		net::socketstream ss;
		ss.open("www.ip-adress.com", 80);
		ss << "GET /reverse_ip/" << ip << " HTTP/1.1\r\n";
		ss << "Host: www.ip-adress.com" << "\r\n";
		ss << "User-Agent: Skivvy: " << VERSION << "\r\n";
		ss << "Accept: text/html" << "\r\n";
		ss << "\r\n" << std::flush;

		net::header_map headers;
		if(!net::read_http_headers(ss, headers))
		{
			log("ERROR reading headers.");
			return false;
		}

		for(const net::header_pair& hp: headers)
			con(hp.first << ": " << hp.second);

		str html;
		if(!net::read_http_response_data(ss, headers, html))
		{
			log("ERROR reading response data.");
			return false;
		}
		ss.close();

		// <h3> IP:</h3>83.100.193.172<h3> server location:</h3>Kingston Upon Hull in United Kingdom<h3> ISP:</h3>Kcom<!-- google_ad_section_end -->

		str line;
		std::istringstream iss(html);
		while(std::getline(iss, line))
		{
			if(line.find("<h3>"))
				continue;
			siz pos;
			if((pos = line.find("ISP:</h3>")) != str::npos)
			{
				line = line.substr(pos + 9);
				if((pos = line.find('<')) != str::npos)
				{
					con("ISP: " << ip << ' ' << line.substr(0, pos));
					break;
				}
			}
		}
	}
}

