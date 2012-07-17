/*
 * ircbot-trivial.cpp
 *
 *  Created on: 01 Jan 2012
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

namespace skivvy { namespace ircbot {

IRC_BOT_PLUGIN(TrivialIrcBotPlugin);
PLUGIN_INFO("Trivial Quiz Game", "0.1");

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

std::string TrivialIrcBotPlugin::get_name() const { return NAME; }
std::string TrivialIrcBotPlugin::get_version() const { return VERSION; }

void TrivialIrcBotPlugin::exit()
{
	if(fut.valid()) fut.get();
//	bug_func();
}

// INTERFACE: IrcBotMonitor

void TrivialIrcBotPlugin::event(const message& msg)
{
	if(msg.cmd == "PRIVMSG")
	{
//		bug("event() RECEIVED");
//		bug("accepting_answers" << accepting_answers);
//		bug("msg.text" << msg.text);
		siz ans;
		if(accepting_answers && std::istringstream(msg.text) >> ans)
		{
			if(stl::find(answered, msg.get_sender()) != answered.end())
			{
				bot.fc_reply(msg, msg.get_sender() + ": You only get one try.");
			}
			else if(stl::find(answers, ans) != answers.end())
			{
				answer_found = true;
				accepting_answers = false;
				bug("event() ANSWER FOUND");
				++totals[msg.get_sender()];
				std::ostringstream oss;
				oss << msg.get_sender() << " has the right answer!";
				oss << " Total: " << totals[msg.get_sender()] << " points.";
				bot.fc_reply(msg, oss.str());
//				++current_question;
			}
			answered.push_back(msg.get_sender());
		}
	}
}

}} // sookee::ircbot
