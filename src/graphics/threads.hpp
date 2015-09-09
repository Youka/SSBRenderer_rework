/*
Project: SSBRenderer
File: threads.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <thread>
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
        #include <unistd.h>
#endif

namespace stdex{
	static inline unsigned hardware_concurrency(){
                unsigned n = std::thread::hardware_concurrency();
                if(!n)	// std::thread::hardware_concurrency is just a hint, means, no or bad implementations
#ifdef _WIN32
			{
				SYSTEM_INFO si;
				GetSystemInfo(&si);
				n = si.dwNumberOfProcessors;
			}
#else
			n = sysconf(_SC_NPROCESSORS_ONLN);
#endif
		return n ? n : 1;	// No processors? I don't think...
	}
}
