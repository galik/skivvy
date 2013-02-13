/*
 * utils.h
 *
 *  Created on: 13 Feb 2013
 *      Author: oaskivvy@gmail.com
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oaskivvy@gmail.com                     |
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

#ifndef _SKIVVY_UTILS_H_
#define _SKIVVY_UTILS_H_

#include <skivvy/types.h>

namespace skivvy { namespace utils {

using namespace skivvy::types;

// "1-4", "7", "9-11 ", " 15 - 16" ... -> siz_vec{1,2,3,4,7,9,10,11,15,16}
bool parse_rangelist(const str& rangelist, siz_vec& items);

}} // skivvy::utils

#endif // _SKIVVY_UTILS_H_
