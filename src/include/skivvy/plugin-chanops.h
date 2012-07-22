#pragma once
#ifndef _SOOKEE_IRCBOT_CHANOPS_H_
#define _SOOKEE_IRCBOT_CHANOPS_H_
/*
 * ircbot-chanops.h
 *
 *  Created on: 02 Aug 2011
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

#include <skivvy/ircbot.h>

#include <bitset>
#include <mutex>

namespace skivvy { namespace ircbot {

#define CHANOPS_GROUP(name) \
static const str G_##name = #name

CHANOPS_GROUP(NONE);
CHANOPS_GROUP(USER);
CHANOPS_GROUP(OPER);
CHANOPS_GROUP(SUPR);
CHANOPS_GROUP(ROOT);

class ChanopsIrcBotPlugin _final_
: public BasicIrcBotPlugin
, public IrcBotMonitor
{
public:

	/*
	 * User database record
	 */
	struct user_r
	{
		str user; // the user by which we log in as
		uint32_t sum; // password sum
		str_set groups;

		friend std::istream& operator>>(std::istream& is, user_r& ur);
		friend std::ostream& operator<<(std::ostream& os, const user_r& ur);
	};


	/**
	 * User object
	 */
	struct user_t
	{
		str prefix; // <nick>!<ircuser>@<host>
		str user; // the user by which we logged in as
		str nick; // current nick
		str_set groups;

//		user_t(const user_t& u): prefix(u.prefix) {}
		user_t(const message& msg, const user_r& ur)
		: prefix(msg.from), user(ur.user), nick(msg.get_sender()), groups(ur.groups) {}

		bool operator<(const user_t& u) const { return prefix < u.prefix; }
		bool operator==(const user_t& u) const { return prefix == u.prefix; }
	};

	typedef std::set<user_t> user_set;
	typedef user_set::iterator user_iter;
	typedef user_set::const_iterator user_citer;

private:

	std::mutex users_mtx;
	user_set users;
	std::mutex bans_mtx;
	str_set bans;

//	bool extract_params(const message& msg, std::initializer_list<str*> args);

	/**
	 * Verify of the user sending the message has
	 * the various permissions.
	 */
	bool permit(const message& msg);

	bool signup(const message& msg);

	bool login(const message& msg);
	void apply_acts(const str& id);
	void apply_acts(const user_t& u);

	/**
	 * List users
	 */
	bool list_users(const message& msg);
	bool ban(const message& msg);
	bool join_event(const message& msg);

//	RandomTimer rt;

	bool reclaim(const message& msg);

	// Assign each function to a group
	str_map perms; // msg.get_user_cmd() -> G_GROUP

//	void op(std::string& nick);
//	void voice(std::string& nick);
//	void kick(std::string& nick);
//	void ban(std::string& nick);

public:
	ChanopsIrcBotPlugin(IrcBot& bot);
	virtual ~ChanopsIrcBotPlugin();

	// INTERFACE: BasicIrcBotPlugin

	virtual bool initialize() _override_;

	// INTERFACE: IrcBotPlugin

	virtual std::string get_name() const _override_;
	virtual std::string get_version() const _override_;
	virtual void exit() _override_;

	// INTERFACE: IrcBotMonitor

	virtual void event(const message& msg) _override_;
};

}} // sookee::ircbot

#endif // _SOOKEE_IRCBOT_CHANOPS_H_