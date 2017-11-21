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

#include "ircbot.h"

namespace skivvy {
namespace ircbot {
namespace db {

using namespace header_only_library::small_types::basic;

class BotDbInterface
{
public:

	virtual ~BotDbInterface() {}

	virtual void set(str const& key, str const& val, siz idx = 0) = 0;

	virtual void add(str const& key, str const& val) = 0;

	virtual str get(str const& key, str const& dflt) = 0;

	virtual str_vec get_all(str const& key) = 0;

	template<typename Container>
	Container get_all_as(str const& key)
	{
		auto& v = get(key);
		return {v.begin(), v.end()};
	}

private:
	virtual str_vec& get(str const& key) = 0;
};

class BotDbImpl
: public BotDbInterface
{
	friend class BotDb;
	friend class Transaction;

public:

	void set(str const& key, str const& val, siz idx = 0) override;

	void add(str const& key, str const& val) override;

	str get(str const& key, str const& dflt) override;

	str_vec get_all(str const& key) override;

private:
	BotDbImpl(IrcBot& bot, str plugin_id);


	str_vec& get(str const& key) override;

	void load(str const& key);
	void save(str const& key);

	void load();
	void save();

	IrcBot& bot;
	str plugin_id;

	str_set changed_keys;

//	std::map<str, bool> changed;
	std::map<str, str_vec> data;
	mutable std::mutex mtx;
};

class Transaction
{
public:
	Transaction(BotDbImpl& db, std::mutex& mtx)
	: db(db), rollback(false), lock(mtx) { db.load(); }

	Transaction(Transaction&& tx)
	: db(tx.db), rollback(tx.rollback), lock(std::move(tx.lock)) {}

	~Transaction() { if(rollback) db.load(); else db.save(); }

	class BotDbInterface* operator->() { return &db; }

private:
	BotDbImpl& db;
	bool rollback = false;
	std::unique_lock<std::mutex> lock;
};

class BotDb
: public std::enable_shared_from_this<BotDb>
{
public:
	using SPtr = std::shared_ptr<BotDb>;

	BotDb(IrcBot& bot, str const& plugin_id)
	: db(bot, plugin_id) {}

	Transaction transaction();

	static SPtr instance(IrcBot& bot, str const& plugin_id);

private:
	BotDbImpl db;
};

static void test_func()
{
	IrcBot bot;
	auto db = BotDb::instance(bot, "");

	auto tx = db->transaction(); // commits end of scope

	tx->add("test_key", "test_data");
	// may tx->rollback(); here is so desired.

	// tx commits here
}

//class BotDbImpl
//{
////	friend class FileBasedBotDb;
//	friend class tx;
//
//public:
//	using SPtr = std::shared_ptr<BotDb>;
//
//	class tx
//	{
//	public:
//		tx(SPtr db, std::mutex& mtx)
//		: db(db), rollback(false), lock(mtx) {}
//		~tx() { if(rollback) db->load(); else db->save(); }
//
//		BotDb* operator->() { return db.get(); }
//
//	private:
//		SPtr db;
//		bool rollback = false;
//		std::lock_guard<std::mutex> lock;
//	};
//
//	auto start_transaction()
//	{
//		return tx(shared_from_this(), mtx);
//	}
//
//	void set(str const& key, str const& val, siz idx = 0) override
//	{
//		auto& v = get(key);
//		v.resize(idx + 1);
//		v[idx] = val;
//		save(key);
//	}
//
//	void add(str const& key, str const& val) override
//	{
//		get(key).push_back(val);
//		save(key);
//	}
//
//	str get(str const& key, str const& dflt) override
//	{
//		auto& v = get(key);
//		if(v.empty())
//			return dflt;
//		return v.front();
//	}
//
//	str_vec get_all(str const& key) override
//	{
//		return get(key);
//	}
//
//	static SPtr instance(IrcBot& bot, str const& plugin_id);
//
//private:
//	BotDbImpl(IrcBot& bot, str plugin_id);
//
//	BotDb(BotDb&&) = delete;
//	BotDb(BotDb const&) = delete;
//
//	BotDb& operator=(BotDb&&) = delete;
//	BotDb& operator=(BotDb const&) = delete;
//
//	str_vec& get(str const& key)
//	{
//		auto found = changed.find(key);
//		if(found == changed.end() || found->second)
//			load(key);
//		return data[key];
//	}
//
//	void load(str const& key)
//	{
//		str_vec().swap(data[key]);
//		if(auto ifs = std::ifstream(fs::path{"botdb"}/plugin_id/key, std::ios::binary))
//			for(str line; sgl(ifs, line);)
//				data[key].push_back(line);
//		changed[key] = false;
//	}
//
//	void save(str const& key)
//	{
//		if(auto ofs = std::ofstream(fs::path{"botdb"}/plugin_id/key, std::ios::binary))
//			for(str line: data[key])
//				ofs << line << '\n';
//		changed[key] = true;
//	}
//
//	IrcBot& bot;
//	str plugin_id;
//
//	std::map<str, bool> changed;
//	std::map<str, str_vec> data;
//
//	mutable std::mutex mtx;
//};

} // db
} // ircbot
} // skivvy

#endif // SKIVVY_BOTDB_H
