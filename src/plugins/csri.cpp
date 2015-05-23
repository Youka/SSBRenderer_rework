/*
Project: SSBRenderer
File: csri.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#ifdef _WIN32
#define CSRIAPI extern "C" __declspec(dllexport)
#else
#define CSRIAPI extern "C" __attribute__((visibility("default")))
#endif
#define CSRI_OWN_HANDLES
using csri_rend = const char*;
using csri_inst = void*;
#include "interfaces/csri.h"

#include "FilterBase.hpp"

#include <string.h>

// Only renderer
csri_rend csri_user_renderer = FilterBase::get_name();

// Open interface with file content
CSRIAPI csri_inst* csri_open_file(csri_rend*, const char* filename, struct csri_openflag*){

	// TODO

	return nullptr;
}

// Open interface with memory content
CSRIAPI csri_inst* csri_open_mem(csri_rend*, const void* data, size_t length, struct csri_openflag*){

	// TODO

	return nullptr;
}

// Close interface
CSRIAPI void csri_close(csri_inst* inst){
	if(inst){
		FilterBase::deinit(*inst);
		delete inst;
	}
}

// Offer supported format and save him
CSRIAPI int csri_request_fmt(csri_inst* inst, const struct csri_fmt* fmt){
	if(!inst || !*inst || fmt->width == 0 || fmt->height == 0)
		return -1;
	FilterBase::ColorType colorspace;
	switch(fmt->pixfmt){
		case CSRI_F_BGRA: colorspace = FilterBase::ColorType::BGRA; break;
		case CSRI_F_BGR: colorspace = FilterBase::ColorType::BGR; break;
		case CSRI_F_BGR_: colorspace = FilterBase::ColorType::BGRX; break;
		case CSRI_F_RGBA:
		case CSRI_F_ARGB:
		case CSRI_F_ABGR:
		case CSRI_F_RGB_:
		case CSRI_F__RGB:
		case CSRI_F__BGR:
		case CSRI_F_RGB:
		case CSRI_F_AYUV:
		case CSRI_F_YUVA:
		case CSRI_F_YVUA:
		case CSRI_F_YUY2:
		case CSRI_F_YV12A:
		case CSRI_F_YV12:
		default: return -1;
	}

	// TODO

	//inst->renderer->set_target(fmt->width, fmt->height, colorspace);
	return 0;
}

// Render on frame with instance data
CSRIAPI void csri_render(csri_inst* inst, struct csri_frame* frame, double time){
	if(inst)
		FilterBase::filter_frame(frame->planes[0], frame->strides[0], time * 1000, inst);
}

// No extensions supported
CSRIAPI void* csri_query_ext(csri_rend*, csri_ext_id){
	return nullptr;
}

// Renderer informations
struct csri_info csri_user_renderer_info = {
	FilterBase::get_name(),	// Identifier
	FilterBase::get_version(),	// Version
	FilterBase::get_name(),	// Long name
	FilterBase::get_author(),	// Author
	FilterBase::get_copyright()	// Copyright
};

// Get renderer informations
CSRIAPI struct csri_info* csri_renderer_info(csri_rend*){
	return &csri_user_renderer_info;
}

// Just this renderer supported
CSRIAPI csri_rend* csri_renderer_byname(const char* name, const char* specific){
	return (strcmp(name, csri_user_renderer_info.name) || (specific && strcmp(specific, csri_user_renderer_info.specific))) ? nullptr : &csri_user_renderer;
}

// Default renderer = this renderer
CSRIAPI csri_rend* csri_renderer_default(){
	return &csri_user_renderer;
}

// No other renderers
CSRIAPI csri_rend* csri_renderer_next(csri_rend*){
	return nullptr;
}
