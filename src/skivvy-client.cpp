#include <iostream>
#include <thread>
#include <string>
#include <cerrno>
#include <cstring>

#include <skivvy/str.h>
#include <skivvy/logrep.h>
#include <skivvy/socketstream.h>

using namespace skivvy;
using namespace skivvy::utils;
using namespace skivvy::string;

int main()
{
	std::cout << "Skivvy Client:" << '\n';
	std::string line;
	std::cout << "> ";
	while(std::getline(std::cin, line))
	{
		if(!trim(line).empty())
		{
			net::socketstream ss;
			if(!ss.open("localhost", 7334))
			{
				std::cout << "error: " << std::strerror(errno);
			}
			else
			{
				(ss << line).put('\0') << std::flush;
				if(std::getline(ss, line, '\0'))
					std::cout << line << '\n';
			}
		}
		std::cout << "> ";
	}
}
