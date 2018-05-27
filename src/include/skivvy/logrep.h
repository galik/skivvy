#ifndef SKIVVY_LOGREP_H_
#define SKIVVY_LOGREP_H_
/*
 * logrep.h
 *
 *  Created on: 1 Aug 2011
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2011 SooKee oaskivvy@gmail.com               |
'------------------------------------------------------------------'

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

http://www.gnu.org/licenses/gpl-2.0.html

'-----------------------------------------------------------------*/

#include <hol/bug.h>

namespace skivvy { namespace utils {

#ifndef DEBUG
#define bug_msg(m)
#define BUG_COMMAND(m)
#else
#define QUOTE(s) #s
#define bug_msg(m) do{printmsg(std::cout, (m));}while(false)
#define BUG_COMMAND(m) \
bug_fun(); \
bug("//---------------------------------------------------"); \
bug_msg(msg); \
bug("//---------------------------------------------------")

#endif

}} // skivvy::utils

#endif // SKIVVY_LOGREP_H_
