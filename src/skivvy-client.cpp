#include <iostream>
#include <thread>
#include <string>
#include <cerrno>
#include <cstring>

#include <skivvy/str.h>
#include <skivvy/logrep.h>
#include <skivvy/socketstream.h>

#include <readline/readline.h>
#include <readline/history.h>

using namespace skivvy;
using namespace skivvy::utils;
using namespace skivvy::string;

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

//	snprintf(shell_prompt, sizeof(shell_prompt), "%s:%s $ ", getenv("USER"), getcwd(NULL, 1024));

	str name;

//	net::socketstream ss;
//	if(!ss.open("localhost", 7334))
//	{
//		std::cout << "error: " << std::strerror(errno);
//		exit(1);
//	}
//	else
//	{
//		(ss << "/botnick").put('\0') << std::flush;
//		if(!std::getline(ss, name, '\0'))
//			name = "Skivvy";
//	}

	str shell_prompt = "unknown: ";

	if(!send("/botnick", name))
		shell_prompt = name + ": ";

	str line;
	cstring_uptr input;

	input.reset(readline(shell_prompt.c_str()));

	while(input.get())
	{
		line = input.get();
		if(!trim(line).empty())
		{
			add_history(line.c_str());
			if(send(line, line))
				std::cout << line << '\n';
			if(send("/botnick", name))
				shell_prompt = name + ": ";
		}
		input.reset(readline(shell_prompt.c_str()));
	}

//
//	std::string line;
//	std::cout << "> ";
//	while(std::getline(std::cin, line))
//	{
//		if(!trim(line).empty())
//		{
//			net::socketstream ss;
//			if(!ss.open("localhost", 7334))
//			{
//				std::cout << "error: " << std::strerror(errno);
//			}
//			else
//			{
//				(ss << line).put('\0') << std::flush;
//				if(std::getline(ss, line, '\0'))
//					std::cout << line << '\n';
//			}
//		}
//		std::cout << "> ";
//	}
}
