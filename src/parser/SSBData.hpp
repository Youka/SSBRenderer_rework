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

#include <exception>
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace SSB{
	// General SSB exception
	class Exception : public std::exception{
		private:
			std::string message;
		public:
			Exception(const std::string& message) : message(message){}
			const char* what() const noexcept override{return this->message.c_str();}
	};

	// Time range
	using Time = unsigned long;
	// Time difference
	using Duration = long;
	// Coordinate precision
	using Coord = double;
	// Color depth
	using Depth = float;

	// Point structure for geometries
	struct Point{Coord x,y;};
	// RGB structure for colors
	struct RGB{
		Depth r, g, b;
		RGB operator-(const RGB& value) const{
			return {this->r - value.r, this->g - value.g, this->b - value.b};
		}
		RGB operator*(const Depth value) const{
			return {this->r * value, this->g * value, this->b * value};
		}
		RGB& operator+=(const RGB& value){
			this->r += value.r;
			this->g += value.g;
			this->b += value.b;
			return *this;
		}
		bool operator==(const RGB& value) const{
			return this->r == value.r && this->g == value.g && this->b == value.b;
		}
	};

	// Base of any tag or geometry
	class Object{
		public:
			enum class Type : char{
				TAG,
				GEOMETRY
			} const type;
			virtual ~Object() = default;
		protected:
			Object(Type type) : type(type){}
	};

	// Base of any tag
	class Tag : public Object{
		public:
			enum class Type : char{
				FONT_FAMILY,
				FONT_STYLE,
				FONT_SIZE,
				FONT_SPACE,
				LINE_WIDTH,
				LINE_STYLE,
				LINE_DASH,
				MODE,
				DEFORM,
				POSITION,
				ALIGN,
				MARGIN,
				DIRECTION,
				IDENTITY,
				TRANSLATE,
				SCALE,
				ROTATE,
				SHEAR,
				TRANSFORM,
				COLOR,
				LINE_COLOR,
				ALPHA,
				LINE_ALPHA,
				TEXTURE,
				TEXFILL,
				BLEND,
				BLUR,
				STENCIL,
				ANTI_ALIASING,
				FADE,
				ANIMATE,
				KARAOKE,
				KARAOKE_COLOR,
				KARAOKE_MODE
			} const type;
		protected:
			Tag(Type type) : Object(Object::Type::TAG), type(type){}
	};

	// Base of any geometry
	class Geometry : public Object{
		public:
			enum class Type : char{
				POINTS,
				PATH,
				TEXT
			} const type;
		protected:
			Geometry(Type type) : Object(Object::Type::GEOMETRY), type(type){}
	};

	// Font family state
	class FontFamily : public Tag{
		public:
			std::string family;
			FontFamily(const std::string& family) : Tag(Tag::Type::FONT_FAMILY), family(family){}
	};

	// Font style state
	class FontStyle : public Tag{
		public:
			bool bold, italic, underline, strikeout;
			FontStyle(bool bold, bool italic, bool underline, bool strikeout) : Tag(Tag::Type::FONT_STYLE), bold(bold), italic(italic), underline(underline), strikeout(strikeout){}
	};

	// Font size state
	class FontSize : public Tag{
		public:
			float size;
			FontSize(float size) : Tag(Tag::Type::FONT_SIZE), size(size){}
	};

	// Font space state
	class FontSpace : public Tag{
		public:
			enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
			Coord x, y;
			FontSpace(Coord x, Coord y) : Tag(Tag::Type::FONT_SPACE), type(Type::BOTH), x(x), y(y){}
			FontSpace(Type type, Coord xy) : Tag(Tag::Type::FONT_SPACE), type(type){
				switch(type){
					case Type::HORIZONTAL: this->x = xy; break;
					case Type::VERTICAL: this->y = xy; break;
					case Type::BOTH: this->x = this->y = xy; break;
				}
			}
	};

	// Line width state
	class LineWidth : public Tag{
		public:
			Coord width;
			LineWidth(Coord width) : Tag(Tag::Type::LINE_WIDTH), width(width){}
	};

	// Line style state
	class LineStyle : public Tag{
		public:
			enum class Join : char{ROUND, BEVEL} join;
			enum class Cap : char{ROUND, FLAT} cap;
			LineStyle(Join join, Cap cap) : Tag(Tag::Type::LINE_STYLE), join(join), cap(cap){}
	};

	// Line dash pattern state
	class LineDash : public Tag{
		public:
			Coord offset;
			std::vector<Coord> dashes;
			LineDash(Coord offset, const std::vector<Coord>& dashes) : Tag(Tag::Type::LINE_DASH), offset(offset), dashes(dashes){}
	};

	// Painting mode state
	class Mode : public Tag{
		public:
			enum class Method : char{FILL, WIRE, BOXED} method;
			Mode(Method method) : Tag(Tag::Type::MODE), method(method){}
	};

	// Deforming state
	class Deform : public Tag{
		public:
			std::string formula_x, formula_y;
			Deform(const std::string& formula_x, const std::string& formula_y) : Tag(Tag::Type::DEFORM), formula_x(formula_x), formula_y(formula_y){}
	};

	// Position state
	class Position : public Tag{
		public:
			Coord x, y;  // 'Unset' in case of maximum values
			Position(Coord x, Coord y) : Tag(Tag::Type::POSITION), x(x), y(y){}
	};

	// Alignment state
	class Align : public Tag{
		public:
			enum Position : char{
				LEFT_BOTTOM = 1,
				CENTER_BOTTOM, // = 2
				RIGHT_BOTTOM, // = 3
				LEFT_MIDDLE, // = 4
				CENTER_MIDDLE, // = 5
				RIGHT_MIDDLE, // = 6
				LEFT_TOP, // = 7
				CENTER_TOP, // = 8
				RIGHT_TOP // = 9
			} pos;
			Align(Position pos) : Tag(Tag::Type::ALIGN), pos(pos){}
	};

	// Margin state
	class Margin : public Tag{
		public:
			enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
			Coord x, y;
			Margin(Coord x, Coord y) : Tag(Tag::Type::MARGIN), type(Type::BOTH), x(x), y(y){}
			Margin(Type type, Coord xy) : Tag(Tag::Type::MARGIN), type(type){
				switch(type){
					case Type::HORIZONTAL: this->x = xy; break;
					case Type::VERTICAL: this->y = xy; break;
					case Type::BOTH: this->x = this->y = xy; break;
				}
			}
	};

	// Direction state
	class Direction : public Tag{
		public:
			enum class Mode : char{LTR, TTB} mode;
			Direction(Mode mode) : Tag(Tag::Type::DIRECTION), mode(mode){}
	};

	// Identity state
	class Identity : public Tag{
		public:
			Identity() : Tag(Tag::Type::IDENTITY){}
	};

	// Translation state
	class Translate : public Tag{
		public:
			enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
			Coord x, y;
			Translate(Coord x, Coord y) : Tag(Tag::Type::TRANSLATE), type(Type::BOTH), x(x), y(y){}
			Translate(Type type, Coord xy) : Tag(Tag::Type::TRANSLATE), type(type){
				switch(type){
					case Type::HORIZONTAL: this->x = xy; break;
					case Type::VERTICAL: this->y = xy; break;
					case Type::BOTH: this->x = this->y = xy; break;
				}
			}
	};

	// Scale state
	class Scale : public Tag{
		public:
			enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
			double x, y;
			Scale(Coord x, Coord y) : Tag(Tag::Type::SCALE), type(Type::BOTH), x(x), y(y){}
			Scale(Type type, Coord xy) : Tag(Tag::Type::SCALE), type(type){
				switch(type){
					case Type::HORIZONTAL: this->x = xy; break;
					case Type::VERTICAL: this->y = xy; break;
					case Type::BOTH: this->x = this->y = xy; break;
				}
			}
	};

	// Rotation state
	class Rotate : public Tag{
		public:
			enum class Axis : char{XY, YX, Z} axis;
			double angle1, angle2;
			Rotate(Axis axis, double angle1, double angle2) : Tag(Tag::Type::ROTATE), axis(axis), angle1(angle1), angle2(angle2){}
			Rotate(double angle) : Tag(Tag::Type::ROTATE), axis(Axis::Z), angle1(angle){}
	};

	// Shear state
	class Shear : public Tag{
		public:
			enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
			double x, y;
			Shear(Coord x, Coord y) : Tag(Tag::Type::SHEAR), type(Type::BOTH), x(x), y(y){}
			Shear(Type type, Coord xy) : Tag(Tag::Type::SHEAR), type(type){
				switch(type){
					case Type::HORIZONTAL: this->x = xy; break;
					case Type::VERTICAL: this->y = xy; break;
					case Type::BOTH: this->x = this->y = xy; break;
				}
			}
	};

	// Transform state
	class Transform : public Tag{
		public:
			double xx, xy, xz, x0, yx, yy, yz, y0, zx, zy, zz, z0;
			Transform(double xx, double xy, double xz, double x0, double yx, double yy, double yz, double y0, double zx, double zy, double zz, double z0)
			: Tag(Tag::Type::TRANSFORM), xx(xx), xy(xy), xz(xz), x0(x0), yx(yx), yy(yy), yz(yz), y0(y0), zx(zx), zy(zy), zz(zz), z0(z0){}
	};

	// Color state
	class Color : public Tag{
		public:
			RGB colors[4];
			Color(Depth r, Depth g, Depth b) : Tag(Tag::Type::COLOR), colors({{r, g, b}, {r, g, b}, {r, g, b}, {r, g, b}}){}
			Color(Depth r0, Depth g0, Depth b0, Depth r1, Depth g1, Depth b1) : Tag(Tag::Type::COLOR), colors({{r0, g0, b0}, {r1, g1, b1}, {r1, g1, b1}, {r0, g0, b0}}){}
			Color(Depth r0, Depth g0, Depth b0, Depth r1, Depth g1, Depth b1, Depth r2, Depth g2, Depth b2, Depth r3, Depth g3, Depth b3) : Tag(Tag::Type::COLOR), colors({{r0, g0, b0}, {r1, g1, b1}, {r2, g2, b2}, {r3, g3, b3}}){}
	};
	class LineColor : public Tag{
		public:
			RGB color;
			LineColor(Depth r, Depth g, Depth b) : Tag(Tag::Type::LINE_COLOR), color({r, g, b}){}
	};

	// Alpha state
	class Alpha : public Tag{
		public:
			Depth alphas[4];
			Alpha(Depth a) : Tag(Tag::Type::ALPHA), alphas{a, a, a, a}{}
			Alpha(Depth a0, Depth a1) : Tag(Tag::Type::ALPHA), alphas{a0, a1, a1, a0}{}
			Alpha(Depth a0, Depth a1, Depth a2, Depth a3) : Tag(Tag::Type::ALPHA), alphas{a0, a1, a2, a3}{}
	};
	class LineAlpha : public Tag{
		public:
			Depth alpha;
			LineAlpha(Depth a) : Tag(Tag::Type::LINE_ALPHA), alpha(a){}
	};

	// Texture state
	class Texture : public Tag{
		public:
			std::string filename;
			Texture(const std::string& filename) : Tag(Tag::Type::TEXTURE), filename(filename){}
	};

	// Texture fill state
	class TexFill : public Tag{
		public:
			Coord x, y;
			enum class WrapStyle : char{CLAMP, REPEAT, MIRROR, FLOW} wrap;
			TexFill(Coord x, Coord y, WrapStyle wrap) : Tag(Tag::Type::TEXFILL), x(x), y(y), wrap(wrap){}
	};

	// Blend state
	class Blend : public Tag{
		public:
			enum class Mode : char{OVER, ADDITION, SUBTRACT, MULTIPLY, SCREEN, DIFFERENCES} mode;
			Blend(Mode mode) : Tag(Tag::Type::BLEND), mode(mode){}
	};

	// Blur state
	class Blur : public Tag{
		public:
			enum class Type : char{HORIZONTAL, VERTICAL, BOTH} type;
			Coord x, y;
			Blur(Coord x, Coord y) : Tag(Tag::Type::BLUR), type(Type::BOTH), x(x), y(y){}
			Blur(Type type, Coord xy) : Tag(Tag::Type::BLUR), type(type){
				switch(type){
					case Type::HORIZONTAL: this->x = xy; break;
					case Type::VERTICAL: this->y = xy; break;
					case Type::BOTH: this->x = this->y = xy; break;
				}
			}
	};

	// Stencil state
	class Stencil : public Tag{
		public:
			enum class Mode : char{OFF, SET, UNSET, INSIDE, OUTSIDE} mode;
			Stencil(Mode mode) : Tag(Tag::Type::STENCIL), mode(mode){}
	};

	// Anti-aliasing state
	class AntiAliasing : public Tag{
		public:
			bool status;
			AntiAliasing(bool status) : Tag(Tag::Type::ANTI_ALIASING), status(status){}
	};

	// Fade state
	class Fade : public Tag{
		public:
			enum class Type : char{INFADE, OUTFADE, BOTH} type;
			Time in, out;
			Fade(Time in, Time out) : Tag(Tag::Type::FADE), type(Type::BOTH), in(in), out(out){}
			Fade(Type type, Time inout) : Tag(Tag::Type::FADE), type(type){
				switch(type){
					case Type::INFADE: this->in = inout; break;
					case Type::OUTFADE: this->out = inout; break;
					case Type::BOTH: this->in = this->out = inout; break;
				}
			}
	};

	// Animation state
	class Animate : public Tag{
		public:
			Duration start, end; // 'Unset' in case of maximum values
			std::string progress_formula;   // 'Unset' in case of emtpiness
			std::vector<std::shared_ptr<Object>> objects;
			Animate(Duration start, Duration end, const std::string& progress_formula, const std::vector<std::shared_ptr<Object>>& objects) : Tag(Tag::Type::ANIMATE), start(start), end(end), progress_formula(progress_formula), objects(objects){}
	};

	// Karaoke time state
	class Karaoke : public Tag{
		public:
			enum class Type : char{DURATION, SET} type;
			Time time;
			Karaoke(Type type, Time time) : Tag(Tag::Type::KARAOKE), type(type), time(time){}
	};

	// Karaoke color state
	class KaraokeColor : public Tag{
		public:
			RGB color;
			KaraokeColor(Depth r, Depth g, Depth b) : Tag(Tag::Type::KARAOKE_COLOR), color({r, g, b}){}
	};

	// Karaoke filling mode
	class KaraokeMode : public Tag{
		public:
			enum class Mode : char{FILL, SOLID, GLOW} mode;
			KaraokeMode(Mode mode) : Tag(Tag::Type::KARAOKE_MODE), mode(mode){}
	};

	// Points geometry
	class Points : public Geometry{
		public:
			std::vector<Point> points;
			Points(const std::vector<Point>& points) : Geometry(Geometry::Type::POINTS), points(points){}
	};

	// Path geometry
	class Path : public Geometry{
		public:
			enum class SegmentType : char{MOVE_TO, LINE_TO, CURVE_TO, ARC_TO, CLOSE};
			struct Segment{
				SegmentType type;
				union{
					Point point;
					double angle;
				};
			};
			std::vector<Segment> segments;
			Path(const std::vector<Segment>& segments) : Geometry(Geometry::Type::PATH), segments(segments){}
	};

	// Text geometry
	class Text : public Geometry{
		public:
			std::string text;
			Text(const std::string& text) : Geometry(Geometry::Type::TEXT), text(text){}
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
		enum class Section : char{NONE, META, FRAME, STYLES, EVENTS} current_section = Section::NONE;
		Meta meta;
		Frame frame;
		std::map<std::string, std::string>/*Name, Content*/ styles;
		std::vector<Event> events;
	};
}	// namespace SSB
