/*
Project: SSBRenderer
File: blur.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "gutils.hpp"
#include <vector>
#include "simd.hpp"
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <thread>

inline std::vector<float> create_gauss_kernel(float radius){
	// Allocate kernel
	const int radius_i = ::ceil(radius);
	std::vector<float> kernel((radius_i << 1) + 1
#ifdef __SSE2__
	, Align16Allocator<float>()
#endif
	);
	// Generate gaussian kernel
	float* pkernel = kernel.data();
	const float sigma = (radius * 2 + 1) / 3,
		part1 = 1 / (sigma * ::sqrt(2 * M_PI)),
		sqrsigma2 = 2 * sigma * sigma;
	for(int x = -radius_i; x <= radius_i; ++x)
		*pkernel++ = part1 * ::exp(-(x*x) / sqrsigma2);
	// Smooth kernel edges
	const float radius_fract = radius - ::floor(radius);
	if(radius_fract > 0)
		kernel.front() *= radius_fract,
		kernel.back() *= radius_fract;
	// Normalize kernel
	float kernel_sum = std::accumulate(kernel.begin(), kernel.end(), 0.0f);
	if(kernel_sum != 1){
		kernel_sum = 1 / kernel_sum;
		for(float& kernel_value : kernel)
			kernel_value *= kernel_sum;
	}
	return kernel;
}

namespace GUtils{
	void blur(unsigned char* data, const unsigned width, const unsigned height, const unsigned stride, const ColorDepth depth, const float strength){
		// Anything to do?
		if(strength <= 0)
			return;
		// Generate filter kernel
		auto kernel = std::move(create_gauss_kernel(strength));

		// TODO

	}
}
