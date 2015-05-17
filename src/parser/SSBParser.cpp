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
#include "utils.hpp"
#include <algorithm>

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
void SSB::Parser::parse_geometry(std::string& geometry, SSB::Geometry::Type geometry_type, SSB::Event& event) throw(std::string){
	switch(geometry_type){
		case SSB::Geometry::Type::POINTS:{
				// Points buffer
				std::vector<SSB::Point> points;
				// Iterate through numbers
				std::istringstream points_stream(geometry);
				Point point;
				while(points_stream >> point.x)
					if(points_stream >> point.y)
						points.push_back(point);
					else if(this->level == SSB::Parser::Level::ALL)
						throw std::string("Points must have 2 numbers");
				// Check for successfull streaming end
				if((points_stream >> std::ws).eof())
					event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Points(points)));
				else if(this->level == SSB::Parser::Level::ALL)
					throw std::string("Points are invalid");
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
								else if(this->level == SSB::Parser::Level::ALL)
									throw std::string(segments[0].type == SSB::Path::SegmentType::MOVE_TO ? "Path (move) is invalid" : "Path (line) is invalid");
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
								}else if(this->level == SSB::Parser::Level::ALL)
									throw std::string("Path (curve) is invalid");
								break;
							case SSB::Path::SegmentType::ARC_TO:
								if(path_stream >> segments[0].point.x &&
									path_stream >> segments[0].point.y &&
									path_stream >> segments[1].angle){
									path.push_back(segments[0]);
									path.push_back(segments[1]);
								}else if(this->level == SSB::Parser::Level::ALL)
									throw std::string("Path (arc) is invalid");
								break;
							case SSB::Path::SegmentType::CLOSE:
								if(this->level == SSB::Parser::Level::ALL)
									throw std::string("Path (close) is invalid");
								break;
						}
					}
				// Successful collection of segments
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Path(path)));
			}
			break;
		case SSB::Geometry::Type::TEXT:{
				// Replace in string \t to 4 spaces
				string_replace(geometry, "\t", "    ");
				// Replace in string \n to real line breaks
				string_replace(geometry, "\\n", "\n");
				// Replace in string \{ to single {
				string_replace(geometry, "\\{", "{");
				// Insert SSBText as SSBObject to SSBEvent
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Text(geometry)));
			}
			break;
	}
}

void SSB::Parser::parse_tags(std::string& tags, SSB::Geometry::Type& geometry_type, SSB::Event& event) throw(std::string){
	// Iterate through tags
	std::istringstream tags_stream(tags);
	std::string tags_token;
	while(std::getline(tags_stream, tags_token, ';'))
		// Evaluate single tags
		if(tags_token.compare(0, 3, "ff=") == 0)
			event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::FontFamily(tags_token.substr(3))));
		else if(tags_token.compare(0, 4, "fst=") == 0){
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
				else if(this->level == SSB::Parser::Level::ALL)
					throw std::string("Invalid font style");
			event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::FontStyle(bold, italic, underline, strikeout)));
		}else if(tags_token.compare(0, 3, "fs=") == 0){
			decltype(SSB::FontSize::size) size;
			if(string_to_number(tags_token.substr(3), size) && size >= 0)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::FontSize(size)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid font size");
		}else if(tags_token.compare(0, 4, "fsp=") == 0){
			decltype(SSB::FontSpace::x) x, y;
			if(string_to_number(tags_token.substr(4), x, y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::FontSpace(x, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid font spaces");
		}else if(tags_token.compare(0, 5, "fsph=") == 0){
			decltype(SSB::FontSpace::x) x;
			if(string_to_number(tags_token.substr(5), x))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::FontSpace(SSB::FontSpace::Type::HORIZONTAL, x)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid horizontal font space");
		}else if(tags_token.compare(0, 5, "fspv=") == 0){
			decltype(SSB::FontSpace::y) y;
			if(string_to_number(tags_token.substr(5), y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::FontSpace(SSB::FontSpace::Type::VERTICAL, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid vertical font space");
		}else if(tags_token.compare(0, 3, "lw=") == 0){
			decltype(SSB::LineWidth::width) width;
			if(string_to_number(tags_token.substr(3), width) && width >= 0)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::LineWidth(width)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid line width");
		}else if(tags_token.compare(0, 4, "lst=") == 0){
			std::string tag_value = tags_token.substr(4);
			std::string::size_type pos;
			if((pos = tag_value.find(',')) != std::string::npos){
				std::string join_string = tag_value.substr(0, pos), cap_string = tag_value.substr(pos+1);
				SSB::LineStyle::Join join = SSB::LineStyle::Join::ROUND;
				if(join_string == "r")
					join = SSB::LineStyle::Join::ROUND;
				else if(join_string == "b")
					join = SSB::LineStyle::Join::BEVEL;
				else if(this->level == SSB::Parser::Level::ALL)
					throw std::string("Invalid line style join");
				SSB::LineStyle::Cap cap = SSB::LineStyle::Cap::ROUND;
				if(cap_string == "r")
					cap = SSB::LineStyle::Cap::ROUND;
				else if(cap_string == "f")
					cap = SSB::LineStyle::Cap::FLAT;
				else if(this->level == SSB::Parser::Level::ALL)
					throw std::string("Invalid line style cap");
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::LineStyle(join, cap)));
			}else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid line style");
		}else if(tags_token.compare(0, 3, "ld=") == 0){
			decltype(SSB::LineDash::offset) offset;
			std::istringstream dash_stream(tags_token.substr(3));
			std::string dash_token;
			if(std::getline(dash_stream, dash_token, ',') && string_to_number(dash_token, offset) && offset >= 0){
				decltype(SSB::LineDash::dashes) dashes;
				decltype(SSB::LineDash::offset) dash;
				while(std::getline(dash_stream, dash_token, ','))
					if(string_to_number(dash_token, dash) && dash >= 0)
						dashes.push_back(dash);
					else if(this->level == SSB::Parser::Level::ALL)
						throw std::string("Invalid line dash");
				if(static_cast<size_t>(std::count(dashes.begin(), dashes.end(), 0)) != dashes.size())
					event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::LineDash(offset, dashes)));
				else if(this->level == SSB::Parser::Level::ALL)
					throw std::string("Dashes must not be only 0");
			}else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid line dashes");
		}else if(tags_token.compare(0, 3, "gm=") == 0){
			std::string tag_value = tags_token.substr(3);
			if(tag_value == "pt")
				geometry_type = SSB::Geometry::Type::POINTS;
			else if(tag_value == "p")
				geometry_type = SSB::Geometry::Type::PATH;
			else if(tag_value == "t")
				geometry_type = SSB::Geometry::Type::TEXT;
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid geometry");
		}else if(tags_token.compare(0, 3, "md=") == 0){
			std::string tag_value = tags_token.substr(3);
			if(tag_value == "f")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Mode(SSB::Mode::Method::FILL)));
			else if(tag_value == "w")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Mode(SSB::Mode::Method::WIRE)));
			else if(tag_value == "b")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Mode(SSB::Mode::Method::BOXED)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid mode");
		}else if(tags_token.compare(0, 3, "df=") == 0){
			std::string tag_value = tags_token.substr(3);
			std::string::size_type pos;
			if((pos = tag_value.find(',')) != std::string::npos && tag_value.find(',', pos+1) == std::string::npos)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Deform(tag_value.substr(0, pos), tag_value.substr(pos+1))));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid deform");
		}else if(tags_token.compare(0, 4, "pos=") == 0){
			std::string tag_value = tags_token.substr(4);
			decltype(SSB::Position::x) x, y;
			constexpr decltype(x) max_pos = std::numeric_limits<decltype(x)>::max();
			if(tag_value.empty())
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Position(max_pos, max_pos)));
			else if(string_to_number(tag_value, x, y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Position(x, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid position");
		}else if(tags_token.compare(0, 3, "an=") == 0){
			std::string tag_value = tags_token.substr(3);
			if(tag_value.length() == 1 && tag_value[0] >= '1' && tag_value[0] <= '9')
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Align(static_cast<SSB::Align::Position>(tag_value[0] - '0'))));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid alignment");
		}else if(tags_token.compare(0, 3, "mg=") == 0){
			std::string tag_value = tags_token.substr(3);
			decltype(SSB::Margin::x) x, y;
			if(string_to_number(tag_value, x))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Margin(SSB::Margin::Type::BOTH, x)));
			else if(string_to_number(tag_value, x, y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Margin(x, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid margin");
		}else if(tags_token.compare(0, 4, "mgh=") == 0){
			decltype(SSB::Margin::x) x;
			if(string_to_number(tags_token.substr(4), x))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Margin(SSB::Margin::Type::HORIZONTAL, x)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid horizontal margin");
		}else if(tags_token.compare(0, 4, "mgv=") == 0){
			decltype(SSB::Margin::y) y;
			if(string_to_number(tags_token.substr(4), y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Margin(SSB::Margin::Type::VERTICAL, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid vertical margin");
		}else if(tags_token.compare(0, 4, "dir=") == 0){
			std::string tag_value = tags_token.substr(4);
			if(tag_value == "ltr")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Direction(SSB::Direction::Mode::LTR)));
			else if(tag_value == "rtl")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Direction(SSB::Direction::Mode::RTL)));
			else if(tag_value == "ttb")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Direction(SSB::Direction::Mode::TTB)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid direction");
		}else if(tags_token == "id")
			event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Identity()));
		else if(tags_token.compare(0, 3, "tl=") == 0){
			decltype(SSB::Translate::x) x, y;
			if(string_to_number(tags_token.substr(3), x, y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Translate(x, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid translation");
		}else if(tags_token.compare(0, 4, "tlx=") == 0){
			decltype(SSB::Translate::x) x;
			if(string_to_number(tags_token.substr(4), x))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Translate(SSB::Translate::Type::HORIZONTAL, x)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid horizontal translation");
		}else if(tags_token.compare(0, 4, "tly=") == 0){
			decltype(SSB::Translate::y) y;
			if(string_to_number(tags_token.substr(4), y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Translate(SSB::Translate::Type::VERTICAL, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid vertical translation");
		}else if(tags_token.compare(0, 3, "sc=") == 0){
			std::string tag_value = tags_token.substr(3);
			decltype(SSB::Scale::x) x, y;
			if(string_to_number(tag_value, x))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Scale(SSB::Scale::Type::BOTH, x)));
			else if(string_to_number(tag_value, x, y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Scale(x, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid scale");
		}else if(tags_token.compare(0, 4, "scx=") == 0){
			decltype(SSB::Scale::x) x;
			if(string_to_number(tags_token.substr(4), x))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Scale(SSB::Scale::Type::HORIZONTAL, x)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid horizontal scale");
		}else if(tags_token.compare(0, 4, "scy=") == 0){
			decltype(SSB::Scale::y) y;
			if(string_to_number(tags_token.substr(4), y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Scale(SSB::Scale::Type::VERTICAL, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid vertical scale");
		}else if(tags_token.compare(0, 4, "rxy=") == 0){
			decltype(SSB::Rotate::angle1) angle1, angle2;
			if(string_to_number(tags_token.substr(4), angle1, angle2))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Rotate(SSB::Rotate::Axis::XY, angle1, angle2)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid rotation on x axis");
		}else if(tags_token.compare(0, 4, "ryx=") == 0){
			decltype(SSB::Rotate::angle1) angle1, angle2;
			if(string_to_number(tags_token.substr(4), angle1, angle2))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Rotate(SSB::Rotate::Axis::YX, angle1, angle2)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid rotation on y axis");
		}else if(tags_token.compare(0, 3, "rz=") == 0){
			decltype(SSB::Rotate::angle1) angle;
			if(string_to_number(tags_token.substr(3), angle))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Rotate(angle)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid rotation on z axis");
		}else if(tags_token.compare(0, 3, "sh=") == 0){
			decltype(SSB::Shear::x) x, y;
			if(string_to_number(tags_token.substr(3), x, y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Shear(x, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid shear");
		}else if(tags_token.compare(0, 4, "shx=") == 0){
			decltype(SSB::Shear::x) x;
			if(string_to_number(tags_token.substr(4), x))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Shear(SSB::Shear::Type::HORIZONTAL, x)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid horizontal shear");
		}else if(tags_token.compare(0, 4, "shy") == 0){
			decltype(SSB::Shear::y) y;
			if(string_to_number(tags_token.substr(4), y))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Shear(SSB::Shear::Type::VERTICAL, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid vertical shear");
		}else if(tags_token.compare(0, 3, "tf=") == 0){
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
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Transform(xx, yx, xy, yy, x0, y0)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid transform");
		}else if(tags_token.compare(0, 3, "cl=") == 0){
			std::string tag_value = tags_token.substr(3);
			unsigned long rgb[4];
			if(hex_string_to_number(tag_value, rgb[0]) &&
				rgb[0] <= 0xffffff)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Color(
											static_cast<decltype(RGB::r)>(rgb[0] >> 16) / 0xff,
											static_cast<decltype(RGB::g)>(rgb[0] >> 8 & 0xff) / 0xff,
											static_cast<decltype(RGB::b)>(rgb[0] & 0xff) / 0xff
											)));
			else if(hex_string_to_number(tag_value, rgb[0], rgb[1]) &&
					rgb[0] <= 0xffffff && rgb[1] <= 0xffffff)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Color(
											static_cast<decltype(RGB::r)>(rgb[0] >> 16) / 0xff,
											static_cast<decltype(RGB::g)>(rgb[0] >> 8 & 0xff) / 0xff,
											static_cast<decltype(RGB::b)>(rgb[0] & 0xff) / 0xff,
											static_cast<decltype(RGB::r)>(rgb[1] >> 16) / 0xff,
											static_cast<decltype(RGB::g)>(rgb[1] >> 8 & 0xff) / 0xff,
											static_cast<decltype(RGB::b)>(rgb[1] & 0xff) / 0xff
											)));
			else if(hex_string_to_number(tag_value, rgb[0], rgb[1], rgb[2], rgb[3]) &&
					rgb[0] <= 0xffffff && rgb[1] <= 0xffffff && rgb[2] <= 0xffffff && rgb[3] <= 0xffffff)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Color(
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
											)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid color");
		}else if(tags_token.compare(0, 4, "lcl=") == 0){
			unsigned long rgb;
			if(hex_string_to_number(tags_token.substr(4), rgb) &&
					rgb <= 0xffffff)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::LineColor(
											static_cast<decltype(RGB::r)>(rgb >> 16) / 0xff,
											static_cast<decltype(RGB::g)>(rgb >> 8 & 0xff) / 0xff,
											static_cast<decltype(RGB::b)>(rgb & 0xff) / 0xff
											)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid line color");
		}else if(tags_token.compare(0, 3, "al=") == 0){
			std::string tag_value = tags_token.substr(3);
			unsigned short a[4];
			if(hex_string_to_number(tag_value, a[0]) &&
					a[0] <= 0xff)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Alpha(static_cast<decltype(RGB::r)>(a[0]) / 0xff)));
			else if(hex_string_to_number(tag_value, a[0], a[1]) &&
					a[0] <= 0xff && a[1] <= 0xff)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Alpha(
											static_cast<decltype(RGB::r)>(a[0]) / 0xff,
											static_cast<decltype(RGB::r)>(a[1]) / 0xff
											)));
			else if(hex_string_to_number(tag_value, a[0], a[1], a[2], a[3]) &&
					a[0] <= 0xff && a[1] <= 0xff && a[2] <= 0xff && a[3] <= 0xff)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Alpha(
											static_cast<decltype(RGB::r)>(a[0]) / 0xff,
											static_cast<decltype(RGB::r)>(a[1]) / 0xff,
											static_cast<decltype(RGB::r)>(a[2]) / 0xff,
											static_cast<decltype(RGB::r)>(a[3]) / 0xff
											)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid alpha");
		}else if(tags_token.compare(0, 4, "lal=") == 0){
			unsigned short a;
			if(hex_string_to_number(tags_token.substr(4), a) &&
					a <= 0xff)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::LineAlpha(static_cast<decltype(RGB::r)>(a) / 0xff)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid line alpha");
		}else if(tags_token.compare(0, 4, "tex=") == 0){
			event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Texture(tags_token.substr(4))));
		}else if(tags_token.compare(0, 5, "texf=") == 0){
			std::string tag_value = tags_token.substr(5);
			decltype(SSB::TexFill::x) x, y;
			std::string::size_type pos1, pos2;
			if((pos1 = tag_value.find(',')) != std::string::npos &&
					string_to_number(tag_value.substr(0, pos1), x) &&
					(pos2 = tag_value.find(',', pos1+1)) != std::string::npos &&
					string_to_number(tag_value.substr(pos1+1, pos2-(pos1+1)), y)){
				std::string wrap = tag_value.substr(pos2+1);
				if(wrap == "c")
					event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::TexFill(x, y, SSB::TexFill::WrapStyle::CLAMP)));
				else if(wrap == "r")
					event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::TexFill(x, y, SSB::TexFill::WrapStyle::REPEAT)));
				else if(wrap == "m")
					event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::TexFill(x, y, SSB::TexFill::WrapStyle::MIRROR)));
				else if(wrap == "f")
					event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::TexFill(x, y, SSB::TexFill::WrapStyle::FLOW)));
				else if(this->level == SSB::Parser::Level::ALL)
					throw std::string("Invalid texture filling wrap style");
			}else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid texture filling");
		}else if(tags_token.compare(0, 4, "bld=") == 0){
			std::string tag_value = tags_token.substr(4);
			if(tag_value == "over")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Blend(SSB::Blend::Mode::OVER)));
			else if(tag_value == "add")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Blend(SSB::Blend::Mode::ADDITION)));
			else if(tag_value == "sub")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Blend(SSB::Blend::Mode::SUBTRACT)));
			else if(tag_value == "mult")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Blend(SSB::Blend::Mode::MULTIPLY)));
			else if(tag_value == "scr")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Blend(SSB::Blend::Mode::SCREEN)));
			else if(tag_value == "diff")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Blend(SSB::Blend::Mode::DIFFERENCES)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid blending");
		}else if(tags_token.compare(0, 3, "bl=") == 0){
			std::string tag_value = tags_token.substr(3);
			decltype(SSB::Blur::x) x, y;
			if(string_to_number(tag_value, x) && x >= 0)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Blur(SSB::Blur::Type::BOTH, x)));
			else if(string_to_number(tag_value, x, y) && x >= 0 && y >= 0)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Blur(x, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid blur");
		}else if(tags_token.compare(0, 4, "blh=") == 0){
			decltype(SSB::Blur::x) x;
			if(string_to_number(tags_token.substr(4), x) && x >= 0)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Blur(SSB::Blur::Type::HORIZONTAL, x)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid horizontal blur");
		}else if(tags_token.compare(0, 4, "blv=") == 0){
			decltype(SSB::Blur::y) y;
			if(string_to_number(tags_token.substr(4), y) && y >= 0)
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Blur(SSB::Blur::Type::VERTICAL, y)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid vertical blur");
		}else if(tags_token.compare(0, 4, "stc=") == 0){
			std::string tag_value = tags_token.substr(4);
			if(tag_value == "off")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Stencil(SSB::Stencil::Mode::OFF)));
			else if(tag_value == "set")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Stencil(SSB::Stencil::Mode::SET)));
			else if(tag_value == "uset")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Stencil(SSB::Stencil::Mode::UNSET)));
			else if(tag_value == "in")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Stencil(SSB::Stencil::Mode::INSIDE)));
			else if(tag_value == "out")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Stencil(SSB::Stencil::Mode::OUTSIDE)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid stencil mode");
		}else if(tags_token.compare(0, 3, "aa=") == 0){
			std::string tag_value = tags_token.substr(3);
			if(tag_value == "on")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::AntiAliasing(true)));
			else if(tag_value == "off")
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::AntiAliasing(false)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid anti-aliasing mode");
		}else if(tags_token.compare(0, 4, "fad=") == 0){
			std::string tag_value = tags_token.substr(4);
			decltype(SSB::Fade::in) in, out;
			if(string_to_number(tag_value, in))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Fade(SSB::Fade::Type::BOTH, in)));
			else if(string_to_number(tag_value, in, out))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Fade(in, out)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid fade");
		}else if(tags_token.compare(0, 5, "fadi=") == 0){
			decltype(SSB::Fade::in) in;
			if(string_to_number(tags_token.substr(5), in))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Fade(SSB::Fade::Type::INFADE, in)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid infade");
		}else if(tags_token.compare(0, 5, "fado=") == 0){
			decltype(SSB::Fade::out) out;
			if(string_to_number(tags_token.substr(5), out))
				event.objects.push_back(std::shared_ptr<SSB::Object>(new SSB::Fade(SSB::Fade::Type::OUTFADE, out)));
			else if(this->level == SSB::Parser::Level::ALL)
				throw std::string("Invalid outfade");
		}


	// TODO


}

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
						SSB::Geometry::Type geometry_type = SSB::Geometry::Type::TEXT;
						bool in_tags = false;
						pos_start = 0;
						do{
							// Evaluate tags
							if(in_tags){
								// Search tags end at closing bracket or cause error
								pos_end = text.find('}', pos_start);
								if(pos_end == std::string::npos){
									if(this->level == SSB::Parser::Level::ALL)
										throw std::string("Tags closing brace not found");
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
