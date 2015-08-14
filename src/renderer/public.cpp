/*
Project: SSBRenderer
File: public.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "public.h"
#include "Renderer.hpp"
#include <cstring>
#include <sstream>
#include <config.h>

ssb_renderer ssb_create_renderer(int width, int height, char format, const char* script, char* warning){
	SSB::Renderer::Colorspace rformat;
	switch(format){
		case SSB_BGR: rformat = SSB::Renderer::Colorspace::BGR; break;
		case SSB_BGRX: rformat = SSB::Renderer::Colorspace::BGRX; break;
		case SSB_BGRA: rformat = SSB::Renderer::Colorspace::BGRA; break;
		default: return 0;
	}
	try{
		return new SSB::Renderer(width, height, rformat, script, warning != 0);
	}catch(SSB::Exception e){
		if(warning)
			strncpy(warning, e.what(), SSB_WARNING_LENGTH-1)[SSB_WARNING_LENGTH-1] = '\0';
		return 0;
	}
}

ssb_renderer ssb_create_renderer_from_memory(int width, int height, char format, const char* data, char* warning){
	SSB::Renderer::Colorspace rformat;
	switch(format){
		case SSB_BGR: rformat = SSB::Renderer::Colorspace::BGR; break;
		case SSB_BGRX: rformat = SSB::Renderer::Colorspace::BGRX; break;
		case SSB_BGRA: rformat = SSB::Renderer::Colorspace::BGRA; break;
		default: return 0;
	}
	std::istringstream data_stream(data);
	try{
		return new SSB::Renderer(width, height, rformat, data_stream, warning != 0);
	}catch(SSB::Exception e){
		if(warning)
			strncpy(warning, e.what(), SSB_WARNING_LENGTH-1)[SSB_WARNING_LENGTH-1] = '\0';
		return 0;
	}
}

void ssb_set_target(ssb_renderer renderer, int width, int height, char format){
	if(renderer){
		SSB::Renderer::Colorspace rformat;
		switch(format){
			case SSB_BGR: rformat = SSB::Renderer::Colorspace::BGR; break;
			case SSB_BGRX: rformat = SSB::Renderer::Colorspace::BGRX; break;
			case SSB_BGRA: rformat = SSB::Renderer::Colorspace::BGRA; break;
			default: return;
		}
		reinterpret_cast<SSB::Renderer*>(renderer)->set_target(width, height, rformat);
	}
}

void ssb_render(ssb_renderer renderer, unsigned char* image, unsigned pitch, unsigned long start_ms){
	if(renderer)
		reinterpret_cast<SSB::Renderer*>(renderer)->render(image, pitch, start_ms);
}

void ssb_free_renderer(ssb_renderer renderer){
	if(renderer)
		delete reinterpret_cast<SSB::Renderer*>(renderer);
}

const char* ssb_get_version(void){
	return PROJECT_VERSION_STRING;
}
