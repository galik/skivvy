#pragma once
#ifndef _SOOKEE_IRCBOT_OASTATS_H_
#define _SOOKEE_IRCBOT_OASTATS_H_
/*
 * ircbot-oastats.h
 *
 *  Created on: 07 Jul 2011
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

#include <skivvy/ircbot.h>

namespace skivvy { namespace ircbot {

// RPC Services
struct keystats
{
	str name;
	double kd;
	double cd;
	double pw;
	size_t to;
	double overall;
	siz rank;

	keystats()
	: name()
	, kd(0.0)
	, cd(0.0)
	, pw(0.0)
	, to(0)
	, overall(0.0)
	, rank(0)
	{
	}

	friend std::ostream& operator<<(std::ostream& os, const keystats& s)
	{
		os << s.kd;
		os << ' ' << s.cd;
		os << ' ' << s.pw;
		os << ' ' << s.to;
		os << ' ' << s.overall;
		os << ' ' << s.rank;
		os << ' ' << s.name;
		return os;
	}

	friend std::istream& operator>>(std::istream& is, keystats& s)
	{
		is >> s.kd;
		is >> s.cd;
		is >> s.pw;
		is >> s.to;
		is >> s.overall;
		is >> s.rank;
		std::getline(is >> std::ws, s.name);
		return is;
	}
};

typedef std::vector<keystats> stats_vector;
/**
 *
 */
class OAStatsIrcBotPlugin
: public BasicIrcBotPlugin
  , public IrcBotRPCService
{
private:

	str_set channels;
	std::mutex channels_mtx;

	RandomTimer timer;

	void regular_stats(const str& channel);
	void oastats_on(const message& msg);
	void oastats_off(const message& msg);

	// RPC clients
	bool rpc_lookup_players(const str& search, std::vector<str>& handles);
	bool rpc_match_player(bool subst, const str& name1, const str& name2);
	str rpc_html_handle_to_irc(str html);

	void oa1v1(const message& msg);
	void oastat(const message& msg);
	void oatop(const message& msg);

public:
	OAStatsIrcBotPlugin(IrcBot& bot);
	virtual ~OAStatsIrcBotPlugin();

	// RPC Services
	bool get_oatop(const str& params, stats_vector& v);

	// INTERFACE: BasicIrcBotPlugin

	virtual bool initialize();

	// INTERFACE: IrcBotPlugin

	virtual str get_name() const;
	virtual str get_version() const;
	virtual void exit();

	// INTERFACE RPC

	virtual bool rpc(rpc::call& c);
	virtual call_ptr create_call() const { return call_ptr(new rpc::local_call); }
};

}} // sookee::ircbot

#endif // _SOOKEE_IRCBOT_OASTATS_H_
