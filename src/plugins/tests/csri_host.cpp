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
#if _WIN32
	#define WIN_LEAN_AND_MEAN
	#include <windows.h>
	#define DLL_HANDLE HMODULE
	#define DLL_OPEN LoadLibrary
	#define DLL_GET_PROC GetProcAddress
	#define DLL_CLOSE FreeLibrary
#else
	#include <dlfcn.h>
	#define DLL_HANDLE void*
	#define DLL_OPEN(libname) dlopen(libname, RTLD_LAZY)
	#define DLL_GET_PROC dlsym
	#define DLL_CLOSE dlclose
#endif // _WIN32
#define ASSIGN_FUNC(dll_handle, funcname) (funcname = reinterpret_cast<decltype(funcname)>(DLL_GET_PROC(dll_handle, #funcname)))

int main(){
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
		return 1;
	if(!(
		ASSIGN_FUNC(dll_handle, csri_renderer_next) &&
		ASSIGN_FUNC(dll_handle, csri_renderer_default) &&
		ASSIGN_FUNC(dll_handle, csri_renderer_byname) &&
		ASSIGN_FUNC(dll_handle, csri_renderer_info) &&
		ASSIGN_FUNC(dll_handle, csri_query_ext) &&
		ASSIGN_FUNC(dll_handle, csri_render) &&
		ASSIGN_FUNC(dll_handle, csri_request_fmt) &&
		ASSIGN_FUNC(dll_handle, csri_close) &&
		ASSIGN_FUNC(dll_handle, csri_open_mem) &&
		ASSIGN_FUNC(dll_handle, csri_open_file)
	)){
		DLL_CLOSE(dll_handle);
		return 1;
	}

	// Test plugin filtering

	DLL_CLOSE(dll_handle);
	return 0;
}
