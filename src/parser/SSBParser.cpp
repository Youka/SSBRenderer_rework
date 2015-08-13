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
#include "../strings/basic.hpp"
#include <algorithm>

// Parses SSB time and converts to milliseconds
template<typename T>
static inline bool parse_time(const std::string& s, T& t){
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

namespace SSB{
	// Parser implementations
	#define ADD_OBJECT(obj) event.objects.emplace_back(new obj)
	#define THROW_STRONG_ERROR(msg) if(this->level != Parser::Level::OFF) throw Exception(msg)
	#define THROW_WEAK_ERROR(msg) if(this->level == Parser::Level::ALL) throw Exception(msg)
	void Parser::parse_geometry(std::string geometry, Geometry::Type geometry_type, Event& event) throw(Exception){
		switch(geometry_type){
			case Geometry::Type::POINTS:{
					// Points buffer
					std::vector<Point> points;
					// Iterate through numbers
					std::istringstream points_stream(geometry);
					Point point;
					while(points_stream >> point.x)
						if(points_stream >> point.y)
							points.push_back(point);
						else
							THROW_WEAK_ERROR("Points must have 2 numbers");
					// Check for successfull streaming end
					if((points_stream >> std::ws).eof())
						ADD_OBJECT(Points(points));
					else
						THROW_WEAK_ERROR("Points are invalid");
				}
				break;
			case Geometry::Type::PATH:{
					// Path segments buffer
					std::vector<Path::Segment> path;
					// Iterate through words
					std::istringstream path_stream(geometry);
					std::string path_token;
					Path::Segment segments[3];
					segments[0].type = Path::SegmentType::MOVE_TO;
					while(path_stream >> path_token)
						// Save next segment type
						if(path_token == "m")
							segments[0].type = Path::SegmentType::MOVE_TO;
						else if(path_token == "l")
							segments[0].type = Path::SegmentType::LINE_TO;
						else if(path_token == "b")
							segments[0].type = segments[1].type = segments[2].type = Path::SegmentType::CURVE_TO;
						else if(path_token == "a")
							segments[0].type = segments[1].type = Path::SegmentType::ARC_TO;
						else if(path_token == "c"){
							segments[0].type = Path::SegmentType::CLOSE;
							path.push_back({Path::SegmentType::CLOSE, 0, 0});
						// Complete next segment
						}else{
							// Put token back in stream for rereading
							path_stream.seekg(-static_cast<long>(path_token.length()), std::istringstream::cur);
							// Parse segment data
							switch(segments[0].type){
								case Path::SegmentType::MOVE_TO:
								case Path::SegmentType::LINE_TO:
									if(path_stream >> segments[0].point.x &&
										path_stream >> segments[0].point.y)
										path.push_back(segments[0]);
									else
										THROW_WEAK_ERROR(segments[0].type == Path::SegmentType::MOVE_TO ? "Path (move) is invalid" : "Path (line) is invalid");
									break;
								case Path::SegmentType::CURVE_TO:
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
								case Path::SegmentType::ARC_TO:
									if(path_stream >> segments[0].point.x &&
										path_stream >> segments[0].point.y &&
										path_stream >> segments[1].angle){
										path.push_back(segments[0]);
										path.push_back(segments[1]);
									}else
										THROW_WEAK_ERROR("Path (arc) is invalid");
									break;
								case Path::SegmentType::CLOSE:
									THROW_WEAK_ERROR("Path (close) is invalid");
									break;
							}
						}
					// Successful collection of segments
					ADD_OBJECT(Path(path));
				}
				break;
			case Geometry::Type::TEXT:{
					// Replace in string \t to 4 spaces
					stdex::string_replace(geometry, "\t", "    ");
					// Replace in string \n to real line breaks
					stdex::string_replace(geometry, "\\n", "\n");
					// Replace in string \{ to single {
					stdex::string_replace(geometry, "\\{", "{");
					// Insert Text as Object to event
					ADD_OBJECT(Text(geometry));
				}
				break;
		}
	}

	void Parser::parse_tags(const std::string& tags, Geometry::Type& geometry_type, Event& event) throw(Exception){
		// Stream for tags
		std::istringstream tags_stream(tags);
		// Registered tags + handler
		static const std::vector<std::pair<std::string, std::function<void(std::string)>>> tags_register = {
			{
				"ff=",
				[&event](std::string tag_values){
					ADD_OBJECT(FontFamily(tag_values));
				}
			},
			{
				"fst=",
				[&event,this](std::string tag_values){
					bool bold = false, italic = false, underline = false, strikeout = false;
					for(char c : tag_values)
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
					ADD_OBJECT(FontStyle(bold, italic, underline, strikeout));
				}
			},
			{
				"fs=",
				[&event,this](std::string tag_values){
					decltype(FontSize::size) size;
					if(stdex::string_to_number(tag_values, size) && size >= 0)
						ADD_OBJECT(FontSize(size));
					else
						THROW_WEAK_ERROR("Invalid font size");
				}
			},
			{
				"fsp=",
				[&event,this](std::string tag_values){
					decltype(FontSpace::x) x, y;
					if(stdex::string_to_number(tag_values, x, y))
						ADD_OBJECT(FontSpace(x, y));
					else
						THROW_WEAK_ERROR("Invalid font spaces");
				}
			},
			{
				"fsph=",
				[&event,this](std::string tag_values){
					decltype(FontSpace::x) x;
					if(stdex::string_to_number(tag_values, x))
						ADD_OBJECT(FontSpace(FontSpace::Type::HORIZONTAL, x));
					else
						THROW_WEAK_ERROR("Invalid horizontal font space");
				}
			},
			{
				"fspv=",
				[&event,this](std::string tag_values){
					decltype(FontSpace::y) y;
					if(stdex::string_to_number(tag_values, y))
						ADD_OBJECT(FontSpace(FontSpace::Type::VERTICAL, y));
					else
						THROW_WEAK_ERROR("Invalid vertical font space");
				}
			},
			{
				"lw=",
				[&event,this](std::string tag_values){
					decltype(LineWidth::width) width;
					if(stdex::string_to_number(tag_values, width) && width >= 0)
						ADD_OBJECT(LineWidth(width));
					else
						THROW_WEAK_ERROR("Invalid line width");
				}
			},
			{
				"lst=",
				[&event,this](std::string tag_values){
					std::string::size_type pos;
					if((pos = tag_values.find(',')) != std::string::npos){
						std::string join_string = tag_values.substr(0, pos), cap_string = tag_values.substr(pos+1);
						LineStyle::Join join = LineStyle::Join::ROUND;
						if(join_string == "r")
							join = LineStyle::Join::ROUND;
						else if(join_string == "b")
							join = LineStyle::Join::BEVEL;
						else
							THROW_WEAK_ERROR("Invalid line style join");
						LineStyle::Cap cap = LineStyle::Cap::ROUND;
						if(cap_string == "r")
							cap = LineStyle::Cap::ROUND;
						else if(cap_string == "f")
							cap = LineStyle::Cap::FLAT;
						else
							THROW_WEAK_ERROR("Invalid line style cap");
						ADD_OBJECT(LineStyle(join, cap));
					}else
						THROW_WEAK_ERROR("Invalid line style");
				}
			},
			{
				"ld=",
				[&event,this](std::string tag_values){
					decltype(LineDash::offset) offset;
					std::istringstream dash_stream(tag_values);
					std::string dash_token;
					if(std::getline(dash_stream, dash_token, ',') && stdex::string_to_number(dash_token, offset) && offset >= 0){
						decltype(LineDash::dashes) dashes;
						decltype(LineDash::offset) dash;
						while(std::getline(dash_stream, dash_token, ','))
							if(stdex::string_to_number(dash_token, dash) && dash >= 0)
								dashes.push_back(dash);
							else
								THROW_WEAK_ERROR("Invalid line dash");
						if(static_cast<size_t>(std::count(dashes.begin(), dashes.end(), 0)) != dashes.size())	// Not all dashes should be zero
							ADD_OBJECT(LineDash(offset, dashes));
						else
							THROW_WEAK_ERROR("Dashes must not be only 0");
					}else
						THROW_WEAK_ERROR("Invalid line dashes");
				}
			},
			{
				"gm=",
				[&event,this,&geometry_type](std::string tag_values){
					if(tag_values == "pt")
						geometry_type = Geometry::Type::POINTS;
					else if(tag_values == "p")
						geometry_type = Geometry::Type::PATH;
					else if(tag_values == "t")
						geometry_type = Geometry::Type::TEXT;
					else
						THROW_WEAK_ERROR("Invalid geometry");
				}
			},
			{
				"md=",
				[&event,this](std::string tag_values){
					if(tag_values == "f")
						ADD_OBJECT(Mode(Mode::Method::FILL));
					else if(tag_values == "w")
						ADD_OBJECT(Mode(Mode::Method::WIRE));
					else if(tag_values == "b")
						ADD_OBJECT(Mode(Mode::Method::BOXED));
					else
						THROW_WEAK_ERROR("Invalid mode");
				}
			},
			{
				"df=",
				[&event,this](std::string tag_values){
					std::string::size_type pos;
					if((pos = tag_values.find(',')) != std::string::npos && tag_values.find(',', pos+1) == std::string::npos)
						ADD_OBJECT(Deform(tag_values.substr(0, pos), tag_values.substr(pos+1)));
					else
						THROW_WEAK_ERROR("Invalid deform");
				}
			},
			{
				"pos=",
				[&event,this](std::string tag_values){
					decltype(Position::x) x, y;
					constexpr decltype(x) max_pos = std::numeric_limits<decltype(x)>::max();
					if(tag_values.empty())
						ADD_OBJECT(Position(max_pos, max_pos));
					else if(stdex::string_to_number(tag_values, x, y))
						ADD_OBJECT(Position(x, y));
					else
						THROW_WEAK_ERROR("Invalid position");
				}
			},
			{
				"an=",
				[&event,this](std::string tag_values){
					if(tag_values.length() == 1 && tag_values[0] >= '1' && tag_values[0] <= '9')
						ADD_OBJECT(Align(static_cast<Align::Position>(tag_values[0] - '0')));
					else
						THROW_WEAK_ERROR("Invalid alignment");
				}
			},
			{
				"mg=",
				[&event,this](std::string tag_values){
					decltype(Margin::x) x, y;
					if(stdex::string_to_number(tag_values, x))
						ADD_OBJECT(Margin(Margin::Type::BOTH, x));
					else if(stdex::string_to_number(tag_values, x, y))
						ADD_OBJECT(Margin(x, y));
					else
						THROW_WEAK_ERROR("Invalid margin");
				}
			},
			{
				"mgh=",
				[&event,this](std::string tag_values){
					decltype(Margin::x) x;
					if(stdex::string_to_number(tag_values, x))
						ADD_OBJECT(Margin(Margin::Type::HORIZONTAL, x));
					else
						THROW_WEAK_ERROR("Invalid horizontal margin");
				}
			},
			{
				"mgv=",
				[&event,this](std::string tag_values){
					decltype(Margin::y) y;
					if(stdex::string_to_number(tag_values, y))
						ADD_OBJECT(Margin(Margin::Type::VERTICAL, y));
					else
						THROW_WEAK_ERROR("Invalid vertical margin");
				}
			},
			{
				"dir=",
				[&event,this](std::string tag_values){
					if(tag_values == "ltr")
						ADD_OBJECT(Direction(Direction::Mode::LTR));
					else if(tag_values == "ttb")
						ADD_OBJECT(Direction(Direction::Mode::TTB));
					else
						THROW_WEAK_ERROR("Invalid direction");
				}
			},
			{
				"tl=",
				[&event,this](std::string tag_values){
					decltype(Translate::x) x, y;
					if(stdex::string_to_number(tag_values, x, y))
						ADD_OBJECT(Translate(x, y));
					else
						THROW_WEAK_ERROR("Invalid translation");
				}
			},
			{
				"tlx=",
				[&event,this](std::string tag_values){
					decltype(Translate::x) x;
					if(stdex::string_to_number(tag_values, x))
						ADD_OBJECT(Translate(Translate::Type::HORIZONTAL, x));
					else
						THROW_WEAK_ERROR("Invalid horizontal translation");
				}
			},
			{
				"tly=",
				[&event,this](std::string tag_values){
					decltype(Translate::y) y;
					if(stdex::string_to_number(tag_values, y))
						ADD_OBJECT(Translate(Translate::Type::VERTICAL, y));
					else
						THROW_WEAK_ERROR("Invalid vertical translation");
				}
			},
			{
				"sc=",
				[&event,this](std::string tag_values){
					decltype(Scale::x) x, y;
					if(stdex::string_to_number(tag_values, x))
						ADD_OBJECT(Scale(Scale::Type::BOTH, x));
					else if(stdex::string_to_number(tag_values, x, y))
						ADD_OBJECT(Scale(x, y));
					else
						THROW_WEAK_ERROR("Invalid scale");
				}
			},
			{
				"scx=",
				[&event,this](std::string tag_values){
					decltype(Scale::x) x;
					if(stdex::string_to_number(tag_values, x))
						ADD_OBJECT(Scale(Scale::Type::HORIZONTAL, x));
					else
						THROW_WEAK_ERROR("Invalid horizontal scale");
				}
			},
			{
				"scy=",
				[&event,this](std::string tag_values){
					decltype(Scale::y) y;
					if(stdex::string_to_number(tag_values, y))
						ADD_OBJECT(Scale(Scale::Type::VERTICAL, y));
					else
						THROW_WEAK_ERROR("Invalid vertical scale");
				}
			},
			{
				"rxy=",
				[&event,this](std::string tag_values){
					decltype(Rotate::angle1) angle1, angle2;
					if(stdex::string_to_number(tag_values, angle1, angle2))
						ADD_OBJECT(Rotate(Rotate::Axis::XY, angle1, angle2));
					else
						THROW_WEAK_ERROR("Invalid rotation on x axis");
				}
			},
			{
				"ryx=",
				[&event,this](std::string tag_values){
					decltype(Rotate::angle1) angle1, angle2;
					if(stdex::string_to_number(tag_values, angle1, angle2))
						ADD_OBJECT(Rotate(Rotate::Axis::YX, angle1, angle2));
					else
						THROW_WEAK_ERROR("Invalid rotation on y axis");
				}
			},
			{
				"rz=",
				[&event,this](std::string tag_values){
					decltype(Rotate::angle1) angle;
					if(stdex::string_to_number(tag_values, angle))
						ADD_OBJECT(Rotate(angle));
					else
						THROW_WEAK_ERROR("Invalid rotation on z axis");
				}
			},
			{
				"sh=",
				[&event,this](std::string tag_values){
					decltype(Shear::x) x, y;
					if(stdex::string_to_number(tag_values, x, y))
						ADD_OBJECT(Shear(x, y));
					else
						THROW_WEAK_ERROR("Invalid shear");
				}
			},
			{
				"shx=",
				[&event,this](std::string tag_values){
					decltype(Shear::x) x;
					if(stdex::string_to_number(tag_values, x))
						ADD_OBJECT(Shear(Shear::Type::HORIZONTAL, x));
					else
						THROW_WEAK_ERROR("Invalid horizontal shear");
				}
			},
			{
				"shy",
				[&event,this](std::string tag_values){
					decltype(Shear::y) y;
					if(stdex::string_to_number(tag_values, y))
						ADD_OBJECT(Shear(Shear::Type::VERTICAL, y));
					else
						THROW_WEAK_ERROR("Invalid vertical shear");
				}
			},
			{
				"tf=",
				[&event,this](std::string tag_values){
					decltype(Transform::xx) xx, xy, xz, x0, yx, yy, yz, y0, zx, zy, zz, z0;
					std::istringstream matrix_stream(tag_values);
					std::string matrix_token;
					if(std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, xx) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, xy) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, xz) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, x0) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, yx) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, yy) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, yz) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, y0) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, zx) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, zy) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, zz) &&
							std::getline(matrix_stream, matrix_token, ',') && stdex::string_to_number(matrix_token, z0) &&
							matrix_stream.eof()
						)
						ADD_OBJECT(Transform(xx, xy, xz, x0, yx, yy, yz, y0, zx, zy, zz, z0));
					else
						THROW_WEAK_ERROR("Invalid transform");
				}
			},
			{
				"cl=",
				[&event,this](std::string tag_values){
					unsigned long rgb[4];
					if(stdex::hex_string_to_number(tag_values, rgb[0]) &&
						rgb[0] <= 0xffffff)
						ADD_OBJECT(Color(
										static_cast<decltype(RGB::r)>(rgb[0] >> 16) / 0xff,
										static_cast<decltype(RGB::g)>(rgb[0] >> 8 & 0xff) / 0xff,
										static_cast<decltype(RGB::b)>(rgb[0] & 0xff) / 0xff
									));
					else if(stdex::hex_string_to_number(tag_values, rgb[0], rgb[1]) &&
							rgb[0] <= 0xffffff && rgb[1] <= 0xffffff)
						ADD_OBJECT(Color(
										static_cast<decltype(RGB::r)>(rgb[0] >> 16) / 0xff,
										static_cast<decltype(RGB::g)>(rgb[0] >> 8 & 0xff) / 0xff,
										static_cast<decltype(RGB::b)>(rgb[0] & 0xff) / 0xff,
										static_cast<decltype(RGB::r)>(rgb[1] >> 16) / 0xff,
										static_cast<decltype(RGB::g)>(rgb[1] >> 8 & 0xff) / 0xff,
										static_cast<decltype(RGB::b)>(rgb[1] & 0xff) / 0xff
									));
					else if(stdex::hex_string_to_number(tag_values, rgb[0], rgb[1], rgb[2], rgb[3]) &&
							rgb[0] <= 0xffffff && rgb[1] <= 0xffffff && rgb[2] <= 0xffffff && rgb[3] <= 0xffffff)
						ADD_OBJECT(Color(
										static_cast<decltype(RGB::r)>(rgb[0] >> 16) / 0xff,
										static_cast<decltype(RGB::g)>(rgb[0] >> 8 & 0xff) / 0xff,
										static_cast<decltype(RGB::b)>(rgb[0] & 0xff) / 0xff,
										static_cast<decltype(RGB::r)>(rgb[1] >> 16) / 0xff,
										static_cast<decltype(RGB::g)>(rgb[1] >> 8 & 0xff) / 0xff,
										static_cast<decltype(RGB::b)>(rgb[1] & 0xff) / 0xff,
										static_cast<decltype(RGB::r)>(rgb[2] >> 16) / 0xff,
										static_cast<decltype(RGB::g)>(rgb[2] >> 8 & 0xff) / 0xff,
										static_cast<decltype(RGB::b)>(rgb[2] & 0xff) / 0xff,
										static_cast<decltype(RGB::r)>(rgb[3] >> 16) / 0xff,
										static_cast<decltype(RGB::g)>(rgb[3] >> 8 & 0xff) / 0xff,
										static_cast<decltype(RGB::b)>(rgb[3] & 0xff) / 0xff
									));
					else
						THROW_WEAK_ERROR("Invalid color");
				}
			},
			{
				"lcl=",
				[&event,this](std::string tag_values){
					unsigned long rgb;
					if(stdex::hex_string_to_number(tag_values, rgb) &&
							rgb <= 0xffffff)
						ADD_OBJECT(LineColor(
										static_cast<decltype(RGB::r)>(rgb >> 16) / 0xff,
										static_cast<decltype(RGB::g)>(rgb >> 8 & 0xff) / 0xff,
										static_cast<decltype(RGB::b)>(rgb & 0xff) / 0xff
									));
					else
						THROW_WEAK_ERROR("Invalid line color");
				}
			},
			{
				"al=",
				[&event,this](std::string tag_values){
					unsigned short a[4];
					if(stdex::hex_string_to_number(tag_values, a[0]) &&
							a[0] <= 0xff)
						ADD_OBJECT(Alpha(static_cast<decltype(RGB::r)>(a[0]) / 0xff));
					else if(stdex::hex_string_to_number(tag_values, a[0], a[1]) &&
							a[0] <= 0xff && a[1] <= 0xff)
						ADD_OBJECT(Alpha(
										static_cast<decltype(RGB::r)>(a[0]) / 0xff,
										static_cast<decltype(RGB::r)>(a[1]) / 0xff
									));
					else if(stdex::hex_string_to_number(tag_values, a[0], a[1], a[2], a[3]) &&
							a[0] <= 0xff && a[1] <= 0xff && a[2] <= 0xff && a[3] <= 0xff)
						ADD_OBJECT(Alpha(
										static_cast<decltype(RGB::r)>(a[0]) / 0xff,
										static_cast<decltype(RGB::r)>(a[1]) / 0xff,
										static_cast<decltype(RGB::r)>(a[2]) / 0xff,
										static_cast<decltype(RGB::r)>(a[3]) / 0xff
									));
					else
						THROW_WEAK_ERROR("Invalid alpha");
				}
			},
			{
				"lal=",
				[&event,this](std::string tag_values){
					unsigned short a;
					if(stdex::hex_string_to_number(tag_values, a) &&
							a <= 0xff)
						ADD_OBJECT(LineAlpha(static_cast<decltype(RGB::r)>(a) / 0xff));
					else
						THROW_WEAK_ERROR("Invalid line alpha");
				}
			},
			{
				"tex=",
				[&event](std::string tag_values){
					ADD_OBJECT(Texture(tag_values));
				}
			},
			{
				"texf=",
				[&event,this](std::string tag_values){
					decltype(TexFill::x) x, y;
					std::string::size_type pos1, pos2;
					if((pos1 = tag_values.find(',')) != std::string::npos &&
							stdex::string_to_number(tag_values.substr(0, pos1), x) &&
							(pos2 = tag_values.find(',', pos1+1)) != std::string::npos &&
							stdex::string_to_number(tag_values.substr(pos1+1, pos2-(pos1+1)), y)){
						std::string wrap = tag_values.substr(pos2+1);
						if(wrap == "c")
							ADD_OBJECT(TexFill(x, y, TexFill::WrapStyle::CLAMP));
						else if(wrap == "r")
							ADD_OBJECT(TexFill(x, y, TexFill::WrapStyle::REPEAT));
						else if(wrap == "m")
							ADD_OBJECT(TexFill(x, y, TexFill::WrapStyle::MIRROR));
						else if(wrap == "f")
							ADD_OBJECT(TexFill(x, y, TexFill::WrapStyle::FLOW));
						else
							THROW_WEAK_ERROR("Invalid texture filling wrap style");
					}else
						THROW_WEAK_ERROR("Invalid texture filling");
				}
			},
			{
				"bld=",
				[&event,this](std::string tag_values){
					if(tag_values == "over")
						ADD_OBJECT(Blend(Blend::Mode::OVER));
					else if(tag_values == "add")
						ADD_OBJECT(Blend(Blend::Mode::ADDITION));
					else if(tag_values == "sub")
						ADD_OBJECT(Blend(Blend::Mode::SUBTRACT));
					else if(tag_values == "mult")
						ADD_OBJECT(Blend(Blend::Mode::MULTIPLY));
					else if(tag_values == "scr")
						ADD_OBJECT(Blend(Blend::Mode::SCREEN));
					else if(tag_values == "diff")
						ADD_OBJECT(Blend(Blend::Mode::DIFFERENCES));
					else
						THROW_WEAK_ERROR("Invalid blending");
				}
			},
			{
				"bl=",
				[&event,this](std::string tag_values){
					decltype(Blur::x) x, y;
					if(stdex::string_to_number(tag_values, x) && x >= 0)
						ADD_OBJECT(Blur(Blur::Type::BOTH, x));
					else if(stdex::string_to_number(tag_values, x, y) && x >= 0 && y >= 0)
						ADD_OBJECT(Blur(x, y));
					else
						THROW_WEAK_ERROR("Invalid blur");
				}
			},
			{
				"blh=",
				[&event,this](std::string tag_values){
					decltype(Blur::x) x;
					if(stdex::string_to_number(tag_values, x) && x >= 0)
						ADD_OBJECT(Blur(Blur::Type::HORIZONTAL, x));
					else
						THROW_WEAK_ERROR("Invalid horizontal blur");
				}
			},
			{
				"blv=",
				[&event,this](std::string tag_values){
					decltype(Blur::y) y;
					if(stdex::string_to_number(tag_values, y) && y >= 0)
						ADD_OBJECT(Blur(Blur::Type::VERTICAL, y));
					else
						THROW_WEAK_ERROR("Invalid vertical blur");
				}
			},
			{
				"stc=",
				[&event,this](std::string tag_values){
					if(tag_values == "off")
						ADD_OBJECT(Stencil(Stencil::Mode::OFF));
					else if(tag_values == "set")
						ADD_OBJECT(Stencil(Stencil::Mode::SET));
					else if(tag_values == "uset")
						ADD_OBJECT(Stencil(Stencil::Mode::UNSET));
					else if(tag_values == "in")
						ADD_OBJECT(Stencil(Stencil::Mode::INSIDE));
					else if(tag_values == "out")
						ADD_OBJECT(Stencil(Stencil::Mode::OUTSIDE));
					else
						THROW_WEAK_ERROR("Invalid stencil mode");
				}
			},
			{
				"aa=",
				[&event,this](std::string tag_values){
					if(tag_values == "on")
						ADD_OBJECT(AntiAliasing(true));
					else if(tag_values == "off")
						ADD_OBJECT(AntiAliasing(false));
					else
						THROW_WEAK_ERROR("Invalid anti-aliasing mode");
				}
			},
			{
				"fad=",
				[&event,this](std::string tag_values){
					decltype(Fade::in) in, out;
					if(stdex::string_to_number(tag_values, in))
						ADD_OBJECT(Fade(Fade::Type::BOTH, in));
					else if(stdex::string_to_number(tag_values, in, out))
						ADD_OBJECT(Fade(in, out));
					else
						THROW_WEAK_ERROR("Invalid fade");
				}
			},
			{
				"fadi=",
				[&event,this](std::string tag_values){
					decltype(Fade::in) in;
					if(stdex::string_to_number(tag_values, in))
						ADD_OBJECT(Fade(Fade::Type::INFADE, in));
					else
						THROW_WEAK_ERROR("Invalid infade");
				}
			},
			{
				"fado=",
				[&event,this](std::string tag_values){
					decltype(Fade::out) out;
					if(stdex::string_to_number(tag_values, out))
						ADD_OBJECT(Fade(Fade::Type::OUTFADE, out));
					else
						THROW_WEAK_ERROR("Invalid outfade");
				}
			},
			{
				"ani=",
				[&event,this,&tags_stream,&geometry_type](std::string tag_values){
					// Collect animation tokens (maximum: 4)
					std::vector<std::string> animate_tokens;
					std::istringstream animate_stream(tag_values);
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
							while(animate_token.back() != ')' && std::getline(tags_stream, tag_values, ';'))
								animate_token += ';' + tag_values;
							animate_tokens.push_back(std::move(animate_token));
							// Finish collecting after last possible token
							break;
						// Get first tokens (times & formula)
						}else
							animate_tokens.push_back(std::move(animate_token));
					// Check for enough animation tokens and last token for brackets
					if(animate_tokens.size() > 0 && animate_tokens.back().length() >= 2 && animate_tokens.back().front() == '(' && animate_tokens.back().back() == ')'){
						// Get animation values
						constexpr decltype(Animate::start) max_duration = std::numeric_limits<decltype(Animate::start)>::max();
						decltype(Animate::start) start_time = max_duration, end_time = max_duration;
						std::string progress_formula, tags;
						Event buffer_event;
						try{
							switch(animate_tokens.size()){
								case 1: tags = animate_tokens[0].substr(1, animate_tokens[0].size()-2);
									break;
								case 2: progress_formula = animate_tokens[0];
									tags = animate_tokens[1].substr(1, animate_tokens[1].size()-2);
									break;
								case 3: if(!stdex::string_to_number(animate_tokens[0], start_time) || !stdex::string_to_number(animate_tokens[1], end_time))
										throw std::string("Invalid time(s)");
									tags = animate_tokens[2].substr(1, animate_tokens[2].size()-2);
									break;
								case 4: if(!stdex::string_to_number(animate_tokens[0], start_time) || !stdex::string_to_number(animate_tokens[1], end_time))
										throw std::string("Invalid time(s)");
									progress_formula = animate_tokens[2];
									tags = animate_tokens[3].substr(1, animate_tokens[3].size()-2);
									break;
							}
							this->parse_tags(tags, geometry_type, buffer_event);
							if(!buffer_event.static_tags)
								throw std::string("No animations in animations allowed");
							event.static_tags = false;
							ADD_OBJECT(Animate(start_time, end_time, progress_formula, buffer_event.objects));
						}catch(std::string error_message){
							THROW_WEAK_ERROR("Animation values incorrect: " + error_message);
						}
					}else
						THROW_WEAK_ERROR("Invalid animate");
				}
			},
			{
				"k=",
				[&event,this](std::string tag_values){
					decltype(Karaoke::time) time;
					if(stdex::string_to_number(tag_values, time)){
						event.static_tags = false;
						ADD_OBJECT(Karaoke(Karaoke::Type::DURATION, time));
					}else
						THROW_WEAK_ERROR("Invalid karaoke");
				}
			},
			{
				"ks=",
				[&event,this](std::string tag_values){
					decltype(Karaoke::time) time;
					if(stdex::string_to_number(tag_values, time)){
						event.static_tags = false;
						ADD_OBJECT(Karaoke(Karaoke::Type::SET, time));
					}else
						THROW_WEAK_ERROR("Invalid karaoke set");
				}
			},
			{
				"kc=",
				[&event,this](std::string tag_values){
					unsigned long int rgb;
					if(stdex::hex_string_to_number(tag_values, rgb) && rgb <= 0xffffff)
						ADD_OBJECT(KaraokeColor(
										static_cast<decltype(RGB::r)>(rgb >> 16) / 0xff,
										static_cast<decltype(RGB::g)>(rgb >> 8 & 0xff) / 0xff,
										static_cast<decltype(RGB::b)>(rgb & 0xff) / 0xff
									));
					else
						THROW_WEAK_ERROR("Invalid karaoke color");
				}
			},
			{
				"km=",
				[&event,this](std::string tag_values){
					if(tag_values == "f")
						ADD_OBJECT(KaraokeMode(KaraokeMode::Mode::FILL));
					else if(tag_values == "s")
						ADD_OBJECT(KaraokeMode(KaraokeMode::Mode::SOLID));
					else if(tag_values == "g")
						ADD_OBJECT(KaraokeMode(KaraokeMode::Mode::GLOW));
					else
						THROW_WEAK_ERROR("Invalid karaoke mode");
				}
			}
		};
		// Iterate through tags
		std::string tags_token;
		while(std::getline(tags_stream, tags_token, ';')){
			// Evaluate single tags
			for(auto& tag_entry : tags_register)
				if(tags_token.compare(0, tag_entry.first.length(), tag_entry.first) == 0){
					tag_entry.second(tags_token.substr(tag_entry.first.length()));
					goto TAG_FOUND;
				}
			THROW_WEAK_ERROR("Invalid tag \"" + tags_token + '\"');
			TAG_FOUND:
				continue;	// Statement expected after label
		}
	}

	void Parser::parse_script(Data& data, std::istream& script) throw(Exception){
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
		}catch(Exception e){
			// Rethrow error message with additional line number
			throw Exception(std::to_string(line_number) + ": " + e.what());
		}
	}

	#define STR_LIT_EQU_FIRST(s, s2) (s.compare(0, sizeof(s2)-1, s2) == 0)
	void Parser::parse_line(Data& data, const std::string& line) throw(Exception){
		// No empty or comment line = no skip
		if(!line.empty() && !STR_LIT_EQU_FIRST(line, "//")){
			// Section header line
			if(line.front() == '#'){
				std::string section = line.substr(1);
				if(section == "META")
					data.current_section = Data::Section::META;
				else if(section == "FRAME")
					data.current_section = Data::Section::FRAME;
				else if(section == "STYLES")
					data.current_section = Data::Section::STYLES;
				else if(section == "EVENTS")
					data.current_section = Data::Section::EVENTS;
				else
					THROW_STRONG_ERROR("Invalid section name");
			}else	// Section content line
				switch(data.current_section){
					case Data::Section::META:
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
					case Data::Section::FRAME:
						if(STR_LIT_EQU_FIRST(line, "Width: ")){
							decltype(data.frame.width) width;
							if(stdex::string_to_number(line.substr(7), width))
								data.frame.width = width;
							else
								THROW_WEAK_ERROR("Invalid frame width");
						}else if(STR_LIT_EQU_FIRST(line, "Heigth: ")){
							decltype(data.frame.height) height;
							if(stdex::string_to_number(line.substr(8), height))
								data.frame.height = height;
							else
								THROW_WEAK_ERROR("Invalid frame height");
						}else
							THROW_STRONG_ERROR("Invalid frame field");
						break;
					case Data::Section::STYLES:{
							auto split_pos = line.find(": ");
							if(split_pos != std::string::npos)
								data.styles[line.substr(0, split_pos)] = line.substr(split_pos+2);
							else
								THROW_STRONG_ERROR("Invalid styles field");
						}
						break;
					case Data::Section::EVENTS:{
							// Temporary event buffer
							Event event;
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
							Geometry::Type geometry_type = Geometry::Type::TEXT;
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
									pos_end = stdex::find_non_escaped_character(text, '{', pos_start);
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
					case Data::Section::NONE:
						THROW_STRONG_ERROR("No section set");
						break;
				}
		}
	}
}
