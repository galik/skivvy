#pragma once
#ifndef _SKIVVY_IRCBOTMONITOR_H_
#define _SKIVVY_IRCBOTMONITOR_H_

/*
 * IrcBotMonitor.h
 *
 *  Created on: 06 Mar 2013
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oaskivvy@gmail.com                     |
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

#include <skivvy/message.h>

namespace skivvy { namespace ircbot {

/**
 * IrcBotMonitor is an interface class for plugins
 * to implement if they want to receive all channel
 * messages, not just the ones triggered by their
 * commands.
 *
 * Plugins interested in receiving these events should
 * implement this interface and register themselves with
 * the IrcBot using IrcBot::add_monitor(IrcBotMonitor& mon).
 *
 * This should ideally be done in the bots implementation
 * of void BasicIrcBotPlugin::initialize().
 */
class IrcBotMonitor
{
public:
	virtual ~IrcBotMonitor() {}

	/**
	 * This function will be called by the IrcBot on
	 * each message it receives from the RemoteIrcServer.
	 * @deprecated
	 */
	virtual void event(const message& msg) = 0;
//	virtual void send_event(const str& msg) = 0;
//	virtual void recv_event(const message& msg) = 0;
};

}} // skivvy::ircbot

#endif // _SKIVVY_IRCBOTMONITOR_H_
