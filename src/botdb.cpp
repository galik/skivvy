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

#include <memory>
#include <hol/small_types.h>

#include "include/skivvy/botdb.h"

namespace skivvy {
namespace ircbot {
namespace db {

using namespace header_only_library::small_types::basic;

BotDbImpl::BotDbImpl(IrcBot& bot, str plugin_id)
: bot(bot), plugin_id(plugin_id)
{
	auto dir = fs::path{"botdb"}/plugin_id;
	fs::create_directories(dir);
}

void BotDbImpl::set(str const& key, str const& val, siz idx)
{
	auto& v = get(key);
	v.resize(idx + 1);
	v[idx] = val;
	changed_keys.insert(key);
}

void BotDbImpl::add(str const& key, str const& val)
{
	get(key).push_back(val);
	changed_keys.insert(key);
}

str BotDbImpl::get(str const& key, str const& dflt)
{
	auto& v = get(key);
	if(v.empty())
		return dflt;
	return v.front();
}

str_vec BotDbImpl::get_all(str const& key)
{
	return get(key);
}

str_vec& BotDbImpl::get(str const& key)
{
	return data[key];
}

void BotDbImpl::load()
{
	for(auto const& key: changed_keys)
		load(key);
	changed_keys.clear();
}

void BotDbImpl::save()
{
	for(auto const& key: changed_keys)
		save(key);
	changed_keys.clear();
}

void BotDbImpl::load(str const& key)
{
	str_vec().swap(data[key]);
	if(auto ifs = std::ifstream(fs::path{"botdb"}/plugin_id/key, std::ios::binary))
		for(str line; sgl(ifs, line);)
			data[key].push_back(line);
}

void BotDbImpl::save(str const& key)
{
	if(auto ofs = std::ofstream(fs::path{"botdb"}/plugin_id/key, std::ios::binary))
		for(str line: data[key])
			ofs << line << '\n';
}

Transaction BotDb::transaction()
{
	return {db, db.mtx};
}

BotDb::SPtr BotDb::instance(IrcBot& bot, str const& plugin_id)
{
	static std::mutex mtx;
	static std::map<str, BotDb::SPtr> dbs;

	std::lock_guard<std::mutex> lock(mtx);

	if(!dbs[plugin_id])
		dbs[plugin_id].reset(new BotDb(bot, plugin_id));

	return dbs[plugin_id]->shared_from_this();
}

//class FileBasedBotDb
//: public BotDb
//{
//	friend class BotDb;
//
//public:
//	auto get(str const& key)-> str override
//	{
//		auto found = changed.find(key);
//		if(found == changed.end || found->second)
//			load(key);
//		return data[key].empty() ? "" : data[key].front();
//	}
//
//	auto set(str const& key, str const& val)-> void override
//	{
//		if(data[key].empty())
//			data[key].push_back(val);
//		else
//			data[key].front() = val;
//		save(key);
//	}
//
//private:
//	FileBasedBotDb(IrcBot& bot, str const& plugin_id)
//	: BotDb(bot, plugin_id)
//	{
//		auto dir = fs::path{"botdb"}/plugin_id;
//		fs::create_directories(dir);
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
//	std::map<str, bool> changed;
//	std::map<str, str_vec> data;
//};

//BotDb::SPtr BotDb::instance(IrcBot& bot, str const& plugin_id)
//{
//	static std::mutex mtx;
//	static std::map<str, BotDb::SPtr> dbs;
//
//	std::lock_guard<std::mutex> lock(mtx);
//
//	if(!dbs[plugin_id])
//		dbs[plugin_id].reset(new FileBasedBotDb(bot, plugin_id));
//
//	return dbs[plugin_id]->shared_from_base<FileBasedBotDb>();
//}

} // db
} // ircbot
} // skivvy
