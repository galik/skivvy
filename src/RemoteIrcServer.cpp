/*
 * ircbot.cpp
 *
 *  Created on: 29 Jul 2011
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

#include <skivvy/RemoteIrcServer.h>
#include <skivvy/irc-constants.h>
#include <skivvy/str.h>

namespace skivvy { namespace ircbot {

using namespace skivvy;
using namespace skivvy::irc;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

// BASE

bool BaseIrcServer::send(const str& cmd)
{
	log("send: " << cmd);
	return send_unlogged(cmd);
}

bool BaseIrcServer::pass(const str& pwd)
{
	if(pwd.empty())
		return send(PASS + " none");
	return send(PASS + " " + pwd);
}

bool BaseIrcServer::nick(const str& n)
{
	return send(NICK + " " + n);
}

bool BaseIrcServer::user(const str& u, const str m, const str r)
{
	return send(USER + " " + u + " " + m + " * :" + r);
}

bool BaseIrcServer::join(const str& channel, const str& key)
{
	return send(JOIN + " " + channel + " " + key);
}

bool BaseIrcServer::part(const str& channel, const str& message)
{
	return send(PART + " " + channel + " " + message);
}

bool BaseIrcServer::ping(const str& info)
{
	return send_unlogged(PING + " " + info);
}

bool BaseIrcServer::pong(const str& info)
{
	return send_unlogged(PONG + " " + info);
}

bool BaseIrcServer::say(const str& to, const str& text)
{
	return send(PRIVMSG + " " + to + " :" + text);
}

bool BaseIrcServer::whois(const str& n)
{
	return send(WHOIS + " " + n);
}

bool BaseIrcServer::quit(const str& reason)
{
	return send(QUIT + " :" + reason);
}

// :SooKee!~SooKee@SooKee.users.quakenet.org KICK #skivvy Skivvy :Skivvy

// Parameters: <channel> *( "," <channel> ) <user> *( "," <user> ) [<comment>]
// KICK KICK #skivvy Skivvy :
bool BaseIrcServer::kick(const str_vec& chans, const str_vec&users, const str& comment)
{
	str cmd = KICK;

	str sep = " ";
	for(const str& s: chans)
		{ cmd += sep + s; sep = ","; }

	sep = " ";
	for(const str& s: users)
		{ cmd += sep + s; sep = ","; }

	return send(cmd + " :" + comment);
}

// non standard??
bool BaseIrcServer::auth(const str& user, const str& pass)
{
	return send(AUTH + " " + user + " " + pass);
}

bool BaseIrcServer::me(const str& to, const str& text)
{
	return say(to, "\001ACTION " + text + "\001");
}

bool BaseIrcServer::query(const str& nick)
{
	return send(QUERY + " " + nick);
}

// MODE #skivvy-admin +o Zim-blackberry

bool BaseIrcServer::mode(const str& chan, const str& mode, const str& nick)
{
	return send(MODE + " " + chan + " " + mode + " " + nick);
}

// correct???
bool BaseIrcServer::mode(const str& nick, const str& mode)
{
	return send(MODE + " " + nick + " " + mode);
}

bool BaseIrcServer::reply(const message& msg, const str& text)
{
	str from;
	if(!msg.prefix.empty())
		from = msg.prefix.substr(0, msg.prefix.find("!"));

	if(!msg.get_to().empty() && msg.get_to()[0] == '#')
		return say(msg.get_to(), text);
	else
		return say(from, text);

	return false;
}

bool BaseIrcServer::reply_pm(const message& msg, const str& text)
{
	return say(msg.get_nick(), text);
}

// REMOTE

bool RemoteIrcServer::send_unlogged(const str& cmd)
{
	lock_guard lock(mtx_ss);
	ss << cmd.substr(0, 510) << "\r\n" << std::flush;
	if(!ss)
		log("ERROR: send failed.");
	return ss;
}

bool RemoteIrcServer::connect(const str& host, long port)
{
	ss.clear();
	ss.open(host, port);
	return ss;
}

bool RemoteIrcServer::receive(str& line)
{
	return std::getline(ss, line);
}

// TEST

bool TestIrcServer::send_unlogged(const str& cmd)
{
	ofs << cmd.substr(0, 510) << "\r\n" << std::flush;
	if(!ofs)
		log("ERROR: send failed.");
	return ofs;
}

bool TestIrcServer::connect(const str& host, long port)
{
//	str ifile = "test/" + host + "-" + std::to_string(port) + "-in.txt";
//	str ofile = "test/" + host + "-" + std::to_string(port) + "-out.txt";

	str ifile = host + "-" + (port<10?"0":"") + std::to_string(port) + "-in.txt";
	str ofile = host + "-" + (port<10?"0":"") + std::to_string(port) + "-out.txt";

	bug_var(ifile);
	bug_var(ofile);

	ifs.clear();
	ifs.open(ifile);
	bug_var(ifs);
	ofs.clear();
	ofs.open(ofile);
	bug_var(ofs);
	return ifs && ofs;
}

bool TestIrcServer::receive(str& line)
{
	static siz no = 0;
	++no;
//	bug_var(no);
	skivvy::utils::botbug() << no << ": ";
	std::getline(ifs, line);
	return ifs;
}

}} // skivvy::ircbot
