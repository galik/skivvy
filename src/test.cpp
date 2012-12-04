#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <skivvy/stl.h>
#include <skivvy/str.h>
#include <skivvy/rcon.h>
#include <skivvy/types.h>
#include <skivvy/ircbot.h>
#include <skivvy/logrep.h>
#include <skivvy/network.h>
#include <skivvy/socketstream.h>

using namespace skivvy;
using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;
using namespace skivvy::ircbot;

template<typename Plugin>
class IrcBotPluginHandle
{
	typedef std::auto_ptr<Plugin> PluginPtr;
	IrcBot& bot;
	const str name;
	PluginPtr plugin;// = 0;
	time_t plugin_load_time = 0;

	static Plugin null_plugin;

public:
	IrcBotPluginHandle(IrcBot& bot, const str& name)
	: bot(bot), name(name)
	{
	}

	void ensure_plugin()
	{
		if(bot.get_plugin_load_time() > plugin_load_time)
		{
			IrcBotPluginPtr ptr = bot.get_plugin(name);
			plugin.reset(dynamic_cast<Plugin*>(ptr.get()));
			plugin_load_time = std::time(0);
		}
	}

	Plugin& operator*()
	{
		ensure_plugin();

		if(!plugin)
		{
			log("ERROR: Bad IrcBotPluginHandle: " << this);
			return *null_plugin;
		}
		return *plugin;
	}

	Plugin* operator->()
	{
		ensure_plugin();

		if(!plugin)
		{
			log("ERROR: Bad IrcBotPluginHandle: " << this);
			return null_plugin;
		}
		return plugin;
	}
};

int main()
{

}

