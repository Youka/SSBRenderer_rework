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
#include "threads.hpp"

std::vector<float> create_gauss_kernel(const float radius){
	// Allocate kernel
	const int radius_i = ::ceil(radius);
	std::vector<float> kernel((radius_i << 1) + 1
#ifdef __SSE2__
	, AlignedAllocator<float,16>()
#endif
	);
	// Generate gaussian kernel (first half)
	auto kernel_iter = kernel.begin();
	const float sigma = (radius * 2 + 1) / 3,
		part1 = 1 / (sigma * ::sqrt(2 * M_PI)),
		sqrsigma2 = 2 * sigma * sigma;
	for(int x = -radius_i; x <= 0; ++x)
		*kernel_iter++ = part1 * ::exp(-(x*x) / sqrsigma2);
	// Smooth kernel edge
	kernel.front() *= 1 - (radius_i - radius);
	// Complete kernel (second half)
	std::copy(kernel.begin(), --kernel_iter, kernel.rbegin());
	// Normalize kernel
	const float kernel_sum_inverse = 1 / std::accumulate(kernel.begin(), kernel.end(), 0.0f);
	for(float& kernel_value : kernel)
		kernel_value *= kernel_sum_inverse;
	return kernel;
}

namespace GUtils{
	void blur(unsigned char* data, const unsigned width, const unsigned height, const unsigned stride, const ColorDepth depth,
		const float strength_h, const float strength_v){
		// Nothing to do?
		if(strength_h <= 0 && strength_v <= 0)
			return;
		// Generate filter kernels
		std::vector<float> kernel_h, kernel_v;
		if(strength_h > 0) kernel_h = std::move(create_gauss_kernel(strength_h));
		if(strength_v > 0) kernel_v = strength_v == strength_h ? kernel_h : std::move(create_gauss_kernel(strength_v));
		// Collect further data informations
		const unsigned trimmed_stride = depth == ColorDepth::X1 ? width : (depth == ColorDepth::X3 ? width * 3 : width << 2/* X4 */);
		const unsigned offset = stride - trimmed_stride;
		// Setup buffers for data in floating point format (required for faster processing)
		std::vector<float> fdata(height * trimmed_stride
#ifdef __SSE2__
	, 		AlignedAllocator<float,16>()
#endif
		), fdata2(kernel_h.empty() || kernel_v.empty() ? 0 : fdata.size()/* Don't waste memory when just one blur happens */, fdata.get_allocator());
		// Copy data in first FP buffer
		if(offset){
			const unsigned char* pdata = data;
			for(auto fdata_iter = fdata.begin(); fdata_iter != fdata.end(); fdata_iter = std::copy(pdata, pdata+trimmed_stride, fdata_iter), pdata += stride);
		}else
			fdata.assign(data, data+fdata.size());
		// Get threads number
		const unsigned threads_n = stdex::hardware_concurrency(),
			remote_threads_n = threads_n - 1;
		// Create blur threads & task storage
		std::vector<std::thread> threads;
		threads.reserve(remote_threads_n);
		std::function<void(const unsigned)> blur_task;
		// Run threads for horizontal blur
#define RUN_THREADED_TASK \
	for(unsigned thread_i = 0; thread_i < remote_threads_n; ++thread_i) \
		threads.emplace_back(blur_task, thread_i); \
	blur_task(remote_threads_n); \
	for(std::thread& t : threads) \
		t.join(); \
	threads.clear();
		if(!kernel_h.empty()){
			if(kernel_v.empty())
				switch(depth){
					case ColorDepth::X1:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
					case ColorDepth::X3:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
					case ColorDepth::X4:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
				}
			else
				switch(depth){
					case ColorDepth::X1:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
					case ColorDepth::X3:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
					case ColorDepth::X4:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
				}
			RUN_THREADED_TASK
		}
		// Run threads for vertical blur
		if(!kernel_v.empty()){
			if(kernel_h.empty())
				switch(depth){
					case ColorDepth::X1:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
					case ColorDepth::X3:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
					case ColorDepth::X4:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
				}
			else
				switch(depth){
					case ColorDepth::X1:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
					case ColorDepth::X3:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
					case ColorDepth::X4:
						blur_task = [&](const unsigned thread_i){

							// TODO

						};
						break;
				}
			RUN_THREADED_TASK
		}
	}
}
