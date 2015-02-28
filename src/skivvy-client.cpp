#include <iostream>
#include <thread>
#include <string>
#include <cerrno>
#include <cstring>

#include <sookee/str.h>
#include <sookee/ios.h>
#include <sookee/cfg.h>

#include <sookee/types.h>
#include <skivvy/socketstream.h>

#include <readline/readline.h>
#include <readline/history.h>

using namespace skivvy;
using namespace sookee::ios;
using namespace sookee::types;
using namespace sookee::utils;
using namespace sookee::props;

class SkivvyClient
{
	str config_dir;
	str host = "localhost";
	str port = "7334";

	bool send(const str& cmd, str& res)
	{
		net::socketstream ss;
		if(!ss.open(host, std::stoi(port)))
		{
			std::cout << "error: " << std::strerror(errno);
			return false;
		}

		(ss << cmd).put('\0') << std::flush;
		return sgl(ss, res, '\0');
	}

public:
	SkivvyClient(const str& config_dir, const str& host = "localhost", const str& port = "7334")
	: config_dir(config_dir), host(host), port(port)
	{
	}

	void run()
	{
		using_history();
		read_history((config_dir + "/.history").c_str());

		str name;

		str shell_prompt = "unknown: ";

		if(send("/botnick", name))
			shell_prompt = name + ": ";

		str line, prev;
		cstring_uptr input;

		input.reset(readline(shell_prompt.c_str()));

		while(input.get() && trim(line = input.get()) != "exit")
		{
			if(!line.empty())
			{
				if(line != prev)
				{
					add_history(line.c_str());
					write_history((config_dir + "/.history").c_str());
					prev = line;
				}

				if(send(line, line) && !trim(line).empty())
					std::cout << line << '\n';

				if(send("/botnick", name))
					shell_prompt = name + ": ";
			}
			input.reset(readline(shell_prompt.c_str()));
		}
	}
};

int main(int argc, char* argv[])
{
	std::cout << "Skivvy Client:" << '\n';

	const str_vec args(argv + 1, argv + argc);

	str port = "7334";
	str host = "localhost";
	str config_dir = "$HOME/.skivvy";

	for(auto arg = args.begin(); arg != args.end(); ++arg)
	{
		if(*arg == "--config")
		{
			if(++arg == args.end())
			{
				con("ERROR: --config requires argument:");
				return 1;
			}
			config_dir = *arg;
		}
		if(*arg == "--host")
		{
			if(++arg == args.end())
			{
				con("ERROR: --host requires argument:");
				return 1;
			}
			host = *arg;
		}
		if(*arg == "--port")
		{
			if(++arg == args.end())
			{
				con("ERROR: --port requires argument:");
				return 1;
			}
			port = *arg;
		}
	}

	config_dir = wordexp(config_dir);

	SkivvyClient client(config_dir, host, port);

	client.run();
}
