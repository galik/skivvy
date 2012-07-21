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
using namespace skivvy::string;

#define VERSION "0.1"

bool get_http(const str& url, str& html)
{
//	bug_func();
	// http://my.ip.com[:port]/path/to/resource
	static net::cookie_jar cookies;

	str prot;
	str host;
	str path;
	str skip;

	std::istringstream iss(url);

	std::getline(iss, prot, ':');
	std::getline(iss, skip, '/');
	std::getline(iss, skip, '/');
	std::getline(iss, host, '/');
	std::getline(iss, path);

//	bug_var(prot);
//	bug_var(host);
//	bug_var(path);

	if(prot != "http")
	{
		return false;
	}

	siz port = 80;

	// extract optional port
	if(host.find(':') != str::npos)
	{
		iss.clear();
		iss.str(host);
		if(!(std::getline(iss, host, ':') >> port))
		{
			return false;
		}
	}
//	bug_var(host);
//	bug_var(port);

	net::socketstream ss;
	ss.open(host, port);
	ss << "GET /" << path << " HTTP/1.1\r\n";
	ss << "Host: " << host << "\r\n";
	ss << "User-Agent: Skivvy: " << VERSION << "\r\n";
	ss << "Accept: text/html" << "\r\n";
	ss << net::get_cookie_header(cookies, host) << "\r\n";
	ss << "\r\n" << std::flush;

	net::header_map headers;
	if(!net::read_http_headers(ss, headers))
	{
		log("ERROR reading headers.");
		return false;
	}

	net::read_http_cookies(ss, headers, cookies);

//	for(const net::header_pair& hp: headers)
//		con(hp.first << ": " << hp.second);

	if(!net::read_http_response_data(ss, headers, html))
	{
		log("ERROR reading response data.");
		return false;
	}
	ss.close();

	return true;
}

bool get_onlinequizarea_quiz()
{
	bug_func();
	const str URL_PREFIX = "http://www.onlinequizarea.com/multiple-choice/general-knowledge-quiz/start/added&next=";
	const str URL_SUFFIX = "/32/1/";
	const str LOC = "question_and_answer_quizzes.php?";

	std::ofstream ofs("data/quiz-rip.txt");

	str url, html;
	for(siz i = 1; i <= 300; i += 30)
	{
		url = URL_PREFIX + std::to_string(i) + URL_SUFFIX;
		if(!get_http(url, html))
			continue;

		str line;
		std::istringstream iss(html);

		str question, answer, skip;
		bool correct = false;

		// <a href="question_and_answer_quizzes.php?cat_title=general-knowledge&amp;type_title=multiple-choice&amp;cat=32&amp;type=1&amp;&amp;id=6323"
		while(std::getline(iss, line))
		{
			if(line.find("</div><div class=\"clear8\">&nbsp;</div><form action=\"\" method=\"get\" name=\"quiz_form\">"))
				continue;

			for(siz end, pos = 0; (pos = line.find(LOC, pos)) != str::npos; pos += LOC.size())
				if((end = line.find('"', pos)) != str::npos)
				{
					url = "http://www.onlinequizarea.com/" + net::fix_entities(net::urldecode(line.substr(pos, end - pos)));
					bug_var(url);

					if(!get_http(url, html))
						continue;

					std::istringstream iss(html);

					while(std::getline(iss, line))
					{
						if(line.find("left green") != str::npos)
						{
							correct = false;
							if((pos = line.find(".</span>")) == str::npos)
								continue;
							pos += 8;
							if((end = line.find('<', pos)) == str::npos)
								continue;
							question = line.substr(pos, end - pos);
							ofs << '\n';
							ofs << trim(question) << '\n';
						}
						else if((pos = line.find("checkanswer(")) != str::npos)
						{
							str id, num, xml;
							// checkanswer('14105','1',document.quiz_form.question_14105,6323, 1, 'q_and_a_4');
							std::istringstream iss(line.substr(pos));
							std::getline(iss, skip, '\'');
							std::getline(iss, id, '\'');
							std::getline(iss, skip, '\'');
							std::getline(iss, num, '\'');

							url = "http://www.onlinequizarea.com/checkanswer.php?formid=question_"
								+ id + "_" + num + "&retry=1";
							if(!get_http(url, xml))
								continue;
							if(xml.find(">Correct!<") != str::npos)
								correct = true;
						}
						else if((pos = line.find("questionradio\">")) != str::npos)
						{
							// questionradio">Carling<
							std::istringstream iss(line.substr(pos));
							std::getline(iss, skip, '>');
							std::getline(iss, answer, '<');

							ofs << (correct ? "*":"") << answer << '\n';
							correct = false;
						}
					}

//					std::ofstream ofs("dump.html");
//					while(std::getline(iss, line))
//						ofs << line << '\n';
//					break;
				}
		}

//		break;
	}

	return true;
}

int main()
{
	get_onlinequizarea_quiz();
}

