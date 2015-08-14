/*
Project: SSBRenderer
File: string.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../string.hpp"
#include <stdexcept>

int main(){
	int a, b, c, d;
	if(stdex::string_to_number(" -42 ", a) || !stdex::string_to_number("3,-87", a, b) || a != 3 || b != -87)
		throw std::logic_error("String to number conversion failed");
	if(stdex::hex_string_to_number("1g", a) || !stdex::hex_string_to_number("00,ff,a5,1E", a, b, c, d) || a != 0x00 || b != 0xff || c != 0xa5 || d != 0x1e)
		throw std::logic_error("Hexadecimal string to number conversion failed");
	if(stdex::find_non_escaped_character("{123}{abc", '{', 1) != 5)
		throw std::logic_error("Couldn't find correct escaped character");
        std::string s("Hello world! All correct, world?");
	if(stdex::string_replace(s, "world", "Spongebob") != "Hello Spongebob! All correct, Spongebob?")
		throw std::logic_error("String replacement didn't work");
	auto words = stdex::get_words(s);
	if(words[0].prespace != 0 || words[0].text != "Hello" ||
	words[1].prespace != 1 || words[1].text != "Spongebob!" ||
	words[2].prespace != 1 || words[2].text != "All" ||
	words[3].prespace != 1 || words[3].text != "correct," ||
	words[4].prespace != 1 || words[4].text != "Spongebob?")
		throw std::logic_error("Word extraction failed");
	std::istringstream ss(s);
	if(stdex::has_empty_last_line(ss, ' ') || !stdex::has_empty_last_line(ss, '?'))
		throw std::logic_error("Couldn't detect empty last line correctly");
	return 0;
}
