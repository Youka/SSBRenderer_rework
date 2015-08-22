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

#include "../parser/SSBData.hpp"
#include "../renderer_backend/Renderer.hpp"
#include "../graphics/gutils.hpp"
#include "../utils/memory.hpp"
#include <config.h>

namespace SSB{
	// Frontend renderer for SSB content
	class Renderer{
		public:
			enum class Colorspace{BGR, BGRX, BGRA};
		private:
			// Image data
			int width, height;
			Colorspace format;
			// Backend renderer
			::Renderer renderer;
			// Script data
			Data script_data;
			const std::string script_directory;
			// Caches
			struct Overlay{
				GUtils::Image2D<> image;
				int x, y;
				Blend::Mode op;
				Time fade_in, fade_out;
			};
			stdex::Cache<Event*, std::vector<Overlay>, MAX_CACHE> event_cache;
			// Initialization
			void init(int width, int height, Colorspace format, std::istream& data, bool warnings) throw(Exception);
		public:
			// Setters
			Renderer(int width, int height, Colorspace format, const std::string& script, bool warnings) throw(Exception);
			Renderer(int width, int height, Colorspace format, std::istream& data, bool warnings) throw(Exception);
			void set_target(int width, int height, Colorspace format);
			// Processing
			void render(unsigned char* image, unsigned pitch, unsigned long start_ms);
			// No copy&move (-> backend renderer limitation)
			Renderer(const Renderer&) = delete;
			Renderer(Renderer&&) = delete;
			Renderer& operator=(const Renderer&) = delete;
			Renderer& operator=(Renderer&&) = delete;
	};
}
