/*
Project: SSBRenderer
File: Overlay.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "../graphics/gutils.hpp"
#include "../parser/SSBData.hpp"

namespace SSB{
	// Image with overlay instruction
	struct Overlay{
		GUtils::Image2D<> image;
		int x, y;
		Blend::Mode op;
		Time fade_in, fade_out;
	};

	// Fades overlay and blends on target
	inline void blend_overlay(Time start_ms, Time end_ms, Time cur_ms,
				Overlay& overlay){
		// Cast SSB blend mode to GUtils blend operation
		GUtils::BlendOp op;
		switch(overlay.op){
			case Blend::Mode::OVER: op = GUtils::BlendOp::OVER; break;
			case Blend::Mode::ADDITION: op = GUtils::BlendOp::ADD; break;
			case Blend::Mode::SUBTRACT: op = GUtils::BlendOp::SUB; break;
			case Blend::Mode::MULTIPLY: op = GUtils::BlendOp::MUL; break;
			case Blend::Mode::SCREEN: op = GUtils::BlendOp::SCR; break;
			case Blend::Mode::DIFFERENCES: op = GUtils::BlendOp::DIFF; break;
		}
		// Calculate fade factor
		Time inner_ms = cur_ms - start_ms,
			inv_inner_ms = end_ms - cur_ms;
		double alpha = inner_ms < overlay.fade_in ? static_cast<double>(inner_ms) / overlay.fade_in : (inv_inner_ms < overlay.fade_out ? static_cast<double>(inv_inner_ms) / overlay.fade_out : 1);
		// Fade image
		if(alpha != 1)
			std::for_each(overlay.image.get_data(), overlay.image.get_data() + overlay.image.get_size(), [&alpha](unsigned char& x){x /= alpha;});
		// Convert target from BGRX to BGRA for blending

                // TODO

		// Blend overlay on target

		// TODO

	}
}
