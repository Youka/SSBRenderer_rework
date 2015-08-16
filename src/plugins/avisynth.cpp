/*
Project: SSBRenderer
File: avisynth.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

// Link Avisynth statically
#define AVSC_NO_DECLSPEC
#include <windows.h>
#include <cstdlib>
#include "interfaces/avisynth_c.h"

#include "FilterBase.hpp"

#include <memory>
#include <string>

namespace AVS{
	// Avisynth library handle (defined in plugin initialization)
	AVS_Library* avs_library = nullptr;

	// Filter finished
	void AVSC_CC free_filter(AVS_FilterInfo* filter_info){
		FilterBase::AVS::deinit(filter_info->user_data);
	}

	// Frame filtering
	AVS_VideoFrame* AVSC_CC get_frame(AVS_FilterInfo* filter_info, int n){
		// Get current frame
		AVS_VideoFrame* frame = avs_library->avs_get_frame(filter_info->child, n);
		// Make frame writable
		avs_library->avs_make_writable(filter_info->env, &frame);
		// Render on frame
		FilterBase::AVS::filter_frame(avs_get_write_ptr(frame), avs_get_pitch(frame), n * (filter_info->vi.fps_denominator * 1000.0 / filter_info->vi.fps_numerator), &filter_info->user_data);
		// Pass frame further in processing chain
		return frame;
	}

	// Filter call
	AVS_Value AVSC_CC apply_filter(AVS_ScriptEnvironment* env, AVS_Value args, void*){
		// Extract clip
		AVS_FilterInfo* filter_info;
		std::unique_ptr<AVS_Clip, std::function<void(AVS_Clip*)>> clip(avs_library->avs_new_c_filter(env, &filter_info, avs_array_elt(args, 0), 1), std::function<void(AVS_Clip*)>([](AVS_Clip* clip){avs_library->avs_release_clip(clip);}));
		// Check video
		AVS_VideoInfo* vinfo_native = &filter_info->vi;
		if(!avs_has_video(vinfo_native))	// Clip must have a video stream
			return avs_new_value_error("Video required!");
		else if(!avs_is_rgb(vinfo_native))	// Video must store colors in RGB24 or RGBA32 format
			return avs_new_value_error("Video colorspace must be RGB!");
		// Pack arguments for filter base
		std::vector<FilterBase::AVS::Variant> packed_args;
		for(int i = 1, args_n = avs_array_size(args); i < args_n; ++i){
			AVS_Value val = avs_array_elt(args, i);
			FilterBase::AVS::Variant var;
			if(!avs_defined(val))
				var.type = FilterBase::AVS::ArgType::NONE;
			else if(avs_is_bool(val)){
				var.type = FilterBase::AVS::ArgType::BOOL;
				var.b = avs_as_bool(val);
			}else if(avs_is_int(val)){
				var.type = FilterBase::AVS::ArgType::INTEGER;
				var.i = avs_as_int(val);
			}else if(avs_is_float(val)){
				var.type = FilterBase::AVS::ArgType::FLOAT;
				var.f = avs_as_float(val);
			}else if(avs_is_string(val)){
				var.type = FilterBase::AVS::ArgType::STRING;
				var.s = avs_as_string(val);
			}
			packed_args.push_back(var);
		}
		// Initialize filter base
		filter_info->user_data = nullptr;
		try{
			FilterBase::AVS::init({vinfo_native->width, vinfo_native->height, avs_is_rgb32(vinfo_native) ? FilterBase::ColorType::BGRA : FilterBase::ColorType::BGR, static_cast<double>(vinfo_native->fps_numerator)/vinfo_native->fps_denominator, vinfo_native->num_frames}, packed_args, &filter_info->user_data);
		}catch(std::string err){
			return avs_new_value_error(err.c_str());
		}
		// Set further callbacks
		filter_info->free_filter = free_filter;
		filter_info->get_frame = get_frame;
		// Return filtered clip
		AVS_Value val;
		avs_library->avs_set_to_clip(&val, clip.get());
		return val;
	}
}

// Avisynth plugin entry point
AVSC_EXPORT const char* avisynth_c_plugin_init(AVS_ScriptEnvironment* env){
	// Get/create avisynth library
	if(!AVS::avs_library && !(AVS::avs_library = avs_load_library())){
		MessageBoxA(NULL, "Couldn't load avisynth functions from library", "Loading error", MB_OK);
		::exit(1);	// Plugin couldn't load, everything is doomed!
	}
	// Valid Avisynth interface version?
	AVS::avs_library->avs_check_version(env, AVISYNTH_INTERFACE_VERSION);
	// Get optional filter parameters (beside clip)
	std::string args_def("c");
	std::vector<std::pair<std::string, FilterBase::AVS::ArgType>> opt_args = FilterBase::AVS::get_args();
	for(auto arg : opt_args)
		switch(arg.second){
			case FilterBase::AVS::ArgType::BOOL: args_def += '[' + arg.first + "]b"; break;
			case FilterBase::AVS::ArgType::INTEGER: args_def += '[' + arg.first + "]i"; break;
			case FilterBase::AVS::ArgType::FLOAT: args_def += '[' + arg.first + "]f"; break;
			case FilterBase::AVS::ArgType::STRING: args_def += '[' + arg.first + "]s"; break;
			case FilterBase::AVS::ArgType::NONE: /* ignored */ break;
		}
	// Register function to Avisynth scripting environment
	AVS::avs_library->avs_add_function(env, FilterBase::get_name(), args_def.c_str(), AVS::apply_filter, nullptr);
	// Return plugin description
	return FilterBase::get_description();
}
