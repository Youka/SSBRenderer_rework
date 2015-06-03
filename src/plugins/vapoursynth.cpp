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
	// Structure for data transport between callbacks
	struct InstanceData{
		std::unique_ptr<VSNodeRef, std::function<void(VSNodeRef*)>> clip;
		void* userdata;
	};

	// Frame filtering
	const VSFrameRef* VS_CC get_frame(int n, int activationReason, void** inst_data, void**, VSFrameContext* frame_ctx, VSCore* core, const VSAPI* vsapi){
		InstanceData* data = reinterpret_cast<InstanceData*>(*inst_data);
		// Frame creation
		if(activationReason == arInitial)
			// Request needed input frames
			vsapi->requestFrameFilter(n, data->clip.get(), frame_ctx);
		// Frame processing
		else if (activationReason == arAllFramesReady){
			// Create new frame
			const VSFrameRef* src = vsapi->getFrameFilter(n, data->clip.get(), frame_ctx);
			VSFrameRef* dst = vsapi->copyFrame(src, core);
			vsapi->freeFrame(src);
			// Render on frame
			const VSVideoInfo* vinfo = vsapi->getVideoInfo(data->clip.get());
			FilterBase::filter_frame(vsapi->getWritePtr(dst, 0), vsapi->getStride(dst, 0), n * (vinfo->fpsDen * 1000.0 / vinfo->fpsNum), &data->userdata);
			// Return new frame
			return dst;
		}
		return nullptr;
	}

	// Filter initialization
	void VS_CC init_filter(VSMap* in, VSMap* out, void** inst_data, VSNode* node, VSCore*, const VSAPI* vsapi){
		// Extract clip
		auto clip_deleter = [vsapi](VSNodeRef* node) -> void{vsapi->freeNode(node);};
		std::unique_ptr<VSNodeRef, decltype(clip_deleter)> clip(vsapi->propGetNode(in, "clip", 0, nullptr), clip_deleter);
		// Check video
		const VSVideoInfo* vinfo_native = vsapi->getVideoInfo(clip.get());
		if(vinfo_native->width < 1 || vinfo_native->height < 1)	// Clip must have a video stream
			vsapi->setError(out, "Video required!");
		else if(vinfo_native->format->id != pfRGB24 && vinfo_native->format->id != pfCompatBGR32)	// Video must store colors in RGB24 or RGB32 format
			vsapi->setError(out, "Video colorspace must be RGB24 or RGB32!");
		else{
			// Pack arguments for filter base
			std::vector<FilterBase::Variant> packed_args;
			std::vector<std::pair<std::string, FilterBase::ArgType>> opt_args = FilterBase::avs_get_args();
			for(auto arg : opt_args){
				FilterBase::Variant var;
				switch(vsapi->propGetType(in, arg.first.c_str())){
					case ptUnset:
						var.type = FilterBase::ArgType::NONE;
						break;
					//case ptBool:
					case ptInt:
						var.type = FilterBase::ArgType::INTEGER;
						var.i = vsapi->propGetInt(in, arg.first.c_str(), 0, nullptr);
						break;
					case ptFloat:
						var.type = FilterBase::ArgType::FLOAT;
						var.f = vsapi->propGetFloat(in, arg.first.c_str(), 0, nullptr);
						break;
					case ptData:
						var.type = FilterBase::ArgType::STRING;
						var.s = vsapi->propGetData(in, arg.first.c_str(), 0, nullptr);
						break;
				}
				packed_args.push_back(var);
			}
			// Allocate cross-callback data
			*inst_data = new InstanceData{std::unique_ptr<VSNodeRef, decltype(clip_deleter)>(vsapi->cloneNodeRef(clip.get()), clip_deleter), nullptr};
			// Initialize filter base & set output video informations
			try{
				FilterBase::avs_init({vinfo_native->width, vinfo_native->height, vinfo_native->format->id == pfCompatBGR32 ? FilterBase::ColorType::BGRA : FilterBase::ColorType::BGR, static_cast<double>(vinfo_native->fpsNum)/vinfo_native->fpsDen, vinfo_native->numFrames}, packed_args, &reinterpret_cast<InstanceData*>(*inst_data)->userdata);
				vsapi->setVideoInfo(vinfo_native, 1, node);
			}catch(const char* err){
				delete reinterpret_cast<InstanceData*>(*inst_data);
				vsapi->setError(out, err);
			}
		}
	}

	// Filter destruction
	void VS_CC free_filter(void* inst_data, VSCore*, const VSAPI*){
		InstanceData* data = reinterpret_cast<InstanceData*>(inst_data);
		FilterBase::deinit(data->userdata);
		delete data;
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
	std::vector<std::pair<std::string, FilterBase::ArgType>> opt_args = FilterBase::avs_get_args();
	for(auto arg : opt_args)
		switch(arg.second){
			case FilterBase::ArgType::BOOL:
			case FilterBase::ArgType::INTEGER: args_def += ';' + arg.first + ":int"; break;
			case FilterBase::ArgType::FLOAT: args_def += ';' + arg.first + ":float"; break;
			case FilterBase::ArgType::STRING: args_def += ';' + arg.first + ":data"; break;
			case FilterBase::ArgType::NONE: /* ignored */ break;
		}
	// Register filter to Vapoursynth with configuration in plugin storage (filter name, arguments, filter creation function, userdata, plugin storage)
	reg_func(FilterBase::get_name(), args_def.c_str(), VS::apply_filter, 0, plugin);
}
