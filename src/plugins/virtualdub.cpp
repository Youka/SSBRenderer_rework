/*
Project: SSBRenderer
File: virtualdub.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "interfaces/vd2/vdvideofilt.h"

#include "FilterBase.hpp"

namespace VDub{
	// Fill description for stringProc & stringProc2
	void fill_description(std::string desc, char* buf, int maxlen = 128){
		// Fill description buffer with given string
		_snprintf(buf, maxlen, desc.c_str());
	}
	// Filter definition
	VDXFilterDefinition filter_definition = {
		nullptr,			// _next
		nullptr,			// _prev
		nullptr,			// _module

		FilterBase::get_name(),		// name
		FilterBase::get_description(),	// desc
		FilterBase::get_author(),	// maker
		nullptr,			// private data
		0,				// inst_data_size

		// initProc (Filter initialization)
		[](VDXFilterActivation* fdata, const VDXFilterFunctions*) -> int{
			fdata->filter_data = nullptr;
			// Success
			return 0;
		},
		// deinitProc (Filter deinitialization)
		[](VDXFilterActivation* fdata, const VDXFilterFunctions*) -> void{
			fdata->filter_data = nullptr;
		},
		// runProc (Filter run/frame processing)
		[](const VDXFilterActivation* fdata, const VDXFilterFunctions*) -> int{
			FilterBase::filter_frame(reinterpret_cast<unsigned char*>(fdata->src.data), fdata->src.pitch, fdata->src.mFrameTimestampStart / 10000, const_cast<void**>(&fdata->filter_data));
			// Success
			return 0;
		},
		// paramProc (Filter video format)
		[](VDXFilterActivation*, const VDXFilterFunctions*) -> long{
			// Use default format (RGB, bottom-up)
			return 0;
		},
		// configProc (Filter configuration)
		[](VDXFilterActivation* fdata, const VDXFilterFunctions*, VDXHWND wnd) -> int{
			return FilterBase::request_config(reinterpret_cast<HWND>(wnd), &fdata->filter_data);
		},
		// stringProc (Filter description)
		[](const VDXFilterActivation* fdata, const VDXFilterFunctions*, char* buf) -> void{
			fill_description(FilterBase::gen_args_description(fdata->filter_data), buf);
		},
		// startProc (Filter start running)
		[](VDXFilterActivation* fdata, const VDXFilterFunctions* ffuncs) -> int{
			// All video informations available?
			if(fdata->pfsi == nullptr)
				ffuncs->Except("Video informations are missing!");
			// Pack video informations for filter base
			FilterBase::VideoInfo vinfo = {fdata->src.w, fdata->src.h, FilterBase::ColorType::BGRX, static_cast<double>(fdata->src.mFrameRateHi)/fdata->src.mFrameRateLo, fdata->src.mFrameCount};
			// Allocate renderer (and free previous renderer in case of buggy twice start)
			try{
				FilterBase::init(vinfo, std::vector<FilterBase::Variant>(), &fdata->filter_data);
			}catch(const char* err){
				ffuncs->Except(err);
				return 1;
			}
			// Success
			return 0;
		},
		// endProc (Filter end running)
		[](VDXFilterActivation* fdata, const VDXFilterFunctions*) -> int{
			FilterBase::deinit(fdata->filter_data);
			fdata->filter_data = nullptr;
			// Success
			return 0;
		},

		nullptr,	// script_obj
		nullptr,	// fssProc

		// stringProc2 (Filter description *newer versions*)
		[](const VDXFilterActivation* fdata, const VDXFilterFunctions*, char* buf, int maxlen) -> void{
			fill_description(FilterBase::gen_args_description(fdata->filter_data), buf, maxlen);
		},
		nullptr,	// serializeProc
		nullptr,	// deserializeProc
		nullptr,	// copyProc

		nullptr,	// prefetchProc

		nullptr,	// copyProc2
		nullptr,	// prefetchProc2
		nullptr	 	// eventProc
	};
}

// VirtualDub plugin interface - register
VDXFilterDefinition *registered_filter_definition;
extern "C" __declspec(dllexport) int VirtualdubFilterModuleInit2(struct VDXFilterModule* fmodule, const VDXFilterFunctions* ffuncs, int& vdfd_ver, int& vdfd_compat){
	// Create register definition
	registered_filter_definition = ffuncs->addFilter(fmodule, &VDub::filter_definition, sizeof(VDXFilterDefinition));
	// Version & compatibility definition
	vdfd_ver = VIRTUALDUB_FILTERDEF_VERSION;
	vdfd_compat = VIRTUALDUB_FILTERDEF_COMPATIBLE_COPYCTOR;
	// Success
	return 0;
}

// VirtualDub plugin interface - unregister
extern "C" __declspec(dllexport) void VirtualdubFilterModuleDeinit(struct VDXFilterModule*, const VDXFilterFunctions *ffuncs){
	// Remove register definition
	ffuncs->removeFilter(registered_filter_definition);
}
