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

#include "config.hpp"

namespace AVS{
	// Avisynth library handle (defined in plugin initialization)
	AVS_Library* avs_library = nullptr;

	// Filter call
	AVS_Value AVSC_CC apply_filter(AVS_ScriptEnvironment* env, AVS_Value args, void*){

		// TODO

		return avs_new_value_bool(true);
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
	// Register function to Avisynth scripting environment
	// AVS::avs_library->avs_add_function(env, FILTER_NAME, "cs[warnings]b", AVS::apply_filter, nullptr);
	// Return plugin description
	return "";	// TODO
}
