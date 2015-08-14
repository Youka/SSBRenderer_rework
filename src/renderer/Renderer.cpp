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
#include "../strings/utf8.hpp"

namespace SSB{
	void Renderer::init(int width, int height, Renderer::Colorspace format, std::istream& data, bool warnings) throw(Exception){
		Parser(warnings ? Parser::Level::ALL : Parser::Level::OFF).parse_script(this->data, data);

		// TODO: backend renderer creation

	}

	Renderer::Renderer(int width, int height, Renderer::Colorspace format, const std::string& script, bool warnings) throw(Exception){
		Utf8::fstream file(script, Utf8::fstream::in);
		if(!file)
			throw Exception("Couldn't open file \"" + script + '\"');

		// TODO: save current directory path

		this->init(width, height, format, file, warnings);
	}

	Renderer::Renderer(int width, int height, Renderer::Colorspace format, std::istream& data, bool warnings) throw(Exception){
		if(!data)
			throw Exception("Bad data stream");
		this->init(width, height, format, data, warnings);
	}

	void Renderer::set_target(int width, int height, Renderer::Colorspace format){

		// TODO

	}

	void Renderer::render(unsigned char* image, unsigned pitch, unsigned long start_ms){

		// TODO

	}
}
