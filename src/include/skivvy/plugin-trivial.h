#pragma once
#ifndef _SOOKEE_IRCBOT_TRIVIAL_H_
#define _SOOKEE_IRCBOT_TRIVIAL_H_
/*
 * ircbot-trivial.h
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

#include <skivvy/types.h>
#include <skivvy/ircbot.h>
#include <skivvy/logrep.h>

#include <mutex>

namespace skivvy { namespace ircbot {

using namespace skivvy::types;

struct question
{
	str q;
	str_vec a;
	str i; // additional info
};

/**
 * PROPERTIES: (Accesed by: bot.props["property"])
 *
 * question.number = number of questions in quiz
 * answer.time = time allowed for answers
 *
 * quiz.file - location of question file
 * FORMAT: [*] = optional * indicates correct answer
 * Question 1
 * [*]Ans 1
 * [*]Ans 2
 * [*]Ans n
 *
 * Question 2
 *
 */
class TrivialIrcBotPlugin
: public BasicIrcBotPlugin
 , public IrcBotMonitor
{
public:
	typedef std::vector<question> q_vec;
	typedef q_vec::iterator q_vec_iter;
	typedef q_vec::const_iterator q_vec_citer;

private:
	q_vec questions;
	str_siz_map totals; // total answers in quiz

	q_vec_citer current_question;
	bool accepting_answers;
	bool answer_found;
	siz_vec answers;
	str_vec answered; // nicks who answered

	void q_open(const message& msg);
	void q_ask(const message& msg);
	void q_end(const message& msg);
	void q_close(const message& msg);

public:
	TrivialIrcBotPlugin(IrcBot& bot);
	virtual ~TrivialIrcBotPlugin();

	// INTERFACE: BasicIrcBotPlugin

	virtual bool initialize();

	// INTERFACE: IrcBotPlugin

	virtual std::string get_name() const;
	virtual std::string get_version() const;
	virtual void exit();

	// INTERFACE: IrcBotMonitor

	virtual void event(const message& msg);
};

}} // sookee::ircbot

#endif // _SOOKEE_IRCBOT_TRIVIAL_H_
