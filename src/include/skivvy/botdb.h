#ifndef SKIVVY_BOTDB_H
#define SKIVVY_BOTDB_H

/*-----------------------------------------------------------------.
| Copyright (C) 2016 Galik galik.bool@gmail.com                    |
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

/**
 * Database abstraction for plugins to store general, channel or
 * group specific information.
 */

#include <memory>
#include <hol/small_types.h>

namespace skivvy {
namespace ircbot {
namespace db {

using namespace hol::small_types::basic;

class BotDb
: public std::enable_shared_from_this<BotDb>
{
	friend class FileBasedBotDb;

public:
	using SPtr = std::shared_ptr<BotDb>;

	virtual auto get(str const& key)->str = 0;
	virtual auto set(str const& key, str const& val)->void = 0;

	virtual ~BotDb() {}
	static SPtr instance(str const& plugin_id);

protected:
	template <typename Derived>
	std::shared_ptr<Derived> shared_from_base()
	{
		return std::static_pointer_cast<Derived>(shared_from_this());
	}

private:
	BotDb(str plugin_id): plugin_id(plugin_id) {}

	BotDb(BotDb&&) = delete;
	BotDb(BotDb const&) = delete;

	BotDb& operator=(BotDb&&) = delete;
	BotDb& operator=(BotDb const&) = delete;

	str plugin_id;
};

class FileBasedBotDb
: public BotDb
{
	friend class BotDb;

public:
	auto get(str const& key)-> str override
	{
		(void) key;
		return {};
	}

	auto set(str const& key, str const& val)-> void override
	{
		(void) key;
		(void) val;
	}

private:
	FileBasedBotDb(str const& plugin_id)
	: BotDb(plugin_id) {}
};

BotDb::SPtr BotDb::instance(str const& plugin_id)
{
	static std::mutex mtx;
	static std::map<str, BotDb::SPtr> dbs;

	std::lock_guard<std::mutex> lock(mtx);

	if(!dbs[plugin_id])
		dbs[plugin_id].reset(new FileBasedBotDb(plugin_id));

	return dbs[plugin_id]->shared_from_base<FileBasedBotDb>();
}

} // db
} // ircbot
} // skivvy

#endif // SKIVVY_BOTDB_H
