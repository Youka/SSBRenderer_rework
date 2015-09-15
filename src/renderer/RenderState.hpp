/*
Project: SSBRenderer
File: RenderState.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "../renderer_backend/Renderer.hpp"
#include "../parser/SSBData.hpp"

#define DBL_MAX std::numeric_limits<double>::max()

namespace SSB{
	// Current state/properties for rendering operations (+ initial values)
	struct RenderState{
		// Font
                std::string font_family = "Arial";
                bool bold = false, italic = false, underline = false, strikeout = false;
                float font_size = 30;
                double font_space_h = 0, font_space_v = 0;
                // Line
                double line_width = 4;
		Backend::Renderer::LineJoin line_join = Backend::Renderer::LineJoin::ROUND;
		Backend::Renderer::LineCap line_cap = Backend::Renderer::LineCap::ROUND;
		double dash_offset = 0;
		std::vector<double> dashes;
		// Geometry
		Backend::Renderer::Mode mode = Backend::Renderer::Mode::FILL;
                std::string deform_x, deform_y;
                double deform_progress = 0;
                // Position
                double pos_x = DBL_MAX, pos_y = DBL_MAX;	// Maximal position = unset
		Align::Position align = Align::Position::CENTER_BOTTOM;
                double margin_h = 0, margin_v = 0;
		bool vertical = false;
		// Transformation
		GUtils::Matrix4x4d matrix;
		// Color
		std::vector<std::array<double,4>> fill_color = {{1,1,1,1}};
		std::array<double,4> line_color = {{0,0,0,1}};
                std::string texture_filename;
		double texture_x = 0, texture_y = 0;
		Backend::Renderer::TexWrap texture_wrap = Backend::Renderer::TexWrap::CLAMP;
		// Rastering
		Blend::Mode blend_mode = Blend::Mode::OVER;
		double blur_h = 0, blur_v = 0;
                Stencil::Mode stencil_mode = Stencil::Mode::OFF;
                bool anti_aliasing = true;
                // Fade
                double fade_in = 0, fade_out = 0;
                // Karaoke
                unsigned long karaoke_start = ULONG_MAX, karaoke_duration = 0;
                std::array<double,3> karaoke_color = {{1,0,0}};
                KaraokeMode::Mode karaoke_mode = KaraokeMode::Mode::FILL;

		// TODO

	};
}
