/*
Project: SSBRenderer
File: minimal.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include <iostream>
#include <stdexcept>
#include "../SSBParser.hpp"

int main(){
	SSB::Data data;
	SSB::Parser parser;
	std::string line("");
	try{
		parser.parse_line(data, line);
		std::cout << "SUCCESS!!!" << std::endl;
	}catch(SSB::Exception e){
		throw std::logic_error(e.what());
	}
	return 0;
}
