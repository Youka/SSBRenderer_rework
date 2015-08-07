/*
Project: SSBRenderer
File: SSBParser.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "SSBData.hpp"
#include <exception>

namespace SSB{
	// Parser exception
	class Exception : public std::exception{
		private:
			std::string message;
		public:
			Exception(std::string message) : message(message){}
			const char* what() const noexcept override{return this->message.c_str();}
	};

	// Subtilte parser to fill data containers
	class Parser{
		private:
			// Level of error detection (0=OFF, 1=SYNTAX, 3=+VALUES)
			enum class Level{OFF, SYNTAX, ALL} const level;
			// Parse event elements
			void parse_geometry(std::string& geometry, Geometry::Type geometry_type, Event& event) throw(Exception);
			void parse_tags(std::string& tags, Geometry::Type& geometry_type, Event& event) throw(Exception);
		public:
			// Constructor
			Parser(Level level = Level::ALL) : level(level){};
			// Parse one text line
			void parse_line(Data& data, std::string& line) throw(Exception);
			// Parse a whole script from stream
			void parse_script(Data& data, std::istream& script) throw(Exception);
	};
}	// namespace SSB
