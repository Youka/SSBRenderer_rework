/*
Project: SSBRenderer
File: utf8.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../utf8.hpp"
#include <iostream>
#include <stdexcept>

int main(){
	std::string s("こんにちは!");
	auto chars = Utf8::chars(s);
	for(auto c : chars)
		std::cout << c << std::endl;
#ifdef _WIN32
	if(s != Utf8::from_utf16(Utf8::to_utf16(s)))
		throw std::logic_error("Conversion back&forth failed");
#endif
	return 0;
}
