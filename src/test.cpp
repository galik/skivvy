#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <skivvy/stl.h>
#include <skivvy/str.h>
//#include <skivvy/rcon.h>
#include <skivvy/types.h>
#include <skivvy/ircbot.h>
#include <skivvy/logrep.h>
#include <skivvy/network.h>
#include <skivvy/socketstream.h>

//#include <boost/regex.hpp>

using namespace skivvy;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;
using namespace skivvy::ircbot;

//bool preg_match(const str& s, const str& r)
//{
//	bug_func();
//	using namespace boost;
//
////	regex reg(r, regex::perl | regex::icase);
//	regex reg("b");//, regex::perl | regex::icase);
//	bug("reg:");
////	bool res = regex_match(s, reg);//, match_default);
//	bool res = regex_match("a", reg, match_default);
//	bug("res:");
//	return res;
//}
//
//bool preg_match(const str& s, const str& r, str_vec& matches)
//{
//	bug_func();
//	using namespace boost;
//
//	regex e(r, regex::perl | regex::icase);
//	bug("reg:");
//	smatch m;
//	regex_match(s, m, e, match_default);
//	bug("match:");
//	matches.assign(m.begin(), m.end());
//	bug("assign:");
//	return !matches.empty();
//}


int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		std::cerr << "Requires 2 parameters: <text> <regex>" << '\n';
		exit(1);
	}

	str s(argv[1]);
	str r(argv[2]);

//	std::cout << "match: " << std::ios::boolalpha << preg_match(s, r) << '\n';

//	str_vec matches;
//
//	if(preg_match(s, r, matches))
//		for(const str& m: matches)
//			std::cout << "match: " << m << '\n';
}

