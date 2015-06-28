/*
Project: SSBRenderer
File: simd.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "gutils.hpp"

#ifdef _MSC_VER
	#include <intrin.h>
	#define bit_SSE2 (1 << 26)
	#define bit_SSE3 (1 << 0)
	#define bit_AVX (1 << 28)
#else
	#include <cpuid.h>
#endif // _MSC_VER

namespace GUtils{
	vector_features detect_vectorization(){
#ifdef _MSC_VER
		int cpu_info[4];
		__cpuid(cpu_info, 1);
		return {static_cast<bool>(cpu_info[3] & bit_SSE2), static_cast<bool>(cpu_info[2] & bit_SSE3), static_cast<bool>(cpu_info[2] & bit_AVX)};
#else
		unsigned eax, ebx, ecx, edx;
		__cpuid (1, eax, ebx, ecx, edx);
		return {static_cast<bool>(edx & bit_SSE2), static_cast<bool>(ecx & bit_SSE3), static_cast<bool>(ecx & bit_AVX)};
#endif // _MSC_VER
	}
}
