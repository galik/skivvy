#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <skivvy/props.h>
#include <sookee/types.h>
#include <skivvy/logrep.h>

using namespace skivvy;
using namespace sookee::types;
using namespace skivvy::props;
//using namespace skivvy::string;


int main()
{
	properties p;

	transaction tx = p.get_transaction();

	tx.set("i1", 4);
	tx.set("i2", 7);
	tx.set("i3", 9);
	tx.set("i4", 2);
	tx.set("i5", 5);
	tx.set("i6", 8);

	bug_var(tx.get("i1"));
	bug_var(tx.get("i2"));
	bug_var(tx.get("i3"));
	bug_var(tx.get("i4"));
	bug_var(tx.get("i5"));
	bug_var(tx.get("i6"));
}

