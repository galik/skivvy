#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <skivvy/props.h>
#include <skivvy/types.h>
#include <skivvy/logrep.h>

using namespace skivvy;
using namespace skivvy::types;
using namespace skivvy::props;
//using namespace skivvy::string;


int main()
{
	properties p;

	p.set("i1", 4);
	p.set("i2", 7);
	p.set("i3", 9);
	p.set("i4", 2);
	p.set("i5", 5);
	p.set("i6", 8);

	bug_var(p.get("i1"));
	bug_var(p.get("i2"));
	bug_var(p.get("i3"));
	bug_var(p.get("i4"));
	bug_var(p.get("i5"));
	bug_var(p.get("i6"));
}

