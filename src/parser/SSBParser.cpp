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
#include "string_utils.hpp"
#include <algorithm>

// Parses SSB time and converts to milliseconds
template<typename T>
static inline bool parse_time(std::string& s, T& t){
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
#define ADD_OBJECT(obj) event.objects.emplace_back(new obj)
#define THROW_STRONG_ERROR(msg) if(this->level != SSB::Parser::Level::OFF) throw SSB::Exception(msg)
#define THROW_WEAK_ERROR(msg) if(this->level == SSB::Parser::Level::ALL) throw SSB::Exception(msg)
void SSB::Parser::parse_geometry(std::string& geometry, SSB::Geometry::Type geometry_type, SSB::Event& event) throw(SSB::Exception){
	switch(geometry_type){
		case SSB::Geometry::Type::POINTS:{
				// Points buffer
				std::vector<SSB::Point> points;
				// Iterate through numbers
				std::istringstream points_stream(geometry);
				SSB::Point point;
				while(points_stream >> point.x)
					if(points_stream >> point.y)
						points.push_back(point);
					else
						THROW_WEAK_ERROR("Points must have 2 numbers");
				// Check for successfull streaming end
				if((points_stream >> std::ws).eof())
					ADD_OBJECT(SSB::Points(points));
				else
					THROW_WEAK_ERROR("Points are invalid");
			}
			break;
		case SSB::Geometry::Type::PATH:{
				// Path segments buffer
				std::vector<SSB::Path::Segment> path;
				// Iterate through words
				std::istringstream path_stream(geometry);
				std::string path_token;
				SSB::Path::Segment segments[3];
				segments[0].type = SSB::Path::SegmentType::MOVE_TO;
				while(path_stream >> path_token)
					// Save next segment type
					if(path_token == "m")
						segments[0].type = SSB::Path::SegmentType::MOVE_TO;
					else if(path_token == "l")
						segments[0].type = SSB::Path::SegmentType::LINE_TO;
					else if(path_token == "b")
						segments[0].type = segments[1].type = segments[2].type = SSB::Path::SegmentType::CURVE_TO;
					else if(path_token == "a")
						segments[0].type = segments[1].type = SSB::Path::SegmentType::ARC_TO;
					else if(path_token == "c"){
						segments[0].type = SSB::Path::SegmentType::CLOSE;
						path.push_back({SSB::Path::SegmentType::CLOSE, 0, 0});
					// Complete next segment
					}else{
						// Put token back in stream for rereading
						path_stream.seekg(-static_cast<long>(path_token.length()), std::istringstream::cur);
						// Parse segment data
						switch(segments[0].type){
							case SSB::Path::SegmentType::MOVE_TO:
							case SSB::Path::SegmentType::LINE_TO:
								if(path_stream >> segments[0].point.x &&
									path_stream >> segments[0].point.y)
									path.push_back(segments[0]);
								else
									THROW_WEAK_ERROR(segments[0].type == SSB::Path::SegmentType::MOVE_TO ? "Path (move) is invalid" : "Path (line) is invalid");
								break;
							case SSB::Path::SegmentType::CURVE_TO:
								if(path_stream >> segments[0].point.x &&
									path_stream >> segments[0].point.y &&
									path_stream >> segments[1].point.x &&
									path_stream >> segments[1].point.y &&
									path_stream >> segments[2].point.x &&
									path_stream >> segments[2].point.y){
									path.push_back(segments[0]);
									path.push_back(segments[1]);
									path.push_back(segments[2]);
								}else
									THROW_WEAK_ERROR("Path (curve) is invalid");
								break;
							case SSB::Path::SegmentType::ARC_TO:
								if(path_stream >> segments[0].point.x &&
									path_stream >> segments[0].point.y &&
									path_stream >> segments[1].angle){
									path.push_back(segments[0]);
									path.push_back(segments[1]);
								}else
									THROW_WEAK_ERROR("Path (arc) is invalid");
								break;
							case SSB::Path::SegmentType::CLOSE:
								THROW_WEAK_ERROR("Path (close) is invalid");
								break;
						}
					}
				// Successful collection of segments
				ADD_OBJECT(SSB::Path(path));
			}
			break;
		case SSB::Geometry::Type::TEXT:{
				// Replace in string \t to 4 spaces
				string_replace(geometry, "\t", "    ");
				// Replace in string \n to real line breaks
				string_replace(geometry, "\\n", "\n");
				// Replace in string \{ to single {
				string_replace(geometry, "\\{", "{");
				// Insert Text as Object to event
				ADD_OBJECT(SSB::Text(geometry));
			}
			break;
	}
}

#define STR_LIT_EQU_FIRST(s, s2) (s.compare(0, sizeof(s2)-1, s2) == 0)
void SSB::Parser::parse_tags(std::string& tags, SSB::Geometry::Type& geometry_type, SSB::Event& event) throw(SSB::Exception){
	// Iterate through tags
	std::istringstream tags_stream(tags);
	std::string tags_token;
	while(std::getline(tags_stream, tags_token, ';'))
		// Evaluate single tags
		if(STR_LIT_EQU_FIRST(tags_token, "ff="))
			ADD_OBJECT(SSB::FontFamily(tags_token.substr(3)));
		else if(STR_LIT_EQU_FIRST(tags_token, "fst=")){
			bool bold = false, italic = false, underline = false, strikeout = false;
			for(char c : tags_token.substr(4))
				if(c == 'b' && !bold)
					bold = true;
				else if(c == 'i' && !italic)
					italic = true;
				else if(c == 'u' && !underline)
					underline = true;
				else if(c == 's' && !strikeout)
					strikeout = true;
				else
					THROW_WEAK_ERROR("Invalid font style");
			ADD_OBJECT(SSB::FontStyle(bold, italic, underline, strikeout));
		}else if(STR_LIT_EQU_FIRST(tags_token, "fs=")){
			decltype(SSB::FontSize::size) size;
			if(string_to_number(tags_token.substr(3), size) && size >= 0)
				ADD_OBJECT(SSB::FontSize(size));
			else
				THROW_WEAK_ERROR("Invalid font size");
		}else if(STR_LIT_EQU_FIRST(tags_token, "fsp=")){
			decltype(SSB::FontSpace::x) x, y;
			if(string_to_number(tags_token.substr(4), x, y))
				ADD_OBJECT(SSB::FontSpace(x, y));
			else
				THROW_WEAK_ERROR("Invalid font spaces");
		}else if(STR_LIT_EQU_FIRST(tags_token, "fsph=")){
			decltype(SSB::FontSpace::x) x;
			if(string_to_number(tags_token.substr(5), x))
				ADD_OBJECT(SSB::FontSpace(SSB::FontSpace::Type::HORIZONTAL, x));
			else
				THROW_WEAK_ERROR("Invalid horizontal font space");
		}else if(STR_LIT_EQU_FIRST(tags_token, "fspv=")){
			decltype(SSB::FontSpace::y) y;
			if(string_to_number(tags_token.substr(5), y))
				ADD_OBJECT(SSB::FontSpace(SSB::FontSpace::Type::VERTICAL, y));
			else
				THROW_WEAK_ERROR("Invalid vertical font space");
		}else if(STR_LIT_EQU_FIRST(tags_token, "lw=")){
			decltype(SSB::LineWidth::width) width;
			if(string_to_number(tags_token.substr(3), width) && width >= 0)
				ADD_OBJECT(SSB::LineWidth(width));
			else
				THROW_WEAK_ERROR("Invalid line width");
		}else if(STR_LIT_EQU_FIRST(tags_token, "lst=")){
			std::string tag_value = tags_token.substr(4);
			std::string::size_type pos;
			if((pos = tag_value.find(',')) != std::string::npos){
				std::string join_string = tag_value.substr(0, pos), cap_string = tag_value.substr(pos+1);
				SSB::LineStyle::Join join = SSB::LineStyle::Join::ROUND;
				if(join_string == "r")
					join = SSB::LineStyle::Join::ROUND;
				else if(join_string == "b")
					join = SSB::LineStyle::Join::BEVEL;
				else
					THROW_WEAK_ERROR("Invalid line style join");
				SSB::LineStyle::Cap cap = SSB::LineStyle::Cap::ROUND;
				if(cap_string == "r")
					cap = SSB::LineStyle::Cap::ROUND;
				else if(cap_string == "f")
					cap = SSB::LineStyle::Cap::FLAT;
				else
					THROW_WEAK_ERROR("Invalid line style cap");
				ADD_OBJECT(SSB::LineStyle(join, cap));
			}else
				THROW_WEAK_ERROR("Invalid line style");
		}else if(STR_LIT_EQU_FIRST(tags_token, "ld=")){
			decltype(SSB::LineDash::offset) offset;
			std::istringstream dash_stream(tags_token.substr(3));
			std::string dash_token;
			if(std::getline(dash_stream, dash_token, ',') && string_to_number(dash_token, offset) && offset >= 0){
				decltype(SSB::LineDash::dashes) dashes;
				decltype(SSB::LineDash::offset) dash;
				while(std::getline(dash_stream, dash_token, ','))
					if(string_to_number(dash_token, dash) && dash >= 0)
						dashes.push_back(dash);
					else
						THROW_WEAK_ERROR("Invalid line dash");
				if(static_cast<size_t>(std::count(dashes.begin(), dashes.end(), 0)) != dashes.size())	// Not all dashes should be zero
					ADD_OBJECT(SSB::LineDash(offset, dashes));
				else
					THROW_WEAK_ERROR("Dashes must not be only 0");
			}else
				THROW_WEAK_ERROR("Invalid line dashes");
		}else if(STR_LIT_EQU_FIRST(tags_token, "gm=")){
			std::string tag_value = tags_token.substr(3);
			if(tag_value == "pt")
				geometry_type = SSB::Geometry::Type::POINTS;
			else if(tag_value == "p")
				geometry_type = SSB::Geometry::Type::PATH;
			else if(tag_value == "t")
				geometry_type = SSB::Geometry::Type::TEXT;
			else
				THROW_WEAK_ERROR("Invalid geometry");
		}else if(STR_LIT_EQU_FIRST(tags_token, "md=")){
			std::string tag_value = tags_token.substr(3);
			if(tag_value == "f")
				ADD_OBJECT(SSB::Mode(SSB::Mode::Method::FILL));
			else if(tag_value == "w")
				ADD_OBJECT(SSB::Mode(SSB::Mode::Method::WIRE));
			else if(tag_value == "b")
				ADD_OBJECT(SSB::Mode(SSB::Mode::Method::BOXED));
			else
				THROW_WEAK_ERROR("Invalid mode");
		}else if(STR_LIT_EQU_FIRST(tags_token, "df=")){
			std::string tag_value = tags_token.substr(3);
			std::string::size_type pos;
			if((pos = tag_value.find(',')) != std::string::npos && tag_value.find(',', pos+1) == std::string::npos)
				ADD_OBJECT(SSB::Deform(tag_value.substr(0, pos), tag_value.substr(pos+1)));
			else
				THROW_WEAK_ERROR("Invalid deform");
		}else if(STR_LIT_EQU_FIRST(tags_token, "pos=")){
			std::string tag_value = tags_token.substr(4);
			decltype(SSB::Position::x) x, y;
			constexpr decltype(x) max_pos = std::numeric_limits<decltype(x)>::max();
			if(tag_value.empty())
				ADD_OBJECT(SSB::Position(max_pos, max_pos));
			else if(string_to_number(tag_value, x, y))
				ADD_OBJECT(SSB::Position(x, y));
			else
				THROW_WEAK_ERROR("Invalid position");
		}else if(STR_LIT_EQU_FIRST(tags_token, "an=")){
			std::string tag_value = tags_token.substr(3);
			if(tag_value.length() == 1 && tag_value[0] >= '1' && tag_value[0] <= '9')
				ADD_OBJECT(SSB::Align(static_cast<SSB::Align::Position>(tag_value[0] - '0')));
			else
				THROW_WEAK_ERROR("Invalid alignment");
		}else if(STR_LIT_EQU_FIRST(tags_token, "mg=")){
			std::string tag_value = tags_token.substr(3);
			decltype(SSB::Margin::x) x, y;
			if(string_to_number(tag_value, x))
				ADD_OBJECT(SSB::Margin(SSB::Margin::Type::BOTH, x));
			else if(string_to_number(tag_value, x, y))
				ADD_OBJECT(SSB::Margin(x, y));
			else
				THROW_WEAK_ERROR("Invalid margin");
		}else if(STR_LIT_EQU_FIRST(tags_token, "mgh=")){
			decltype(SSB::Margin::x) x;
			if(string_to_number(tags_token.substr(4), x))
				ADD_OBJECT(SSB::Margin(SSB::Margin::Type::HORIZONTAL, x));
			else
				THROW_WEAK_ERROR("Invalid horizontal margin");
		}else if(STR_LIT_EQU_FIRST(tags_token, "mgv=")){
			decltype(SSB::Margin::y) y;
			if(string_to_number(tags_token.substr(4), y))
				ADD_OBJECT(SSB::Margin(SSB::Margin::Type::VERTICAL, y));
			else
				THROW_WEAK_ERROR("Invalid vertical margin");
		}else if(STR_LIT_EQU_FIRST(tags_token, "dir=")){
			std::string tag_value = tags_token.substr(4);
			if(tag_value == "ltr")
				ADD_OBJECT(SSB::Direction(SSB::Direction::Mode::LTR));
			else if(tag_value == "rtl")
				ADD_OBJECT(SSB::Direction(SSB::Direction::Mode::RTL));
			else if(tag_value == "ttb")
				ADD_OBJECT(SSB::Direction(SSB::Direction::Mode::TTB));
			else
				THROW_WEAK_ERROR("Invalid direction");
		}else if(tags_token == "id")
			ADD_OBJECT(SSB::Identity());
		else if(STR_LIT_EQU_FIRST(tags_token, "tl=")){
			decltype(SSB::Translate::x) x, y;
			if(string_to_number(tags_token.substr(3), x, y))
				ADD_OBJECT(SSB::Translate(x, y));
			else
				THROW_WEAK_ERROR("Invalid translation");
		}else if(STR_LIT_EQU_FIRST(tags_token, "tlx=")){
			decltype(SSB::Translate::x) x;
			if(string_to_number(tags_token.substr(4), x))
				ADD_OBJECT(SSB::Translate(SSB::Translate::Type::HORIZONTAL, x));
			else
				THROW_WEAK_ERROR("Invalid horizontal translation");
		}else if(STR_LIT_EQU_FIRST(tags_token, "tly=")){
			decltype(SSB::Translate::y) y;
			if(string_to_number(tags_token.substr(4), y))
				ADD_OBJECT(SSB::Translate(SSB::Translate::Type::VERTICAL, y));
			else
				THROW_WEAK_ERROR("Invalid vertical translation");
		}else if(STR_LIT_EQU_FIRST(tags_token, "sc=")){
			std::string tag_value = tags_token.substr(3);
			decltype(SSB::Scale::x) x, y;
			if(string_to_number(tag_value, x))
				ADD_OBJECT(SSB::Scale(SSB::Scale::Type::BOTH, x));
			else if(string_to_number(tag_value, x, y))
				ADD_OBJECT(SSB::Scale(x, y));
			else
				THROW_WEAK_ERROR("Invalid scale");
		}else if(STR_LIT_EQU_FIRST(tags_token, "scx=")){
			decltype(SSB::Scale::x) x;
			if(string_to_number(tags_token.substr(4), x))
				ADD_OBJECT(SSB::Scale(SSB::Scale::Type::HORIZONTAL, x));
			else
				THROW_WEAK_ERROR("Invalid horizontal scale");
		}else if(STR_LIT_EQU_FIRST(tags_token, "scy=")){
			decltype(SSB::Scale::y) y;
			if(string_to_number(tags_token.substr(4), y))
				ADD_OBJECT(SSB::Scale(SSB::Scale::Type::VERTICAL, y));
			else
				THROW_WEAK_ERROR("Invalid vertical scale");
		}else if(STR_LIT_EQU_FIRST(tags_token, "rxy=")){
			decltype(SSB::Rotate::angle1) angle1, angle2;
			if(string_to_number(tags_token.substr(4), angle1, angle2))
				ADD_OBJECT(SSB::Rotate(SSB::Rotate::Axis::XY, angle1, angle2));
			else
				THROW_WEAK_ERROR("Invalid rotation on x axis");
		}else if(STR_LIT_EQU_FIRST(tags_token, "ryx=")){
			decltype(SSB::Rotate::angle1) angle1, angle2;
			if(string_to_number(tags_token.substr(4), angle1, angle2))
				ADD_OBJECT(SSB::Rotate(SSB::Rotate::Axis::YX, angle1, angle2));
			else
				THROW_WEAK_ERROR("Invalid rotation on y axis");
		}else if(STR_LIT_EQU_FIRST(tags_token, "rz=")){
			decltype(SSB::Rotate::angle1) angle;
			if(string_to_number(tags_token.substr(3), angle))
				ADD_OBJECT(SSB::Rotate(angle));
			else
				THROW_WEAK_ERROR("Invalid rotation on z axis");
		}else if(STR_LIT_EQU_FIRST(tags_token, "sh=")){
			decltype(SSB::Shear::x) x, y;
			if(string_to_number(tags_token.substr(3), x, y))
				ADD_OBJECT(SSB::Shear(x, y));
			else
				THROW_WEAK_ERROR("Invalid shear");
		}else if(STR_LIT_EQU_FIRST(tags_token, "shx=")){
			decltype(SSB::Shear::x) x;
			if(string_to_number(tags_token.substr(4), x))
				ADD_OBJECT(SSB::Shear(SSB::Shear::Type::HORIZONTAL, x));
			else
				THROW_WEAK_ERROR("Invalid horizontal shear");
		}else if(STR_LIT_EQU_FIRST(tags_token, "shy")){
			decltype(SSB::Shear::y) y;
			if(string_to_number(tags_token.substr(4), y))
				ADD_OBJECT(SSB::Shear(SSB::Shear::Type::VERTICAL, y));
			else
				THROW_WEAK_ERROR("Invalid vertical shear");
		}else if(STR_LIT_EQU_FIRST(tags_token, "tf=")){
			decltype(SSB::Transform::xx) xx, yx, xy, yy, x0, y0;
			std::istringstream matrix_stream(tags_token.substr(3));
			std::string matrix_token;
			if(std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, xx) &&
					std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, yx) &&
					std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, xy) &&
					std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, yy) &&
					std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, x0) &&
					std::getline(matrix_stream, matrix_token, ',') && string_to_number(matrix_token, y0) &&
					matrix_stream.eof()
				)
				ADD_OBJECT(SSB::Transform(xx, yx, xy, yy, x0, y0));
			else
				THROW_WEAK_ERROR("Invalid transform");
		}else if(STR_LIT_EQU_FIRST(tags_token, "cl=")){
			std::string tag_value = tags_token.substr(3);
			unsigned long rgb[4];
			if(hex_string_to_number(tag_value, rgb[0]) &&
				rgb[0] <= 0xffffff)
				ADD_OBJECT(SSB::Color(
								static_cast<decltype(SSB::RGB::r)>(rgb[0] >> 16) / 0xff,
								static_cast<decltype(SSB::RGB::g)>(rgb[0] >> 8 & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::b)>(rgb[0] & 0xff) / 0xff
							));
			else if(hex_string_to_number(tag_value, rgb[0], rgb[1]) &&
					rgb[0] <= 0xffffff && rgb[1] <= 0xffffff)
				ADD_OBJECT(SSB::Color(
								static_cast<decltype(SSB::RGB::r)>(rgb[0] >> 16) / 0xff,
								static_cast<decltype(SSB::RGB::g)>(rgb[0] >> 8 & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::b)>(rgb[0] & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::r)>(rgb[1] >> 16) / 0xff,
								static_cast<decltype(SSB::RGB::g)>(rgb[1] >> 8 & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::b)>(rgb[1] & 0xff) / 0xff
							));
			else if(hex_string_to_number(tag_value, rgb[0], rgb[1], rgb[2], rgb[3]) &&
					rgb[0] <= 0xffffff && rgb[1] <= 0xffffff && rgb[2] <= 0xffffff && rgb[3] <= 0xffffff)
				ADD_OBJECT(SSB::Color(
								static_cast<decltype(SSB::RGB::r)>(rgb[0] >> 16) / 0xff,
								static_cast<decltype(SSB::RGB::g)>(rgb[0] >> 8 & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::b)>(rgb[0] & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::r)>(rgb[1] >> 16) / 0xff,
								static_cast<decltype(SSB::RGB::g)>(rgb[1] >> 8 & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::b)>(rgb[1] & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::r)>(rgb[2] >> 16) / 0xff,
								static_cast<decltype(SSB::RGB::g)>(rgb[2] >> 8 & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::b)>(rgb[2] & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::r)>(rgb[3] >> 16) / 0xff,
								static_cast<decltype(SSB::RGB::g)>(rgb[3] >> 8 & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::b)>(rgb[3] & 0xff) / 0xff
							));
			else
				THROW_WEAK_ERROR("Invalid color");
		}else if(STR_LIT_EQU_FIRST(tags_token, "lcl=")){
			unsigned long rgb;
			if(hex_string_to_number(tags_token.substr(4), rgb) &&
					rgb <= 0xffffff)
				ADD_OBJECT(SSB::LineColor(
								static_cast<decltype(SSB::RGB::r)>(rgb >> 16) / 0xff,
								static_cast<decltype(SSB::RGB::g)>(rgb >> 8 & 0xff) / 0xff,
								static_cast<decltype(SSB::RGB::b)>(rgb & 0xff) / 0xff
							));
			else
				THROW_WEAK_ERROR("Invalid line color");
		}else if(STR_LIT_EQU_FIRST(tags_token, "al=")){
			std::string tag_value = tags_token.substr(3);
			unsigned short a[4];
			if(hex_string_to_number(tag_value, a[0]) &&
					a[0] <= 0xff)
				ADD_OBJECT(SSB::Alpha(static_cast<decltype(SSB::RGB::r)>(a[0]) / 0xff));
			else if(hex_string_to_number(tag_value, a[0], a[1]) &&
					a[0] <= 0xff && a[1] <= 0xff)
				ADD_OBJECT(SSB::Alpha(
								static_cast<decltype(SSB::RGB::r)>(a[0]) / 0xff,
								static_cast<decltype(SSB::RGB::r)>(a[1]) / 0xff
							));
			else if(hex_string_to_number(tag_value, a[0], a[1], a[2], a[3]) &&
					a[0] <= 0xff && a[1] <= 0xff && a[2] <= 0xff && a[3] <= 0xff)
				ADD_OBJECT(SSB::Alpha(
								static_cast<decltype(SSB::RGB::r)>(a[0]) / 0xff,
								static_cast<decltype(SSB::RGB::r)>(a[1]) / 0xff,
								static_cast<decltype(SSB::RGB::r)>(a[2]) / 0xff,
								static_cast<decltype(SSB::RGB::r)>(a[3]) / 0xff
							));
			else
				THROW_WEAK_ERROR("Invalid alpha");
		}else if(STR_LIT_EQU_FIRST(tags_token, "lal=")){
			unsigned short a;
			if(hex_string_to_number(tags_token.substr(4), a) &&
					a <= 0xff)
				ADD_OBJECT(SSB::LineAlpha(static_cast<decltype(SSB::RGB::r)>(a) / 0xff));
			else
				THROW_WEAK_ERROR("Invalid line alpha");
		}else if(STR_LIT_EQU_FIRST(tags_token, "tex=")){
			ADD_OBJECT(SSB::Texture(tags_token.substr(4)));
		}else if(STR_LIT_EQU_FIRST(tags_token, "texf=")){
			std::string tag_value = tags_token.substr(5);
			decltype(SSB::TexFill::x) x, y;
			std::string::size_type pos1, pos2;
			if((pos1 = tag_value.find(',')) != std::string::npos &&
					string_to_number(tag_value.substr(0, pos1), x) &&
					(pos2 = tag_value.find(',', pos1+1)) != std::string::npos &&
					string_to_number(tag_value.substr(pos1+1, pos2-(pos1+1)), y)){
				std::string wrap = tag_value.substr(pos2+1);
				if(wrap == "c")
					ADD_OBJECT(SSB::TexFill(x, y, SSB::TexFill::WrapStyle::CLAMP));
				else if(wrap == "r")
					ADD_OBJECT(SSB::TexFill(x, y, SSB::TexFill::WrapStyle::REPEAT));
				else if(wrap == "m")
					ADD_OBJECT(SSB::TexFill(x, y, SSB::TexFill::WrapStyle::MIRROR));
				else if(wrap == "f")
					ADD_OBJECT(SSB::TexFill(x, y, SSB::TexFill::WrapStyle::FLOW));
				else
					THROW_WEAK_ERROR("Invalid texture filling wrap style");
			}else
				THROW_WEAK_ERROR("Invalid texture filling");
		}else if(STR_LIT_EQU_FIRST(tags_token, "bld=")){
			std::string tag_value = tags_token.substr(4);
			if(tag_value == "over")
				ADD_OBJECT(SSB::Blend(SSB::Blend::Mode::OVER));
			else if(tag_value == "add")
				ADD_OBJECT(SSB::Blend(SSB::Blend::Mode::ADDITION));
			else if(tag_value == "sub")
				ADD_OBJECT(SSB::Blend(SSB::Blend::Mode::SUBTRACT));
			else if(tag_value == "mult")
				ADD_OBJECT(SSB::Blend(SSB::Blend::Mode::MULTIPLY));
			else if(tag_value == "scr")
				ADD_OBJECT(SSB::Blend(SSB::Blend::Mode::SCREEN));
			else if(tag_value == "diff")
				ADD_OBJECT(SSB::Blend(SSB::Blend::Mode::DIFFERENCES));
			else
				THROW_WEAK_ERROR("Invalid blending");
		}else if(STR_LIT_EQU_FIRST(tags_token, "bl=")){
			std::string tag_value = tags_token.substr(3);
			decltype(SSB::Blur::x) x, y;
			if(string_to_number(tag_value, x) && x >= 0)
				ADD_OBJECT(SSB::Blur(SSB::Blur::Type::BOTH, x));
			else if(string_to_number(tag_value, x, y) && x >= 0 && y >= 0)
				ADD_OBJECT(SSB::Blur(x, y));
			else
				THROW_WEAK_ERROR("Invalid blur");
		}else if(STR_LIT_EQU_FIRST(tags_token, "blh=")){
			decltype(SSB::Blur::x) x;
			if(string_to_number(tags_token.substr(4), x) && x >= 0)
				ADD_OBJECT(SSB::Blur(SSB::Blur::Type::HORIZONTAL, x));
			else
				THROW_WEAK_ERROR("Invalid horizontal blur");
		}else if(STR_LIT_EQU_FIRST(tags_token, "blv=")){
			decltype(SSB::Blur::y) y;
			if(string_to_number(tags_token.substr(4), y) && y >= 0)
				ADD_OBJECT(SSB::Blur(SSB::Blur::Type::VERTICAL, y));
			else
				THROW_WEAK_ERROR("Invalid vertical blur");
		}else if(STR_LIT_EQU_FIRST(tags_token, "stc=")){
			std::string tag_value = tags_token.substr(4);
			if(tag_value == "off")
				ADD_OBJECT(SSB::Stencil(SSB::Stencil::Mode::OFF));
			else if(tag_value == "set")
				ADD_OBJECT(SSB::Stencil(SSB::Stencil::Mode::SET));
			else if(tag_value == "uset")
				ADD_OBJECT(SSB::Stencil(SSB::Stencil::Mode::UNSET));
			else if(tag_value == "in")
				ADD_OBJECT(SSB::Stencil(SSB::Stencil::Mode::INSIDE));
			else if(tag_value == "out")
				ADD_OBJECT(SSB::Stencil(SSB::Stencil::Mode::OUTSIDE));
			else
				THROW_WEAK_ERROR("Invalid stencil mode");
		}else if(STR_LIT_EQU_FIRST(tags_token, "aa=")){
			std::string tag_value = tags_token.substr(3);
			if(tag_value == "on")
				ADD_OBJECT(SSB::AntiAliasing(true));
			else if(tag_value == "off")
				ADD_OBJECT(SSB::AntiAliasing(false));
			else
				THROW_WEAK_ERROR("Invalid anti-aliasing mode");
		}else if(STR_LIT_EQU_FIRST(tags_token, "fad=")){
			std::string tag_value = tags_token.substr(4);
			decltype(SSB::Fade::in) in, out;
			if(string_to_number(tag_value, in))
				ADD_OBJECT(SSB::Fade(SSB::Fade::Type::BOTH, in));
			else if(string_to_number(tag_value, in, out))
				ADD_OBJECT(SSB::Fade(in, out));
			else
				THROW_WEAK_ERROR("Invalid fade");
		}else if(STR_LIT_EQU_FIRST(tags_token, "fadi=")){
			decltype(SSB::Fade::in) in;
			if(string_to_number(tags_token.substr(5), in))
				ADD_OBJECT(SSB::Fade(SSB::Fade::Type::INFADE, in));
			else
				THROW_WEAK_ERROR("Invalid infade");
		}else if(STR_LIT_EQU_FIRST(tags_token, "fado=")){
			decltype(SSB::Fade::out) out;
			if(string_to_number(tags_token.substr(5), out))
				ADD_OBJECT(SSB::Fade(SSB::Fade::Type::OUTFADE, out));
			else
				THROW_WEAK_ERROR("Invalid outfade");
		}else if(STR_LIT_EQU_FIRST(tags_token, "ani=")){
				// Collect animation tokens (maximum: 4)
				std::vector<std::string> animate_tokens;
				std::istringstream animate_stream(tags_token.substr(4));
				std::string animate_token;
				for(unsigned char i = 0; std::getline(animate_stream, animate_token, ',') && i < 4; ++i)
					// Get last token with brackets
					if(!animate_token.empty() && animate_token.front() == '('){
						// If animated tags contain ',' too, add the rest of animation
						if(animate_stream.unget() && animate_stream.get() == ','){
							std::string animate_rest;
							std::getline(animate_stream, animate_rest);
							animate_token += ',' + animate_rest;
						}
						// Extend animation stream to get all animated tags
						while(animate_token.back() != ')' && std::getline(tags_stream, tags_token, ';'))
							animate_token += ';' + tags_token;
						animate_tokens.push_back(std::move(animate_token));
						// Finish collecting after last possible token
						break;
					// Get first tokens (times & formula)
					}else
						animate_tokens.push_back(std::move(animate_token));
				// Check for enough animation tokens and last token for brackets
				if(animate_tokens.size() > 0 && animate_tokens.back().length() >= 2 && animate_tokens.back().front() == '(' && animate_tokens.back().back() == ')'){
					// Get animation values
					constexpr decltype(SSB::Animate::start) max_duration = std::numeric_limits<decltype(SSB::Animate::start)>::max();
					decltype(SSB::Animate::start) start_time = max_duration, end_time = max_duration;
					std::string progress_formula, tags;
					SSB::Event buffer_event;
					try{
						switch(animate_tokens.size()){
							case 1: tags = animate_tokens[0].substr(1, animate_tokens[0].size()-2);
								break;
							case 2: progress_formula = animate_tokens[0];
								tags = animate_tokens[1].substr(1, animate_tokens[1].size()-2);
								break;
							case 3: if(!string_to_number(animate_tokens[0], start_time) || !string_to_number(animate_tokens[1], end_time))
									throw std::string("Invalid time(s)");
								tags = animate_tokens[2].substr(1, animate_tokens[2].size()-2);
								break;
							case 4: if(!string_to_number(animate_tokens[0], start_time) || !string_to_number(animate_tokens[1], end_time))
									throw std::string("Invalid time(s)");
								progress_formula = animate_tokens[2];
								tags = animate_tokens[3].substr(1, animate_tokens[3].size()-2);
								break;
						}
						this->parse_tags(tags, geometry_type, buffer_event);
						if(!buffer_event.static_tags)
							throw std::string("No animations in animations allowed");
						event.static_tags = false;
						ADD_OBJECT(SSB::Animate(start_time, end_time, progress_formula, buffer_event.objects));
					}catch(std::string error_message){
						THROW_WEAK_ERROR("Animation values incorrect: " + error_message);
					}
				}else
					THROW_WEAK_ERROR("Invalid animate");
			}else if(STR_LIT_EQU_FIRST(tags_token, "k=")){
				decltype(SSB::Karaoke::time) time;
				if(string_to_number(tags_token.substr(2), time)){
					event.static_tags = false;
					ADD_OBJECT(SSB::Karaoke(SSB::Karaoke::Type::DURATION, time));
				}else
					THROW_WEAK_ERROR("Invalid karaoke");
			}else if(STR_LIT_EQU_FIRST(tags_token, "ks=")){
				decltype(SSB::Karaoke::time) time;
				if(string_to_number(tags_token.substr(3), time)){
					event.static_tags = false;
					ADD_OBJECT(SSB::Karaoke(SSB::Karaoke::Type::SET, time));
				}else
					THROW_WEAK_ERROR("Invalid karaoke set");
			}else if(STR_LIT_EQU_FIRST(tags_token, "kc=")){
				unsigned long int rgb;
				if(hex_string_to_number(tags_token.substr(3), rgb) && rgb <= 0xffffff)
					ADD_OBJECT(SSB::KaraokeColor(
									static_cast<decltype(SSB::RGB::r)>(rgb >> 16) / 0xff,
									static_cast<decltype(SSB::RGB::g)>(rgb >> 8 & 0xff) / 0xff,
									static_cast<decltype(SSB::RGB::b)>(rgb & 0xff) / 0xff
								));
				else
					THROW_WEAK_ERROR("Invalid karaoke color");
			}else if(STR_LIT_EQU_FIRST(tags_token, "km=")){
				std::string tag_value = tags_token.substr(3);
				if(tag_value == "f")
					ADD_OBJECT(SSB::KaraokeMode(SSB::KaraokeMode::Mode::FILL));
				else if(tag_value == "s")
					ADD_OBJECT(SSB::KaraokeMode(SSB::KaraokeMode::Mode::SOLID));
				else if(tag_value == "g")
					ADD_OBJECT(SSB::KaraokeMode(SSB::KaraokeMode::Mode::GLOW));
				else
					THROW_WEAK_ERROR("Invalid karaoke mode");
			}else
				THROW_WEAK_ERROR("Invalid tag \"" + tags_token + '\"');
}

void SSB::Parser::parse_script(SSB::Data& data, std::istream& script) throw(SSB::Exception){
	// Skip UTF-8 byte-order-mask
	unsigned char BOM[3];
	script.read(reinterpret_cast<char*>(BOM), 3);
	if(!script.good())	// Reading BOM behind EOF shouldn't cause error flag
		script.clear();
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
	}catch(SSB::Exception e){
		// Rethrow error message with additional line number
		throw SSB::Exception(std::to_string(line_number) + ": " + e.what());
	}
}

void SSB::Parser::parse_line(SSB::Data& data, std::string& line) throw(SSB::Exception){
	// No empty or comment line = no skip
	if(!line.empty() && !STR_LIT_EQU_FIRST(line, "//")){
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
			else
				THROW_STRONG_ERROR("Invalid section name");
		}else	// Section content line
			switch(data.current_section){
				case SSB::Data::Section::META:
					if(STR_LIT_EQU_FIRST(line, "Title: "))
						data.meta.title = line.substr(7);
					else if(STR_LIT_EQU_FIRST(line, "Author: "))
						data.meta.author = line.substr(8);
					else if(STR_LIT_EQU_FIRST(line, "Description: "))
						data.meta.description = line.substr(13);
					else if(STR_LIT_EQU_FIRST(line, "Version: "))
						data.meta.version = line.substr(9);
					else
						THROW_STRONG_ERROR("Invalid meta field");
					break;
				case SSB::Data::Section::FRAME:
					if(STR_LIT_EQU_FIRST(line, "Width: ")){
						decltype(data.frame.width) width;
						if(string_to_number(line.substr(7), width))
							data.frame.width = width;
						else
							THROW_WEAK_ERROR("Invalid frame width");
					}else if(STR_LIT_EQU_FIRST(line, "Heigth: ")){
						decltype(data.frame.height) height;
						if(string_to_number(line.substr(8), height))
							data.frame.height = height;
						else
							THROW_WEAK_ERROR("Invalid frame height");
					}else
						THROW_STRONG_ERROR("Invalid frame field");
					break;
				case SSB::Data::Section::STYLES:{
						auto split_pos = line.find(": ");
						if(split_pos != std::string::npos)
							data.styles[line.substr(0, split_pos)] = line.substr(split_pos+2);
						else
							THROW_STRONG_ERROR("Invalid styles field");
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
							THROW_STRONG_ERROR("Couldn't find start time");
							break;
						}
						// Extract end time
						if(!std::getline(line_stream, line_token, '|') || !parse_time(line_token, event.end_ms)){
							THROW_STRONG_ERROR("Couldn't find end time");
							break;
						}
						// Check valid times
						if(event.start_ms < event.end_ms){
							THROW_WEAK_ERROR("Start time mustn't be after end time");
							break;
						}
						// Extract style name
						if(!std::getline(line_stream, line_token, '|')){
							THROW_STRONG_ERROR("Couldn't find style");
							break;
						}
						// Get style content for later insertion
						std::string style_content;
						if(!line_token.empty() && !data.styles.count(line_token)){
							THROW_WEAK_ERROR("Couldn't find style");
							break;
						}else
							style_content = data.styles[line_token];
						// Skip note
						if(!std::getline(line_stream, line_token, '|')){
							THROW_STRONG_ERROR("Couldn't find note");
							break;
						}
						// Extract text
						if(!line_stream.unget() || line_stream.get() != '|'){
							THROW_STRONG_ERROR("Couldn't find text");
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
							}else
								pos_start = pos_end + 2;
						}
						// Evaluate text tokens
						SSB::Geometry::Type geometry_type = SSB::Geometry::Type::TEXT;
						bool in_tags = false;
						pos_start = 0;
						do{
							// Evaluate tags
							if(in_tags){
								// Search tags end at closing bracket or cause error
								pos_end = text.find('}', pos_start);
								if(pos_end == std::string::npos){
									THROW_WEAK_ERROR("Tags closing brace not found");
									break;
								}
								// Parse single tags
								std::string tags = text.substr(pos_start, pos_end - pos_start);
								if(!tags.empty())
									this->parse_tags(tags, geometry_type, event);
							// Evaluate geometries
							}else{
								// Search geometry end at tags bracket (unescaped) or text end
								pos_end = find_non_escaped_character(text, '{', pos_start);
								if(pos_end == std::string::npos)
									pos_end = text.length();
								// Parse geometry by type
								std::string geometry = text.substr(pos_start, pos_end - pos_start);
								if(!geometry.empty())
									this->parse_geometry(geometry, geometry_type, event);
							}
							// Update for next token
							pos_start = pos_end + 1;
							in_tags = !in_tags;
						}while(pos_start < text.length());
						// Event complete -> commit to data
						data.events.push_back(std::move(event));
					}
					break;
				case SSB::Data::Section::NONE:
					THROW_STRONG_ERROR("No section set");
					break;
			}
	}
}
