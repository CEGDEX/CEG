/*
	CEG is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	CEG is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with CEG.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UTILS_BASE64_H_
#define UTILS_BASE64_H_

#include "modp_b64.h"

namespace utils {
	/*
		Encodes the input string in base64.  
		Returns true if successful and false otherwise.  
		The output string is only modified if successful.
	*/
	bool Base64Encode(const std::string &input, std::string &output);

	/* 
		Decodes the base64 input string.  
		Returns true if successful and false otherwise.  
		The output string is only modified if successful.
	 */
	bool Base64Decode(const std::string &input, std::string &output);
}

#endif
