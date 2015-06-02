/*
Project: SSBRenderer
File: FilterBase.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include <vector>
#include <string>
#ifdef _WIN32
#include <windef.h>
#else
#define HWND void*
#endif	// _WIN32
#ifndef CLSID_DEFINED
struct CLSID;
#endif	// CLSID_DEFINED

// Cross interface filter functions
namespace FilterBase{
	// Data types for functions below
	enum class ArgType{BOOL, INTEGER, FLOAT, STRING, NONE};
	enum class ColorType{BGR, BGRX, BGRA, UNKNOWN};
	struct VideoInfo{
		int width, height;
		ColorType format;
		double fps;
		long frames;
	};
	struct Variant{
		ArgType type;
		union{
			bool b;
			int i;
			float f;
			const char* s;
		};
	};
	// Meta informations
	const char* get_id();	// Vapoursynth only
	const char* get_namespace();	// Vapoursynth only
	const CLSID* get_filter_guid();	// DirectShow only
	const CLSID* get_filter_config_guid();	// DirectShow only
	const char* get_name();
	const wchar_t* get_namew();	// DirectShow only
	const char* get_description();
	const char* get_version();
	const char* get_author();
	const char* get_copyright();
	// Avisynth & Vapoursynth processes
	std::vector<std::pair<std::string, ArgType>> get_opt_args();
	void init(VideoInfo vinfo, std::vector<Variant> args, void** userdata) throw (const char*);
	// VirtualDub processes
	void init(void** userdata) throw (const char*);
	std::string gen_args_description(void* userdata);
	int request_config(HWND wnd, void** userdata);
	void start(VideoInfo vinfo, void** userdata) throw (const char*);
	void end(void** userdata);
	// CSRI processes
	bool init(const char* filename, void** userdata);
	bool init(std::istream& stream, void** userdata);
	void setup(decltype(VideoInfo::width) width, decltype(VideoInfo::height) height, decltype(VideoInfo::format) format, void** userdata);
	// Overall processes
	void filter_frame(unsigned char* image_data, int stride, unsigned long ms, void** userdata);
	void deinit(void* userdata);
}
