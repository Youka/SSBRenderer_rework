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
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#else
        #include <unistd.h>
#endif

std::vector<float> create_gauss_kernel(float radius){
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
	kernel.front() = kernel.back() *= 1 - (radius_i - radius);
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
	void blur(unsigned char* data, const unsigned width, const unsigned height, const unsigned stride, const ColorDepth depth,
		const float strength_h, const float strength_v){
		// Anything to do?
		if(strength_h <= 0 && strength_v <= 0)
			return;
		// Generate filter kernels
		std::vector<float> kernel_h, kernel_v;
		if(strength_h > 0) kernel_h = std::move(create_gauss_kernel(strength_h));
		if(strength_v > 0) kernel_v = strength_v == strength_h ? kernel_h : std::move(create_gauss_kernel(strength_v));
		// Setup buffers for data in floating point format (required for faster processing)
		const unsigned trimmed_stride = depth == ColorDepth::X1 ? width : (depth == ColorDepth::X3 ? width * 3 : width << 2/* X4 */);
		std::vector<float> fdata(height * trimmed_stride
#ifdef __SSE2__
	, 		Align16Allocator<float>()
#endif
		), fdata2(fdata.size(), fdata.get_allocator());
		if(stride == trimmed_stride)
			std::copy(data, data+fdata.size(), fdata.begin());
		else{
			const unsigned char* pdata = data;
			float* pfdata = fdata.data();
			for(unsigned y = 0; y < height; ++y)
				pfdata = std::copy(pdata, pdata+trimmed_stride, pfdata),
				pdata += stride;
		}
		// Get threads number
                unsigned threads_n = std::thread::hardware_concurrency();
                if(!threads_n)	// std::thread::hardware_concurrency is just a hint, means, no or bad implementations
#ifdef _WIN32
			{
				SYSTEM_INFO si;
				GetSystemInfo(&si);
				threads_n = si.dwNumberOfProcessors;
			}
#else
			threads_n = sysconf(_SC_NPROCESSORS_ONLN);
#endif
		const unsigned remote_threads_n = threads_n - 1;
		// Create threads & run for horizontal blur
		std::vector<std::thread> threads;
		threads.reserve(remote_threads_n);
		auto blur_h = [&](unsigned i){

			// TODO: kernel check & horizontal blur

		};
		for(unsigned thread_i = 0; thread_i < remote_threads_n; ++thread_i)
			threads.emplace_back(blur_h, thread_i);
		blur_h(remote_threads_n);
		for(std::thread& t : threads)
			t.join();
		// Recreate threads & run for vertical blur
		threads.clear();
		auto blur_v = [&](unsigned i){

			// TODO: kernel check & vertical blur

		};
		for(unsigned thread_i = 0; thread_i < remote_threads_n; ++thread_i)
			threads.emplace_back(blur_v, thread_i);
		blur_v(remote_threads_n);
		for(std::thread& t : threads)
			t.join();
	}
}
