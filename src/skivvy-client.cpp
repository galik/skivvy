#include <iostream>
#include <thread>
#include <string>
#include <cerrno>
#include <cstring>

#include <sookee/str.h>

#include <sookee/types.h>
#include <skivvy/socketstream.h>

#include <readline/readline.h>
#include <readline/history.h>

using namespace skivvy;
using namespace sookee::types;
using namespace sookee::string;

bool send(const str& cmd, str& res)
{
	net::socketstream ss;
	if(!ss.open("localhost", 7334))
	{
		std::cout << "error: " << std::strerror(errno);
		return false;
	}
	else
	{
		(ss << cmd).put('\0') << std::flush;
		return sgl(ss, res, '\0');
	}
	return false;
}

int main()
{
	std::cout << "Skivvy Client:" << '\n';

	using_history();

	read_history((str(getenv("HOME")) + "/.skivvy/.history").c_str());

	str name;

	str shell_prompt = "unknown: ";

	if(!send("/botnick", name))
		shell_prompt = name + ": ";

	str line;
	cstring_uptr input;

	input.reset(readline(shell_prompt.c_str()));

	while(input.get() && trim(line = input.get()) != "exit")
	{
		if(!line.empty())
		{
			add_history(line.c_str());
			write_history((str(getenv("HOME")) + "/.skivvy/.history").c_str());

			if(send(line, line) && !trim(line).empty())
				std::cout << line << '\n';

			if(send("/botnick", name))
				shell_prompt = name + ": ";
		}
		input.reset(readline(shell_prompt.c_str()));
	}
}
