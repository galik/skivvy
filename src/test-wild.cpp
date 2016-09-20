#include <string>
#include <vector>
#include <iostream>

using str = std::string;
using str_iter = str::iterator;
using str_citer = str::const_iterator;
using str_str_pair = std::pair<str, str>;

bool wildmatch(str_citer mi, const str_citer me
	, str_citer ii, const str_citer ie)
{
	while(mi != me)
	{
		if(*mi == '*')
		{
			for(str_citer i = ii; i != ie; ++i)
				if(wildmatch(mi + 1, me, i, ie))
					return true;
			return false;
		}
		if(*mi != *ii) return false;
		++mi;
		++ii;
	}
	return ii == ie;
}

bool wildmatch(const str& mask, const str& id)
{
	return wildmatch(mask.begin(), mask.end(), id.begin(), id.end());
}

struct tset
{
	str match;
	str data;
	bool ans;
};

static const std::vector<tset> test =
{
	{"*!*AIM|MEGA@*", "A|I|M!~AIM|MEGA@pool-74-102-184-218.nwrknj.east.verizon.net", true}
	,{"wi*b", "wobble", false}
	,{"wi*b", "wib", true}
	,{"wi*b", "winob", true}
	,{"wi*b", "wibbbb", true}
	,{"wi*b", "widble", false}
};

str fit_to_width(const str& s, size_t w, char align = 'L', char fill = ' ')
{
	str fmt;

	if(s.size() > w)
	{
		size_t diff = s.size() - w;
		size_t liff = diff / 2;
		size_t riff = diff - liff;
		if(align == 'L') fmt = s.substr(0, w);
		else if(align == 'R') fmt = s.substr(diff);
		else fmt = s.substr(liff, riff);
	}
	else
	{
		size_t diff = w - s.size();
		size_t liff = diff / 2;
		size_t riff = diff - liff;
		if(align == 'L') fmt = s + str(diff, fill);
		else if(align == 'R') fmt = str(diff, fill) + s;
		else fmt = str(liff, fill) + s + str(riff, fill);
	}

	return fmt;
}


int main()
{

//	std::string s = "the cat sat on the mat";
//
//	std::cout << "L: " << fit_to_width(s, 8, 'L', '_') << '\n';
//	std::cout << "R: " << fit_to_width(s, 8, 'R', '_') << '\n';
//	std::cout << "C: " << fit_to_width(s, 8, 'C', '_') << '\n';
//
//	std::cout << "L: " << fit_to_width(s, 7, 'L', '_') << '\n';
//	std::cout << "R: " << fit_to_width(s, 7, 'R', '_') << '\n';
//	std::cout << "C: " << fit_to_width(s, 7, 'C', '_') << '\n';
//
//	std::cout << "L: " << fit_to_width(s, 26, 'L', '_') << '\n';
//	std::cout << "R: " << fit_to_width(s, 26, 'R', '_') << '\n';
//	std::cout << "C: " << fit_to_width(s, 26, 'C', '_') << '\n';
//
//	std::cout << "L: " << fit_to_width(s, 25, 'L', '_') << '\n';
//	std::cout << "R: " << fit_to_width(s, 25, 'R', '_') << '\n';
//	std::cout << "C: " << fit_to_width(s, 25, 'C', '_') << '\n';

	std::cout  << std::boolalpha;
	for(const tset& p: test)
	{
		std::cout << "m: " << p.match << '\n';
		std::cout << "d: " << p.data << '\n';
		std::cout << "a: " << p.ans << '\n';
		std::cout << "r: " << wildmatch(p.match, p.data) << '\n';
		std::cout << std::endl;
	}

	return 0;
}
