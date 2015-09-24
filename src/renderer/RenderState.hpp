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
#define DEG_TO_RAD(x) (x * M_PI / 180.0)

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
	};

	// Tag to state
	static inline void state_update(RenderState& state, const Tag* const tag,
					Time start_ms, Duration inner_ms){
		switch(tag->type){
			case Tag::Type::FONT_FAMILY:
				state.font_family = dynamic_cast<const FontFamily*>(tag)->family;
				break;
			case Tag::Type::FONT_STYLE:
				{
					const FontStyle* style = dynamic_cast<const FontStyle*>(tag);
					state.bold = style->bold,
					state.italic = style->italic,
					state.underline = style->underline,
					state.strikeout = style->strikeout;
				}
				break;
			case Tag::Type::FONT_SIZE:
				state.font_size = dynamic_cast<const FontSize*>(tag)->size;
				break;
			case Tag::Type::FONT_SPACE:
				{
					const FontSpace* space = dynamic_cast<const FontSpace*>(tag);
					switch(space->type){
						case FontSpace::Type::HORIZONTAL:
							state.font_space_h = space->x;
							break;
						case FontSpace::Type::VERTICAL:
							state.font_space_v = space->y;
							break;
						case FontSpace::Type::BOTH:
							state.font_space_h = space->x,
							state.font_space_v = space->y;
							break;
					}
				}
				break;
			case Tag::Type::LINE_WIDTH:
				state.line_width = dynamic_cast<const LineWidth*>(tag)->width;
				break;
			case Tag::Type::LINE_STYLE:
				{
					const LineStyle* style = dynamic_cast<const LineStyle*>(tag);
					switch(style->join){
						case LineStyle::Join::BEVEL:
							state.line_join = Backend::Renderer::LineJoin::BEVEL;
                                                        break;
						case LineStyle::Join::ROUND:
							state.line_join = Backend::Renderer::LineJoin::ROUND;
							break;
					}
					switch(style->cap){
						case LineStyle::Cap::FLAT:
							state.line_cap = Backend::Renderer::LineCap::FLAT;
							break;
						case LineStyle::Cap::ROUND:
							state.line_cap = Backend::Renderer::LineCap::ROUND;
							break;
					}
				}
				break;
			case Tag::Type::LINE_DASH:
                                {
					const LineDash* dash = dynamic_cast<const LineDash*>(tag);
					state.dash_offset = dash->offset,
                                        state.dashes = dash->dashes;
                                }
				break;
			case Tag::Type::MODE:
				switch(dynamic_cast<const Mode*>(tag)->method){
					case Mode::Method::FILL:
						state.mode = Backend::Renderer::Mode::FILL;
						break;
					case Mode::Method::WIRE:
						state.mode = Backend::Renderer::Mode::WIRE;
						break;
					case Mode::Method::BOXED:
						state.mode = Backend::Renderer::Mode::BOXED;
						break;
				}
				break;
			case Tag::Type::DEFORM:
				{
					const Deform* formula = dynamic_cast<const Deform*>(tag);
					state.deform_x = formula->formula_x,
					state.deform_y = formula->formula_y,
					state.deform_progress = 0;
				}
				break;
			case Tag::Type::POSITION:
				{
					const Position* position = dynamic_cast<const Position*>(tag);
					state.pos_x = position->x,
					state.pos_y = position->y;
				}
				break;
			case Tag::Type::ALIGN:
				state.align = dynamic_cast<const Align*>(tag)->pos;
				break;
			case Tag::Type::MARGIN:
				{
					const Margin* margins = dynamic_cast<const Margin*>(tag);
					switch(margins->type){
						case Margin::Type::HORIZONTAL:
							state.margin_h = margins->x;
							break;
						case Margin::Type::VERTICAL:
							state.margin_v = margins->y;
							break;
						case Margin::Type::BOTH:
							state.margin_h = margins->x,
							state.margin_v = margins->y;
							break;
					}
				}
				break;
			case Tag::Type::DIRECTION:
                                state.vertical = dynamic_cast<const Direction*>(tag)->mode == Direction::Mode::TTB;
				break;
			case Tag::Type::IDENTITY:
				state.matrix.identity();
				break;
			case Tag::Type::TRANSLATE:
				{
					const Translate* translation = dynamic_cast<const Translate*>(tag);
					switch(translation->type){
						case Translate::Type::HORIZONTAL:
							state.matrix.translate(translation->x, 0, 0);
							break;
						case Translate::Type::VERTICAL:
							state.matrix.translate(0, translation->y, 0);
							break;
						case Translate::Type::BOTH:
							state.matrix.translate(translation->x, translation->y, 0);
							break;
					}
				}
				break;
			case Tag::Type::SCALE:
				{
					const Scale* scale = dynamic_cast<const Scale*>(tag);
					switch(scale->type){
						case Scale::Type::HORIZONTAL:
							state.matrix.scale(scale->x, 0, 0);
							break;
						case Scale::Type::VERTICAL:
							state.matrix.scale(0, scale->y, 0);
							break;
						case Scale::Type::BOTH:
							state.matrix.scale(scale->x, scale->y, 0);
							break;
					}
				}
				break;
			case Tag::Type::ROTATE:
				{
					const Rotate* rotation = dynamic_cast<const Rotate*>(tag);
					switch(rotation->axis){
						case Rotate::Axis::XY:
							state.matrix.rotate_x(DEG_TO_RAD(rotation->angle1)).rotate_y(DEG_TO_RAD(rotation->angle2));
							break;
						case Rotate::Axis::YX:
							state.matrix.rotate_y(DEG_TO_RAD(rotation->angle1)).rotate_x(DEG_TO_RAD(rotation->angle2));
							break;
						case Rotate::Axis::Z:
							state.matrix.rotate_z(DEG_TO_RAD(rotation->angle1));
							break;
					}
				}
				break;
			case Tag::Type::SHEAR:
				{
					const Shear* shear = dynamic_cast<const Shear*>(tag);
					switch(shear->type){
						case Shear::Type::HORIZONTAL:
							state.matrix.multiply({
                                                                1, shear->x, 0, 0,
                                                                0, 1, 0, 0,
                                                                0, 0, 1, 0,
                                                                0, 0, 0, 1
							});
							break;
						case Shear::Type::VERTICAL:
							state.matrix.multiply({
                                                                1, 0, 0, 0,
                                                                shear->y, 1, 0, 0,
                                                                0, 0, 1, 0,
                                                                0, 0, 0, 1
							});
							break;
						case Shear::Type::BOTH:
							state.matrix.multiply({
                                                                1, shear->x, 0, 0,
                                                                shear->y, 1, 0, 0,
                                                                0, 0, 1, 0,
                                                                0, 0, 0, 1
							});
							break;
					}
					// TODO
				}
				break;
			case Tag::Type::TRANSFORM:
				{
					const Transform* transformation = dynamic_cast<const Transform*>(tag);
					state.matrix.multiply({
						transformation->xx, transformation->xy, transformation->xz, transformation->x0,
						transformation->yx, transformation->yy, transformation->yz, transformation->y0,
						transformation->zx, transformation->zy, transformation->zz, transformation->z0,
						0, 0, 0, 1
					});
				}
				break;
			case Tag::Type::COLOR:
				{
					const Color* color = dynamic_cast<const Color*>(tag);

                                        // TODO

				}
				break;
			case Tag::Type::LINE_COLOR:
				// TODO
				break;
			case Tag::Type::ALPHA:
				// TODO
				break;
			case Tag::Type::LINE_ALPHA:
				// TODO
				break;
			case Tag::Type::TEXTURE:
				// TODO
				break;
			case Tag::Type::TEXFILL:
				// TODO
				break;
			case Tag::Type::BLEND:
				// TODO
				break;
			case Tag::Type::BLUR:
				// TODO
				break;
			case Tag::Type::STENCIL:
				// TODO
				break;
			case Tag::Type::ANTI_ALIASING:
				// TODO
				break;
			case Tag::Type::FADE:
				// TODO
				break;
			case Tag::Type::ANIMATE:
				// TODO
				break;
			case Tag::Type::KARAOKE:
				{
					const Karaoke* karaoke = dynamic_cast<const Karaoke*>(tag);
					switch(karaoke->type){
						case Karaoke::Type::DURATION:
							// Activate karaoke
							if(state.karaoke_start == ULONG_MAX)
								state.karaoke_start = 0;
							// Update karaoke
							state.karaoke_start += state.karaoke_duration,
							state.karaoke_duration = karaoke->time;
							break;
						case Karaoke::Type::SET:
							state.karaoke_start = karaoke->time,
							state.karaoke_duration = 0;
							break;
					}
				}
				break;
			case Tag::Type::KARAOKE_COLOR:
				{
					const KaraokeColor* color = dynamic_cast<const KaraokeColor*>(tag);
					state.karaoke_color = {color->color.r, color->color.g, color->color.b};
				}
				break;
			case Tag::Type::KARAOKE_MODE:
				state.karaoke_mode = dynamic_cast<const KaraokeMode*>(tag)->mode;
				break;
		}
	}
}
