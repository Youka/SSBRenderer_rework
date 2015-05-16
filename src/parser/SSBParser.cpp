/*
Project: SSBRenderer
File: SSBParser.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "SSBParser.hpp"
#include "config.h"
#include <sstream>

// Converts string to number
template<typename T>
inline bool string_to_number(std::string src, T& dst){
	std::istringstream s(src);
	return !(s >> std::noskipws >> dst) || !s.eof() ? false : true;
}

// Parses SSB time and converts to milliseconds
template<typename T>
inline bool parse_time(std::string& s, T& t){
	// Check for empty timestamp
	if(s.empty())
		return false;
	// Time sections
	enum class TimeUnit{MS, MS_10, MS_100, MS_LIMIT, SEC, SEC_10, SEC_LIMIT, MIN, MIN_10, MIN_LIMIT, H, H_10, END} unit = TimeUnit::MS;
	// Iterate through timestamp characters
	for(auto it = s.crbegin(); it != s.crend(); ++it)
		switch(unit){
			case TimeUnit::MS:
				if(*it >= '0' && *it <= '9'){
					t = *it - '0';
					unit = TimeUnit::MS_10;
				}else
					return false;
				break;
			case TimeUnit::MS_10:
				if(*it >= '0' && *it <= '9'){
					t += (*it - '0') * 10;
					unit = TimeUnit::MS_100;
				}else if(*it == '.')
					unit = TimeUnit::SEC;
				else
					return false;
				break;
			case TimeUnit::MS_100:
				if(*it >= '0' && *it <= '9'){
					t += (*it - '0') * 100;
					unit = TimeUnit::MS_LIMIT;
				}else if(*it == '.')
					unit = TimeUnit::SEC;
				else
					return false;
				break;
			case TimeUnit::MS_LIMIT:
				if(*it == '.')
					unit = TimeUnit::SEC;
				else
					return false;
				break;
			case TimeUnit::SEC:
                                if(*it >= '0' && *it <= '9'){
					t += (*it - '0') * 1000;
					unit = TimeUnit::SEC_10;
                                }else
					return false;
				break;
			case TimeUnit::SEC_10:
				if(*it >= '0' && *it <= '5'){
					t += (*it - '0') * 10 * 1000;
					unit = TimeUnit::SEC_LIMIT;
				}else if(*it == ':')
					unit = TimeUnit::MIN;
				else
					return false;
				break;
			case TimeUnit::SEC_LIMIT:
				if(*it == ':')
					unit = TimeUnit::MIN;
				else
					return false;
				break;
			case TimeUnit::MIN:
				if(*it >= '0' && *it <= '9'){
					t += (*it - '0') * 60 * 1000;
					unit = TimeUnit::MIN_10;
				}else
					return false;
				break;
			case TimeUnit::MIN_10:
				if(*it >= '0' && *it <= '5'){
					t += (*it - '0') * 10 * 60 * 1000;
					unit = TimeUnit::MIN_LIMIT;
				}else if(*it == ':')
					unit = TimeUnit::H;
				else
					return false;
				break;
			case TimeUnit::MIN_LIMIT:
				if(*it == ':')
					unit = TimeUnit::H;
				else
					return false;
				break;
			case TimeUnit::H:
				if(*it >= '0' && *it <= '9'){
					t += (*it - '0') * 60 * 60 * 1000;
					unit = TimeUnit::H_10;
				}else
					return false;
				break;
			case TimeUnit::H_10:
				if(*it >= '0' && *it <= '9'){
					t += (*it - '0') * 10 * 60 * 60 * 1000;
					unit = TimeUnit::END;
				}else
					return false;
				break;
			case TimeUnit::END:
				return false;
		}
	// No problem occured
	return true;
}

// Parser implementations
void SSB::Parser::parse_script(SSB::Data& data, std::istream& script) throw(std::string){
	// Skip UTF-8 byte-order-mask
	unsigned char BOM[3];
	script.read(reinterpret_cast<char*>(BOM), 3);
	if(!(script.gcount() == 3 && BOM[0] == 0xef && BOM[1] == 0xbb && BOM[2] == 0xbf))
		script.seekg(0);
	// Line number for advanced error message
	unsigned long line_number = 0;
	// Line input buffer
	std::string line;
	// Catch error in actual parsing
	try{
		// Iterate through stream lines
		while(std::getline(script, line)){
			// Remove carriage return in case of CRLF ending
			if(!line.empty() && line.back() == '\r')
				line.pop_back();
			// Update line number to current stream
			line_number++;
			// Parse the current stream line
			this->parse_line(data, line);
		}
	}catch(std::string error_message){
		// Rethrow error message with additional line number
                throw line_number + ": " + error_message;
	}
}

void SSB::Parser::parse_line(SSB::Data& data, std::string& line) throw(std::string){
	// No empty or comment line = no skip
	if(!line.empty() && line.compare(0, 2, "//") != 0){
		// Section header line
		if(line.front() == '#'){
			std::string section = line.substr(1);
			if(section == "META")
				data.current_section = SSB::Data::Section::META;
			else if(section == "FRAME")
				data.current_section = SSB::Data::Section::FRAME;
			else if(section == "STYLES")
				data.current_section = SSB::Data::Section::STYLES;
			else if(section == "EVENTS")
				data.current_section = SSB::Data::Section::EVENTS;
			else if(this->level != SSB::Parser::Level::OFF)
				throw std::string("Invalid section name");
		}else	// Section content line
			switch(data.current_section){
				case SSB::Data::Section::META:
					if(line.compare(0, 7, "Title: ") == 0)
						data.meta.title = line.substr(7);
					else if(line.compare(0, 8, "Author: ") == 0)
						data.meta.author = line.substr(8);
					else if(line.compare(0, 13, "Description: ") == 0)
						data.meta.description = line.substr(13);
					else if(line.compare(0, 9, "Version: ") == 0)
						data.meta.version = line.substr(9);
					else if(this->level != SSB::Parser::Level::OFF)
						throw std::string("Invalid meta field");
					break;
				case SSB::Data::Section::FRAME:
					if(line.compare(0, 7, "Width: ") == 0){
						decltype(data.frame.width) width;
						if(string_to_number(line.substr(7), width))
							data.frame.width = width;
						else if(this->level == SSB::Parser::Level::ALL)
							throw std::string("Invalid frame width");
					}else if(line.compare(0,8, "Heigth: ") == 0){
						decltype(data.frame.height) height;
						if(string_to_number(line.substr(8), height))
							data.frame.height = height;
						else if(this->level == SSB::Parser::Level::ALL)
							throw std::string("Invalid frame height");
					}else if(this->level != SSB::Parser::Level::OFF)
						throw std::string("Invalid frame field");
					break;
				case SSB::Data::Section::STYLES:{
						auto split_pos = line.find(": ");
						if(split_pos != std::string::npos)
							data.styles[line.substr(0, split_pos)] = line.substr(split_pos+2);
						else if(this->level != SSB::Parser::Level::OFF)
							throw std::string("Invalid styles field");
					}
					break;
				case SSB::Data::Section::EVENTS:{
						// Temporary event buffer
						SSB::Event event;
						// Prepare line tokenization
						std::istringstream line_stream(line);
						std::string line_token;
						// Extract start time
						if(!std::getline(line_stream, line_token, '-') || !parse_time(line_token, event.start_ms)){
							if(this->level != SSB::Parser::Level::OFF)
								throw std::string("Couldn't find start time");
							break;
						}
						// Extract end time
						if(!std::getline(line_stream, line_token, '|') || !parse_time(line_token, event.end_ms)){
							if(this->level != SSB::Parser::Level::OFF)
								throw std::string("Couldn't find end time");
							break;
						}
						// Check valid times
						if(event.start_ms < event.end_ms){
							if(this->level == SSB::Parser::Level::ALL)
								throw std::string("Start time mustn't be after end time");
							break;
						}
						// Extract style name
						if(!std::getline(line_stream, line_token, '|')){
							if(this->level != SSB::Parser::Level::OFF)
								throw std::string("Couldn't find style");
							break;
						}
						// Get style content for later insertion
						std::string style_content;
						if(!line_token.empty() && !data.styles.count(line_token)){
							if(this->level == SSB::Parser::Level::ALL)
								throw std::string("Couldn't find style");
							break;
						}else
							style_content = data.styles[line_token];
						// Skip note
						if(!std::getline(line_stream, line_token, '|')){
							if(this->level != SSB::Parser::Level::OFF)
								throw std::string("Couldn't find note");
							break;
						}
						// Extract text
						if(!line_stream.unget() || line_stream.get() != '|'){
							if(this->level != SSB::Parser::Level::OFF)
								throw std::string("Couldn't find text");
							break;
						}
						std::string text = std::getline(line_stream, line_token) ? style_content + line_token : style_content;
						// Add inline styles to text
						unsigned inline_count = MAX_INLINE_STYLES;
						std::string::size_type pos_start = 0, pos_end;
						while(inline_count && (pos_start = text.find("\\\\", pos_start)) != std::string::npos && (pos_end = text.find("\\\\", pos_start+2)) != std::string::npos){
							std::string macro = text.substr(pos_start + 2, pos_end - (pos_start + 2));
							if(data.styles.count(macro)){
								text.replace(pos_start, macro.length()+4, data.styles[macro]);
								inline_count--;
							}
								pos_start = pos_end + 2;
						}
						// Evaluate text tokens

						// TODO

						// Event complete -> commit to data
						data.events.push_back(event);
					}
					break;
				case SSB::Data::Section::NONE:
					if(this->level != SSB::Parser::Level::OFF)
						throw std::string("No section set");
					break;
			}
	}
}
