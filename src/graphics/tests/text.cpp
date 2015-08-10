/*
Project: SSBRenderer
File: text.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../gutils.hpp"
#include <stdexcept>
#include <iostream>

int main(){
	GUtils::Font font("Arial", 32, true, true, true);
	if(!font)
		throw std::logic_error("Font invalid");
	if(font.get_family().empty())
		throw std::logic_error("Couldn't pick any font family");
	if(font.get_size() != 32 || !font.get_bold() || !font.get_italic() || !font.get_underline() || font.get_strikeout() || font.get_spacing() != 0 || font.get_rtl())
		throw std::logic_error("Font construction values couldn't get retrieved");
	GUtils::Font::Metrics metrics = font.metrics();
	std::cout << "Height: " << metrics.height
		<< "\nAscent: " << metrics.ascent
		<< "\nDescent: " << metrics.descent
		<< "\nInternal leading: " << metrics.internal_leading
		<< "\nExternal leading: " << metrics.external_leading
		<< std::endl;
	std::string text = "Hello world です!";
	std::cout << "Text: " << text
		<< "\nText width: " << font.text_width(text)
		<<std::endl;
	if(font.text_path(text).empty())
		throw std::logic_error("Couldn't generate the text path");
	return 0;
}
