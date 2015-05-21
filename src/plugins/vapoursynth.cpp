/*
Project: SSBRenderer
File: vapoursynth.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "interfaces/VapourSynth.h"

#include "FilterBase.hpp"

#include <memory>

namespace VS{
	// Frame filtering
	const VSFrameRef* VS_CC get_frame(int n, int activationReason, void** inst_data, void**, VSFrameContext* frame_ctx, VSCore* core, const VSAPI* vsapi){
		// Frame creation
		/*if(activationReason == arInitial)
			// Request needed input frames
			vsapi->requestFrameFilter(n, data->clip, frame_ctx);
		// Frame processing
		else if (activationReason == arAllFramesReady){
			// Create new frame
			const VSFrameRef* src = vsapi->getFrameFilter(n, data->clip, frame_ctx);
			VSFrameRef* dst = vsapi->copyFrame(src, core);
			vsapi->freeFrame(src);
			// Render on frame
			data->renderer->render(vsapi->getWritePtr(dst, 0), vsapi->getStride(dst, 0), n * (data->clip.info()->fpsDen * 1000.0 / data->clip.info()->fpsNum));
			// Return new frame
			return dst;
		}*/
		return nullptr;
	}

	// Filter initialization
	void VS_CC init_filter(VSMap* in, VSMap* out, void** inst_data, VSNode* node, VSCore*, const VSAPI* vsapi){
		// Extract clip
		std::unique_ptr<VSNodeRef, std::function<void(VSNodeRef*)>> clip(vsapi->propGetNode(in, "clip", 0, nullptr), std::function<void(VSNodeRef*)>([vsapi](VSNodeRef* node){vsapi->freeNode(node);}));
		// Check video
		const VSVideoInfo* vinfo_native = vsapi->getVideoInfo(clip.get());
		if(vinfo_native->width < 1 || vinfo_native->height < 1)	// Clip must have a video stream
			vsapi->setError(out, "Video required!");
		else if(vinfo_native->format->id != pfRGB24 && vinfo_native->format->id != pfCompatBGR32)	// Video must store colors in RGB24 or RGB32 format
			vsapi->setError(out, "Video colorspace must be RGB24 or RGB32!");
		else{
			// Pack video informations for filter base
			FilterBase::VideoInfo vinfo = {vinfo_native->width, vinfo_native->height, vinfo_native->format->id == pfCompatBGR32, static_cast<double>(vinfo_native->fpsNum)/vinfo_native->fpsDen, vinfo_native->numFrames};
			// Pack arguments for filter base
			std::vector<FilterBase::Variant> packed_args;
			/*for(int i = 0, keys_n = vsapi->propNumKeys(in); i < keys_n && vsapi->propGetType(in, vsapi->propGetKey(in, i)); ++i){
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
			}*/
			// Initialize filter base & set output video informations
			try{
				FilterBase::init(vinfo, packed_args, inst_data);
				vsapi->setVideoInfo(vinfo_native, 1, node);
			}catch(const char* err){
				vsapi->setError(out, err);
			}
		}
	}

	// Filter destruction
	void VS_CC free_filter(void* inst_data, VSCore*, const VSAPI*){
		FilterBase::deinit(inst_data);
	}

	// Filter creation
	void VS_CC apply_filter(const VSMap* in, VSMap* out, void*, VSCore* core, const VSAPI* vsapi){
		vsapi->createFilter(in, out, FilterBase::get_name(), init_filter, get_frame, free_filter, fmParallel, 0, nullptr, core);
	}
}

// Vapoursynth plugin entry point
VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin config_func, VSRegisterFunction reg_func, VSPlugin* plugin){
	// Write filter information to Vapoursynth configuration (identifier, namespace, description, vs version, is read-only, plugin storage)
	config_func(FilterBase::get_id(), FilterBase::get_namespace(), FilterBase::get_description(), VAPOURSYNTH_API_VERSION, 1, plugin);
	// Get optional filter parameters (beside clip)
	std::string args_def("clip:clip");
	std::vector<std::pair<std::string, FilterBase::ArgType>> opt_args = FilterBase::get_opt_args();
	for(auto arg : opt_args)
		switch(arg.second){
			case FilterBase::ArgType::BOOL: args_def += ';' + arg.first + ":bool"; break;
			case FilterBase::ArgType::INTEGER: args_def += ';' + arg.first + ":int"; break;
			case FilterBase::ArgType::FLOAT: args_def += ';' + arg.first + ":float"; break;
			case FilterBase::ArgType::STRING: args_def += ';' + arg.first + ":data"; break;
		}
	// Register filter to Vapoursynth with configuration in plugin storage (filter name, arguments, filter creation function, userdata, plugin storage)
	reg_func(FilterBase::get_name(), args_def.c_str(), VS::apply_filter, 0, plugin);
}
