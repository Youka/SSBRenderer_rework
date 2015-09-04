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
#include <exception>
#include <vector>
#include <algorithm>

namespace GUtils{
	// 4x4 double-precision floating point matrix for transformations
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
			Matrix4x4d& shear_x(double y, double z, Order order = Order::PREPEND);
			Matrix4x4d& shear_y(double x, double z, Order order = Order::PREPEND);
			Matrix4x4d& shear_z(double x, double y, Order order = Order::PREPEND);
	};

	// Simple 2D image container
	template<typename Format = char>
	class Image2D{
		private:
			// Image header
			unsigned width, height, stride;
			Format format;
			// Image data
			std::vector<unsigned char> data;
		public:
			// Ctors
			Image2D() : width(0), height(0), stride(0), format(Format()), data(0){}
			Image2D(unsigned width, unsigned height, unsigned stride, Format format = Format())
			: width(width), height(height), stride(stride), format(format), data(this->height * this->stride){}
			Image2D(unsigned width, unsigned height, unsigned stride, Format format, const unsigned char* data)
			: width(width), height(height), stride(stride), format(format), data(data, data + this->height * this->stride){}
			// Getters
			unsigned get_width() const{return this->width;}
			unsigned get_height() const{return this->height;}
			unsigned get_stride() const{return this->stride;}
			Format get_format() const{return this->format;}
			unsigned char* get_data() const{return const_cast<unsigned char*>(this->data.data());}
			size_t get_size() const{return this->data.size();}
			// Setters
			void set_width(unsigned width){
				this->width = width;
			}
			void set_height(unsigned height){
				this->height = height,
				this->data.resize(height * this->stride);
			}
			void set_stride(unsigned stride){
				decltype(this->data) new_data(this->height * stride);
				for(auto iter = this->data.begin(), new_iter = new_data.begin(); iter != this->data.end(); iter += this->stride, new_iter += stride)
					std::copy(iter, iter + stride, new_iter);
				this->stride = stride,
				this->data = std::move(new_data);
			}
			void set_format(Format format){
				this->format = format;
			}
			void set_data(const unsigned char* data){
				this->data.assign(data, data + this->data.size());
			}
	};

	// Flip image vertically
	void flip(unsigned char* data, const unsigned height, const unsigned stride);
	void flip(const unsigned char* src_data, const unsigned height, const unsigned stride, unsigned char* dst_data);

	// Blend one image anywhere on another with specific operation
	enum class BlendOp{SOURCE, OVER, ADD, SUB, MUL, SCR, DIFF};
	bool blend(const unsigned char* src_data, unsigned src_width, unsigned src_height, const unsigned src_stride, const bool src_with_alpha,
		unsigned char* dst_data, const unsigned dst_width, const unsigned dst_height, const unsigned dst_stride, const bool dst_with_alpha,
		const int dst_x, const int dst_y, const BlendOp op);

	// Gaussian blur on image
	enum class ColorDepth{X1/* A */, X3/* RGB */, X4/* RGBA */};
	void blur(unsigned char* data, const unsigned width, const unsigned height, const unsigned stride, const ColorDepth depth,
		const float strength_h, const float strength_v);

	// Path processing
	struct PathSegment{
		enum class Type{MOVE, LINE, CURVE/*Cubic bezier*/, CLOSE} type;
		double x, y;
	};
	bool path_extents(const std::vector<PathSegment>& path, double* x0, double* y0, double* x1, double* y1);
	void path_close(std::vector<PathSegment>& path);
	void path_flatten(std::vector<PathSegment>& path, double tolerance);
	std::vector<PathSegment> path_by_arc(double x, double y, double cx, double cy, double angle);

	// Exception for font problems (see Font class below)
	class FontException : public std::exception{
		private:
			std::string message;
		public:
			FontException(const std::string& message) : message(message){}
			const char* what() const noexcept override{return this->message.c_str();}
	};

	// Native font wrapper
	class Font{
		private:
#ifdef _WIN32
			HDC dc;
			HFONT font;
			HGDIOBJ old_font;
			double spacing;
#else
			cairo_surface_t* surface;
			cairo_t* context;
			PangoLayout* layout;
#endif
			void copy(const Font& other);
			void move(Font&& other);
		public:
			// Rule-of-five
			Font();
			Font(const std::string& family, float size = 12, bool bold = false, bool italic = false, bool underline = false, bool strikeout = false, double spacing = 0.0, bool rtl = false) throw(FontException);
#ifdef _WIN32
			Font(const std::wstring& family, float size = 12, bool bold = false, bool italic = false, bool underline = false, bool strikeout = false, double spacing = 0.0, bool rtl = false) throw(FontException);
#endif
			~Font();
			Font(const Font& other);
			Font& operator=(const Font& other);
			Font(Font&& other);
			Font& operator=(Font&& other);
			// Check state
			operator bool() const;
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
			double get_spacing();
			bool get_rtl();
			// Font metrics
			struct Metrics{
				double height, ascent, descent, internal_leading, external_leading;
			};
			Metrics metrics();
			// Text width by extents (height from metrics)
			double text_width(const std::string& text);
#ifdef _WIN32
			double text_width(const std::wstring& text);
#endif
			// Text to graphical path
			std::vector<PathSegment> text_path(const std::string& text) throw(FontException);
#ifdef _WIN32
			std::vector<PathSegment> text_path(const std::wstring& text) throw(FontException);
#endif
	};
}
