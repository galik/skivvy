#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <bitset>

#include <skivvy/str.h>
#include <skivvy/logrep.h>

using namespace skivvy::types;
using namespace skivvy::utils;
using namespace skivvy::string;

struct cvar_t
{
	str name;
	str desc;
	str dflt;
	str type;

	bool operator<(const cvar_t& c) const { return name < c.name; }

	friend std::istream& operator>>(std::istream& is, cvar_t& cvar)
	{
		std::getline(is, cvar.name);
		std::getline(is, cvar.desc);
		std::getline(is, cvar.dflt);
		std::getline(is, cvar.type);
		return is;
	}

	friend std::ostream& operator<<(std::ostream& os, const cvar_t& cvar)
	{
		os << cvar.name << '\n';
		os << cvar.desc << '\n';
		os << cvar.dflt << '\n';
		os << cvar.type;
		return os;
	}
};

int main()
{
	str line;
	while(std::getline(std::cin, line))
	{
		if(line.find("\\002") == str::npos)
			continue;

		std::istringstream iss(line);
		std::getline(iss, line, '"');
		std::getline(iss, line, '"');
		if(!line.empty())
		{
			// \002cg_accX\002 | No Info \002Default Value:\002 450 \002Data Type:\002 integer
			cvar_t cvar;
			siz pos = extract_delimited_text(line, "\\002", "\\002", cvar.name, 0);
			pos = extract_delimited_text(line, "|", "\\002", cvar.desc, pos);
			pos = extract_delimited_text(line, "\\002", "\\002", cvar.dflt, pos);
			pos = line.find("\\002", pos);
			if(pos != str::npos)
			{
				cvar.type = line.substr(pos + 4);
				trim(cvar.name);
				trim(cvar.desc);
				trim(cvar.dflt);
				trim(cvar.type);

				std::cout << cvar << '\n';
			}
		}
	}

}

