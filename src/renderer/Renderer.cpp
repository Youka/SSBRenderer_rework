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

namespace SSB{
	Renderer::Renderer(int width, int height, Renderer::Colorspace format, const std::string& script, bool warnings) throw(Exception) : warnings(warnings){

		// TODO

	}

	Renderer::Renderer(int width, int height, Renderer::Colorspace format, std::istream& data, bool warnings) throw(Exception) : warnings(warnings){

		// TODO

	}

	void Renderer::set_target(int width, int height, Renderer::Colorspace format){

		// TODO

	}

	void Renderer::render(unsigned char* image, unsigned pitch, unsigned long start_ms){

		// TODO

	}
}