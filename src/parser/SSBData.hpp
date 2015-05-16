/*
Project: SSBRenderer
File: SSBData.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

namespace SSB{
	// Time range
	using Time = unsigned long;
	// Coordinate precision
	using Coord = double;

	class Object{
		// TODO
	};

	// Meta informations (no effect on rendering)
	struct Meta{
		std::string title, description, author, version;
	};

	// Target frame resolution
	struct Frame{
		unsigned width = 0, height = 0;
	};

	// Event with rendering informations
	struct Event{
		Time start_ms = 0, end_ms = 0;
		std::vector<std::shared_ptr<Object>> objects;
		bool static_tags = true;
	};

	// Complete data from any script
	struct Data{
		enum class Section{NONE, META, FRAME, STYLES, EVENTS} current_section = Section::NONE;
		Meta meta;
		Frame frame;
		std::map<std::string, std::string>/*Name, Content*/ styles;
		std::vector<Event> events;
	};
}	// namespace SSB