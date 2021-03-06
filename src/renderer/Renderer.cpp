/*
Project: SSBRenderer
File: Renderer.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "Renderer.hpp"
#include "../parser/SSBParser.hpp"
#include "../utils/io.hpp"
#include "Geometry.hpp"
#include "RenderState.hpp"

namespace SSB{
	void Renderer::init(int width, int height, Colorspace format, std::istream& data, bool warnings) throw(Exception){
		this->set_target(width, height, format);
		Parser(warnings ? Parser::Level::ALL : Parser::Level::OFF).parse_script(this->script_data, data);
	}

	Renderer::Renderer(int width, int height, Colorspace format, const std::string& script, bool warnings) throw(Exception)
	: script_directory(stdex::get_file_dir(script)){
		stdex::fstream file(script, stdex::fstream::in);
		if(!file)
			throw Exception("Couldn't open file \"" + script + '\"');
		this->init(width, height, format, file, warnings);
	}

	Renderer::Renderer(int width, int height, Colorspace format, std::istream& data, bool warnings) throw(Exception){
		if(!data)
			throw Exception("Bad data stream");
		this->init(width, height, format, data, warnings);
	}

	void Renderer::set_target(int width, int height, Colorspace format){
		this->width = width,
		this->height = height,
		this->format = format,
		this->renderer.set_size(::abs(width), ::abs(height)),
		this->event_cache.clear();	// New positions by margin -> change!
	}

	void Renderer::render(unsigned char* image, unsigned stride, unsigned long start_ms){
		// Flag for correct image flipping
		bool image_flipped = false;
		// Iterate through SSB events
		for(Event& event : this->script_data.events)
			// Active SSB event?
			if(start_ms >= event.start_ms && start_ms < event.end_ms){
				// Flip image for right row alignment
				if(this->height < 0 && !image_flipped)
					GUtils::flip(image, ::abs(this->height), stride),
					image_flipped = true;
				// Recycle event overlays from cache
				if(this->event_cache.contains(&event))
					for(Overlay& overlay : this->event_cache.get(&event))
						blend_overlay(event.start_ms, event.end_ms, start_ms,
								overlay,
								image, ::abs(this->width), ::abs(this->height), stride, this->format);
				// Draw event
				else{
					// Event overlays collection
					std::vector<Overlay> overlays;
					// Get scale for script->frame
					double frame_scale_x, frame_scale_y;
					get_2d_scale(::abs(this->width), ::abs(this->height), this->script_data.frame.width, this->script_data.frame.height, frame_scale_x, frame_scale_y);
					// Collect render sizes

					// TODO

					// Draw!

					// TODO

					// Clear stencil
					this->renderer.clear_stencil();
					// Save event overlays to cache
					if(!overlays.empty())
						this->event_cache.add(&event, std::move(overlays));
				}
			}
		// Image needs to get flipped back?
		if(image_flipped)
			GUtils::flip(image, ::abs(this->height), stride);
	}
}
