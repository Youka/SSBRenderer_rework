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
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include "../SSBParser.hpp"

int main(int argc, char** argv){
	SSB::Data data;
	SSB::Parser parser;
	std::stringstream stream;
	for(int i=1; i < argc; ++i)
		stream << argv[i] << '\n';
	try{
		parser.parse_script(data, stream);
		std::cout << "#META\nTitle: " << data.meta.title << "\nDescription: " << data.meta.description << "\nAuthor: " << data.meta.author << "\nVersion: " << data.meta.version
				<< "\n\n#FRAME\nWidth: " << data.frame.width << "\nHeight: " << data.frame.height
				<< "\n\n#STYLES";
		std::for_each(data.styles.begin(), data.styles.end(), [](std::pair<std::string,std::string> style){std::cout << "\n" << style.first << ": " << style.second;});
		std::cout << "\n\n#EVENTS";
		std::for_each(data.events.begin(), data.events.end(), [](SSB::Event& event){std::cout << "\n" << event.start_ms << " - " << event.end_ms << " | " << event.objects.size() << " objects " << (event.static_tags ? "(static)" : "(dynamic)");});
	}catch(SSB::Exception e){
		throw std::logic_error(e.what());
	}
	return 0;
}
