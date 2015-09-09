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
		GUtils::Image2D<> image; // Always RGBA
		int x, y;
		Blend::Mode op;
		Time fade_in, fade_out;
	};

	// Colorspace type
	enum class Colorspace{BGR, BGRX, BGRA};

	// Fades overlay and blends on target
	static inline void blend_overlay(Time start_ms, Time end_ms, Time cur_ms,
				Overlay& overlay,
				unsigned char* data, unsigned width, unsigned height, unsigned stride, Colorspace format){
		// Calculate fade factor
		Time inner_ms = cur_ms - start_ms,
			inv_inner_ms = end_ms - cur_ms;
		double alpha = inner_ms < overlay.fade_in ? static_cast<double>(inner_ms) / overlay.fade_in : (inv_inner_ms < overlay.fade_out ? static_cast<double>(inv_inner_ms) / overlay.fade_out : 1.0);
		// Fade image
		if(alpha != 1.0){
			unsigned char* pdata = overlay.image.get_data();
			const unsigned char* const data_end = pdata + overlay.image.get_size();
			while(pdata != data_end)
				*pdata++ *= alpha;
		}
		// Convert target from BGRX to BGRA for blending requirements
		if(format == Colorspace::BGRX){
			unsigned char* pdata = overlay.image.get_data();
			const unsigned char* const data_end = pdata + overlay.image.get_size();
			const unsigned row_length = overlay.image.get_width() << 2,
				offset = overlay.image.get_stride() - row_length;
			const unsigned char* pdata_row_end;
			while(pdata != data_end){
				for(pdata_row_end = pdata + row_length; pdata != pdata_row_end; pdata += 4)
					pdata[3] = 255;
				pdata += offset;
			}
		}
		// Cast SSB blend mode to GUtils blend operation
		GUtils::BlendOp op;
		switch(overlay.op){
			case Blend::Mode::OVER: op = GUtils::BlendOp::OVER; break;
			case Blend::Mode::ADDITION: op = GUtils::BlendOp::ADD; break;
			case Blend::Mode::SUBTRACT: op = GUtils::BlendOp::SUB; break;
			case Blend::Mode::MULTIPLY: op = GUtils::BlendOp::MUL; break;
			case Blend::Mode::SCREEN: op = GUtils::BlendOp::SCR; break;
			case Blend::Mode::DIFFERENCES: op = GUtils::BlendOp::DIFF; break;
			default: op = GUtils::BlendOp::OVER; break;	// Compiler nonsense, all possibles cases were already handled
		}
		// Blend overlay on target
		GUtils::blend(
			overlay.image.get_data(), overlay.image.get_width(), overlay.image.get_height(), overlay.image.get_stride(), true,
			data, width, height, stride, format != Colorspace::BGR,
			overlay.x, overlay.y, op
		);
	}
}
