#pragma once
#ifndef _SOOKEE_IRCBOT_PFINDER_H_
#define _SOOKEE_IRCBOT_PFINDER_H_
/*
 * ircbot-pfinder.h
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

/**
 *
 */
class PFinderIrcBotPlugin
: public BasicIrcBotPlugin
, public IrcBotRPCService
{
private:

	struct player
	{
		str ping;
		str frags;
		str handle;
	};

	struct server
	{
		str address;
		str players;
		str map;
		str gametype;
		str name;

		bool operator<(const server& s) const { return name < s.name; }
	};

	// RPC Services

	/**
	 * The supplied parameter may resolve into a list of handles
	 * or just one
	 *
	 * @return true if a substitution took place or false if the original
	 * search term was returned in the vector.
	 **/
	bool lookup_players(const str& search, std::vector<str>& handles);
	std::vector<str> oafind(const str handle);
	void check_tell();

	/**
	 * perform exact matching on substitutions
	 * and case insensitive substring matching otherwise
	 **/
	bool match_player(bool substitution, const str& name1, const str& name2);
	str html_handle_to_irc(str html) const;

	// utilities

	bool server_uidfile_is_old() const;
	void write_server_uidfile(std::vector<server>& servers) const;
	str::size_type extract_server(const str& line, server& s, str::size_type pos = 0) const;
	str::size_type extract_player(const str& line, player& p, str::size_type pos = 0) const;

	// Bot Commands

//	void oarcon(const message& msg);
//	void oarconmsg(const message& msg);

	void oacvar(const message& msg);
	void oafind(const message& msg);
	void oalink(const message& msg);
	void oaunlink(const message& msg);
	void oalist(const message& msg);
	void oatell(const message& msg);
	void oaserver(const message& msg);

public:
	PFinderIrcBotPlugin(IrcBot& bot);
	virtual ~PFinderIrcBotPlugin();

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

#endif // _SOOKEE_IRCBOT_PFINDER_H_
