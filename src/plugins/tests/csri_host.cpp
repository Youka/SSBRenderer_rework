/*
Project: SSBRenderer
File: csri_host.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../interfaces/csri.h"
#include "../../utils/io.hpp"
#include <stdexcept>

static bool write_ppm(const char* filename, unsigned width, unsigned height, unsigned char* data, unsigned long data_size){
	std::ofstream img(filename, std::ios::binary);
	if(!img)
		return false;
	img << "P6\n" << width << ' ' << height << "\n255\n";
	img.write(reinterpret_cast<char*>(data), data_size);
	return true;
}

int main(){
	// Load plugin
	DLL_HANDLE dll_handle;
	void* (*csri_renderer_next)(void*);
	void* (*csri_renderer_default)(void);
	void* (*csri_renderer_byname)(const char*, const char*);
	struct csri_info* (*csri_renderer_info)(void);
	void* (*csri_query_ext)(void*, csri_ext_id);
	void (*csri_render)(void*, struct csri_frame*, double);
	int (*csri_request_fmt)(void*, const struct csri_fmt*);
	void (*csri_close)(void*);
	void* (*csri_open_mem)(void*, const void*, size_t, struct csri_openflag*);
	void* (*csri_open_file)(void*, const char*, struct csri_openflag*);
	if(!(dll_handle = DLL_OPEN("libssbplugins_invert")))
		throw std::domain_error("Couldn't load plugin DLL.");
	if(!(
		DLL_ASSIGN_PROC(dll_handle, csri_renderer_next) &&
		DLL_ASSIGN_PROC(dll_handle, csri_renderer_default) &&
		DLL_ASSIGN_PROC(dll_handle, csri_renderer_byname) &&
		DLL_ASSIGN_PROC(dll_handle, csri_renderer_info) &&
		DLL_ASSIGN_PROC(dll_handle, csri_query_ext) &&
		DLL_ASSIGN_PROC(dll_handle, csri_render) &&
		DLL_ASSIGN_PROC(dll_handle, csri_request_fmt) &&
		DLL_ASSIGN_PROC(dll_handle, csri_close) &&
		DLL_ASSIGN_PROC(dll_handle, csri_open_mem) &&
		DLL_ASSIGN_PROC(dll_handle, csri_open_file)
	)){
		DLL_CLOSE(dll_handle);
		throw std::domain_error("Couldn't load DLL function.");
	}
	// Generate PPM image data
	const unsigned width = 800, height = 450;	/* Caution!!! Too big image could exceed stack memory. */
	unsigned x = 0, y = 0;
	unsigned char data[width*height*3];
	for(unsigned char* pdata = data, *data_end = data + sizeof(data); pdata != data_end; pdata += 3){
		float gamma = static_cast<decltype(gamma)>(x+y) / (width+height);
		pdata[0] = gamma * 255;
		pdata[1] = (1-gamma) * 255;
		pdata[2] = 0;
		if(x == width-1)
			x = 0, y++;
		else
			x++;
	}
	// Write original image
	if(!write_ppm("test.ppm", width, height, data, sizeof(data))){
		DLL_CLOSE(dll_handle);
		throw std::domain_error("Couldn't write original image.");
	}
	// Filter image with dummy renderer
	void* rinst;
	if(!(rinst = csri_open_mem(nullptr, "test", 4, nullptr))){
		DLL_CLOSE(dll_handle);
		throw std::bad_alloc();
	}
        const struct csri_fmt fmt = {CSRI_F_BGR, width, height};
	if(csri_request_fmt(rinst, &fmt)){
		DLL_CLOSE(dll_handle);
		throw std::invalid_argument ("Couldn't set format to CSRI instance.");
	}
	struct csri_frame frame = {CSRI_F_BGR, {data}, {width*3}};
	csri_render(rinst, &frame, 0.0);
	csri_close(rinst);
	// Write filtered image
	if(!write_ppm("test_filtered.ppm", width, height, data, sizeof(data))){
		DLL_CLOSE(dll_handle);
		throw std::domain_error("Couldn't write filtered image.");
	}
	// Unload plugin
	DLL_CLOSE(dll_handle);
	// Successfull finish
	return 0;
}
