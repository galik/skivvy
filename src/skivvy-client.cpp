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

namespace this_thread {

class out
: public std::ostream
{
	std::mutex mtx;
	std::ostream& os;

public:
	out(std::ostream& os): os(os) {}

	template<typename Type>
	std::ostream& operator<<(const Type& type)
	{
		std::lock_guard<std::mutex> lock(mtx);
		return os << type;
	}
};

}

class SkivvyClient
{
	str config_dir;
	str host = "localhost";
	str port = "7334";

	this_thread::out os;

	bool send(const str& cmd, str& res, bool report = true)
	{
		net::socketstream ss;
		if(!ss.open(host, std::stoi(port)))
		{
			if(report)
				os << "error: " << std::strerror(errno) << std::endl;
			return false;
		}

		(ss << cmd).put('\0') << std::flush;
		return sgl(ss, res, '\0');
	}

	std::atomic<bool> done;
	std::atomic<bool> dusted;
	std::future<void> fut;

public:
	SkivvyClient(const str& config_dir, const str& host = "localhost", const str& port = "7334")
	: config_dir(config_dir), host(host), port(port), os(std::cout)
	, done(false), dusted(false)
	{
	}

	using str_vec_map = std::map<str, str_vec>;

	str_vec parse_status(str&& line)
	{
		str_vec status;
		siss iss(line);
		while(sgl(iss, line))
			status.push_back(std::move(line));
		return status;
	}

	void poll()
	{
		str line;
		while(!done)
		{
			if(send("/get_status", line))
				for(auto&& l: parse_status(std::move(line)))
					os << l << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
		dusted = true;
	}

	void run()
	{
		using_history();
		read_history((config_dir + "/.history").c_str());

		fut = std::async(std::launch::async, [this]{ poll(); });

		str name;
		str chan;

		str shell_prompt = "unknown: ";

		if(send("/botnick", name))
			shell_prompt = name + ": ";

		str line, prev;
		cstring_uptr input;

		input.reset(readline(shell_prompt.c_str()));

		while(input && trim(line = input.get()) != "exit")
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
					os << line << std::endl;

				if(send("/botnick", name))
					shell_prompt = "<" + name + ">: ";
			}
			input.reset(readline(shell_prompt.c_str()));
		}

		done = true;

		st_clk::time_point timeout = st_clk::now() + std::chrono::seconds(10);

		while(!dusted && timeout < st_clk::now())
			std::this_thread::sleep_for(std::chrono::seconds(1));

		if(!dusted)
			log("ERROR: client timed out waiting for response");

		if(fut.valid())
			fut.get();
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
