/*
Project: SSBRenderer
File: gutils.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <type_traits>
#ifdef _WIN32
	#include <windef.h>
#else
	#include <cairo.h>
	#include <pango/pango-layout.h>
#endif
#include <string>
#include <vector>

namespace GUtils{
	class Matrix4x4d{
		private:
			// Raw matrix data
#ifdef __SSE2__
			std::aligned_storage<sizeof(double)<<4,
#ifdef __AVX__
				sizeof(double)<<2
#else
				sizeof(float)<<2
#endif
			>::type storage;	// MSVC doesn't support keyword 'alignas'
#else
			double storage[16];
#endif
			double* const matrix = reinterpret_cast<decltype(this->matrix)>(&this->storage);
		public:
			// Ctors, dtor & assignments (rule-of-five)
			Matrix4x4d();
			Matrix4x4d(const double* matrix);
			Matrix4x4d(double x11, double x12, double x13, double x14,
				double x21, double x22, double x23, double x24,
				double x31, double x32, double x33, double x34,
				double x41, double x42, double x43, double x44);
			Matrix4x4d(const Matrix4x4d& other);
			Matrix4x4d& operator=(const Matrix4x4d& other);
			Matrix4x4d(Matrix4x4d&& other) = delete;	// No movable resources
			Matrix4x4d& operator=(Matrix4x4d&& other) = delete;
			~Matrix4x4d() = default;
			// Raw data access
			double* data() const;
			double& operator[](unsigned index) const;
			// General transformations
			enum class Order{PREPEND, APPEND};
			Matrix4x4d& multiply(const Matrix4x4d& other, Order order = Order::PREPEND);
			double* transform2d(double* vec);
			double* transform3d(double* vec);
			double* transform4d(double* vec);
			// Unary operations
			Matrix4x4d& identity();
			bool invert();
			// Binary operations
			Matrix4x4d& translate(double x, double y, double z, Order order = Order::PREPEND);
			Matrix4x4d& scale(double x, double y, double z, Order order = Order::PREPEND);
			Matrix4x4d& rotate_x(double rad, Order order = Order::PREPEND);
			Matrix4x4d& rotate_y(double rad, Order order = Order::PREPEND);
			Matrix4x4d& rotate_z(double rad, Order order = Order::PREPEND);
	};

	void flip(unsigned char* data, const unsigned height, const unsigned stride);
	void flip(const unsigned char* src_data, const unsigned height, const unsigned stride, unsigned char* dst_data);

	enum class BlendOp{SOURCE, OVER, ADD, SUB, MUL, SCR, DIFF};
	bool blend(const unsigned char* src_data, unsigned src_width, unsigned src_height, const unsigned src_stride, const bool src_with_alpha,
		unsigned char* dst_data, const unsigned dst_width, const unsigned dst_height, const unsigned dst_stride, const bool dst_with_alpha,
		const int dst_x, const int dst_y, const BlendOp op);

	enum class ColorDepth{X1/* A */, X3/* RGB */, X4/* RGBA */};
	void blur(unsigned char* data, const unsigned width, const unsigned height, const unsigned stride, const ColorDepth depth,
		const float strength_h, const float strength_v);

	class Font{
		private:
#ifdef _WIN32
			HDC dc;
			HFONT font;
			HGDIOBJ old_font;
#else
			cairo_surface_t* surface;
			cairo_t* context;
			PangoLayout* layout;
#endif
		public:
			// Rule-of-five
			Font();
			Font(std::string family, float size = 12, bool bold = false, bool italic = false, bool underline = false, bool strikeout = false, bool rtl = false);
#ifdef _WIN32
			Font(std::wstring family, float size = 12, bool bold = false, bool italic = false, bool underline = false, bool strikeout = false, bool rtl = false);
#endif
			~Font();
			Font(const Font& other);
			Font& operator=(const Font& other);
			Font(Font&& other);
			Font& operator=(Font&& other);
			// Getters
			std::string get_family();
#ifdef _WIN32
			std::wstring get_family_unicode();
#endif
			float get_size();
			bool get_bold();
			bool get_italic();
			bool get_underline();
			bool get_strikeout();
			bool get_rtl();
			// Setters
			void set_family(std::string family);
#ifdef _WIN32
			void set_family(std::wstring family);
#endif
			void set_size(float size);
			void set_bold(bool bold);
			void set_italic(bool italic);
			void set_underline(bool underline);
			void set_strikeout(bool strikeout);
			void set_rtl(bool rtl);
			// Check state
			operator bool() const;
			// Font metrics
			struct Metrics{
				double height, ascent, descent, internal_leading, external_leading;
			};
			Metrics metrics();
			// Text width by extents (height from metrics)
			double text_width(std::string text);
#ifdef _WIN32
			double text_width(std::wstring text);
#endif
			// Text to graphical path
			struct PathSegment{
				enum class Type{MOVE, LINE, CURVE, CLOSE} type;
				double x, y;
			};
			std::vector<PathSegment> text_path(std::string text);
#ifdef _WIN32
			std::vector<PathSegment> text_path(std::wstring text);
#endif
	};
}
