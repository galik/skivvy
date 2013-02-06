/*
 * ircbot-trivial.cpp
 *
 *  Created on: 01 Jan 2012
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

#include <skivvy/plugin-trivial.h>

#include <ctime>
#include <future>
#include <thread>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>

#include <skivvy/stl.h>
#include <skivvy/str.h>
#include <skivvy/logrep.h>
#include <skivvy/network.h>

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(TrivialIrcBotPlugin);
PLUGIN_INFO("trivial", "Trivial Quiz Game", "0.1");

using namespace skivvy;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

std::istream& operator>>(std::istream& is, question& q)
{
	bug_func();
	q.q.clear();
	q.a.clear();
	q.i.clear();
	std::getline(is, q.q);
	bug("q: " << q.q);
	str line;
	while(std::getline(is, line) && !trim(line).empty())
	{
		bug("a: " << line);
		if(line[0] == '#')
			q.i = line;
		else
			q.a.push_back(line);
	}
	return is;
}

// http://www.onlinequizarea.com/multiple-choice/general-knowledge-quiz/start/added/32/1/
// http://www.onlinequizarea.com/multiple-choice/general-knowledge-quiz/start/added&next=1/32/1/
// http://www.onlinequizarea.com/multiple-choice/general-knowledge-quiz/start/added&next=30/32/1/
// http://www.onlinequizarea.com/multiple-choice/general-knowledge-quiz/start/added&next=60/32/1/
// ...
// http://www.onlinequizarea.com/multiple-choice/general-knowledge-quiz/start/added&next=300/32/1/

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

TrivialIrcBotPlugin::TrivialIrcBotPlugin(IrcBot& bot)
: BasicIrcBotPlugin(bot)
, current_question(questions.end())
, accepting_answers(false)
, answer_found(false)
{
}

TrivialIrcBotPlugin::~TrivialIrcBotPlugin() {}

void TrivialIrcBotPlugin::q_open(const message& msg)
{
	bug("-----------------------------------------------------");
	bug(get_name() << ": " << "q_open()");
	bug("-----------------------------------------------------");
	bug_msg(msg);

	get_onlinequizarea_quiz();

	questions.clear();
	std::ifstream ifs(bot.getf("trivial.quiz.file", "trivial-quiz.txt"));

	question q;
	while(ifs >> q)
		questions.push_back(q);

	stl::random_shuffle(questions);
	current_question = questions.begin();
}

void TrivialIrcBotPlugin::q_end(const message& msg)
{
	time_t secs = bot.get("trivial.quiz.answer.time", 20);
	std::this_thread::sleep_for(std::chrono::seconds(secs));
	accepting_answers = false;

	if(answer_found) return;

	bot.fc_reply(msg, "Time UP!!");

//	++current_question;
}

static std::future<void> fut;

void TrivialIrcBotPlugin::q_ask(const message& msg)
{
	bug("-----------------------------------------------------");
	bug(get_name() << ": " << "q_ask()");
	bug("-----------------------------------------------------");
	bug_msg(msg);

	if(accepting_answers)
	{
		bot.fc_reply(msg, "Question in progress.");
		return;
	}

	if(current_question == questions.end())
	{
		bot.fc_reply(msg, "First you must use !q_open to load the questions.");
		return;
	}

	bot.fc_reply(msg, current_question->q);

	answers.clear();
	size_t i = 0;
	for(const str& a: current_question->a)
	{
		if(!a.empty())
		{
			std::ostringstream oss;
			oss << "[" << ++i << "] " << (a[0] == '*' ? a.substr(1) : a);
			if(a[0] == '*')
				answers.push_back(i);
			bot.fc_reply(msg, oss.str());
		}
	}

	if(fut.valid())
		fut.get();
	answered.clear();
	answer_found = false;
	accepting_answers = true;
	fut = std::async(std::launch::async, [&]{ q_end(msg); });

	++current_question;
}

void TrivialIrcBotPlugin::q_close(const message& msg)
{
	bug("-----------------------------------------------------");
	bug(get_name() << ": " << "q_close()");
	bug("-----------------------------------------------------");
	bug_msg(msg);

	questions.clear();
	questions.resize(0);
}

// INTERFACE: BasicIrcBotPlugin

bool TrivialIrcBotPlugin::initialize()
{
	add
	({
		"!q_open"
		, "!q_open Open the quiz questions file."
		, [&](const message& msg){ q_open(msg); }
	});
	add
	({
		"!q_ask"
		, "!q_ask Ask a question from the open file."
		, [&](const message& msg){ q_ask(msg); }
	});
	add
	({
		"!q_close"
		, "!q_close Close the quiz questions file."
		, [&](const message& msg){ q_close(msg); }
	});
	bot.add_monitor(*this);
	return true;
}

// INTERFACE: IrcBotPlugin

std::string TrivialIrcBotPlugin::get_id() const { return ID; }
std::string TrivialIrcBotPlugin::get_name() const { return NAME; }
std::string TrivialIrcBotPlugin::get_version() const { return VERSION; }

void TrivialIrcBotPlugin::exit()
{
	if(fut.valid())
		if(fut.wait_for(std::chrono::seconds(10)) == std::future_status::ready)
			fut.get();

//	if(fut.valid())
//		fut.get();
}

// INTERFACE: IrcBotMonitor

void TrivialIrcBotPlugin::event(const message& msg)
{
	if(msg.cmd_cp == "PRIVMSG")
	{
//		bug("event() RECEIVED");
//		bug("accepting_answers" << accepting_answers);
//		bug("msg.text" << msg.text);
		siz ans;
		if(accepting_answers && std::istringstream(msg.text_cp) >> ans)
		{
			if(stl::find(answered, msg.get_nick_cp()) != answered.end())
			{
				bot.fc_reply(msg, msg.get_nick_cp() + ": You only get one try.");
			}
			else if(stl::find(answers, ans) != answers.end())
			{
				answer_found = true;
				accepting_answers = false;
				bug("event() ANSWER FOUND");
				++totals[msg.get_nick_cp()];
				std::ostringstream oss;
				oss << msg.get_nick_cp() << " has the right answer!";
				oss << " Total: " << totals[msg.get_nick_cp()] << " points.";
				bot.fc_reply(msg, oss.str());
//				++current_question;
			}
			answered.push_back(msg.get_nick_cp());
		}
	}
}

}} // sookee::ircbot
