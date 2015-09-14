/*
Project: SSBRenderer
File: Renderer.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "../graphics/gutils.hpp"

namespace Backend{
	// RGBA renderer on CPU or GPU (holding own image)
	class Renderer{
		private:
			// Resources by implementation
			void* data;
		public:
			// Setters
			Renderer();
			Renderer(unsigned width, unsigned height);
			void set_size(unsigned width, unsigned height);
			// Free resources
			~Renderer();
			// No copy&move (-> resources limitation)
			Renderer(const Renderer&) = delete;
			Renderer(Renderer&&) = delete;
			Renderer& operator=(const Renderer&) = delete;
			Renderer& operator=(Renderer&&) = delete;
			// Getters
			unsigned width();
			unsigned height();
			void copy_image(unsigned char* image, unsigned padding);
			// State
			void reset();

			void set_font(const std::string& family, float size, bool bold, bool italic, bool underline, bool strikeout, double spacing);
			void set_deform(const std::string& x_formula, const std::string& y_formula, double progress);
			void set_matrix(const GUtils::Matrix4x4d& matrix);
			enum class Mode{FILL, WIRE, BOXED};
			void set_mode(Mode mode);
			void set_fill_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
			void set_fill_color(unsigned char r0, unsigned char g0, unsigned char b0, unsigned char a0,
					unsigned char r1, unsigned char g1, unsigned char b1, unsigned char a1,
					unsigned char r2, unsigned char g2, unsigned char b2, unsigned char a2,
					unsigned char r3, unsigned char g3, unsigned char b3, unsigned char a3);
			void set_line_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
			void set_line_color(unsigned char r0, unsigned char g0, unsigned char b0, unsigned char a0,
					unsigned char r1, unsigned char g1, unsigned char b1, unsigned char a1,
					unsigned char r2, unsigned char g2, unsigned char b2, unsigned char a2,
					unsigned char r3, unsigned char g3, unsigned char b3, unsigned char a3);
                        void set_texture(const std::string& filename);
			void set_texture_offset(double x, double y);
			enum class TexWrap{CLAMP, REPEAT, MIRROR, FLOW};
			void set_texture_wrap(TexWrap wrap);
			void set_line_width(double width);
			enum class LineJoin{ROUND, BEVEL, MITER};
			void set_line_join(LineJoin join);
			enum class LineCap{ROUND, SQUARE, FLAT};
			void set_line_cap(LineCap cap);
			void set_line_dash(double offset, std::vector<double> dashes);
			void set_antialiasing(unsigned level);

			std::string get_font_family();
			float get_font_size();
			bool get_font_bold();
			bool get_font_italic();
			bool get_font_underline();
			bool get_font_strikeout();
			double get_font_spacing();
			std::string get_deform_x();
			std::string get_deform_y();
			double get_deform_progress();
			GUtils::Matrix4x4d get_matrix();
			Mode get_mode();
                        std::vector<double> get_fill_color();
                        std::vector<double> get_line_color();
			std::string get_texture();
			double get_texture_offset_x();
			double get_texture_offset_y();
                        TexWrap get_texture_wrap();
			double get_line_width();
			LineJoin get_line_join();
			LineCap get_line_cap();
			double get_line_dash_offset();
			std::vector<double> get_line_dash();
			unsigned get_antialiasing();
			// Font/text analyzation
			GUtils::Font::Metrics font_metrics();
			double text_width(const std::string& text);
			std::vector<GUtils::PathSegment> text_path(const std::string& text);
			// Processing
			void clear_image();
			void clear_stencil();
	};
}
