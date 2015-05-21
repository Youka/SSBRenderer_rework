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
		// Pack video informations for filter base
		FilterBase::VideoInfo vinfo = {vinfo_native->width, vinfo_native->height, static_cast<bool>(avs_is_rgb32(vinfo_native)), static_cast<double>(vinfo_native->fps_numerator)/vinfo_native->fps_denominator, vinfo_native->num_frames};
		// Pack arguments for filter base
		std::vector<FilterBase::Variant> packed_args;
		AVS_Value val;
		for(int i = 1, args_n = avs_array_size(args); i < args_n && avs_defined(val = avs_array_elt(args, i)); ++i){
			FilterBase::Variant var;
			if(avs_is_bool(val)){
				var.type = FilterBase::ArgType::BOOL;
				var.b = avs_as_bool(val);
			}else if(avs_is_int(val)){
				var.type = FilterBase::ArgType::INTEGER;
				var.i = avs_as_int(val);
			}else if(avs_is_float(val)){
				var.type = FilterBase::ArgType::FLOAT;
				var.f = avs_as_float(val);
			}else if(avs_is_string(val)){
				var.type = FilterBase::ArgType::STRING;
				var.s = avs_as_string(val);
			}
			packed_args.push_back(var);
		}
		// Initialize filter base
		try{
			FilterBase::init(vinfo, packed_args, &filter_info->user_data);
		}catch(const char* err){
			return avs_new_value_error(err);
		}

		// TODO

		// Return filtered clip
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
	std::vector<FilterBase::ArgType> opt_args = FilterBase::get_opt_args();
	for(auto arg : opt_args)
		switch(arg){
			case FilterBase::ArgType::BOOL_OPT: args_def += "[]";
			case FilterBase::ArgType::BOOL: args_def += 'b'; break;
			case FilterBase::ArgType::INTEGER_OPT: args_def += "[]";
			case FilterBase::ArgType::INTEGER: args_def += 'i'; break;
			case FilterBase::ArgType::FLOAT_OPT: args_def += "[]";
			case FilterBase::ArgType::FLOAT: args_def += 'f'; break;
			case FilterBase::ArgType::STRING_OPT: args_def += "[]";
			case FilterBase::ArgType::STRING: args_def += 's'; break;
		}
	// Register function to Avisynth scripting environment
	AVS::avs_library->avs_add_function(env, FilterBase::get_name(), args_def.c_str(), AVS::apply_filter, nullptr);
	// Return plugin description
	return FilterBase::get_description();
}
