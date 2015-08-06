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

static std::vector<float> create_gauss_kernel(const float radius){
	// Allocate kernel
	const int radius_i = ::ceil(radius);
	std::vector<float> kernel((radius_i << 1) + 1);
	// Generate gaussian kernel (first half)
	auto kernel_iter = kernel.begin();
	constexpr float sqrpi2 = ::sqrt(2 * M_PI);
	const float sigma = (radius * 2 + 1) / 3,
		part1 = 1 / (sigma * sqrpi2),
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
		// Setup buffers for data in floating point format (required for faster processing)
		const unsigned trimmed_stride = depth == ColorDepth::X1 ? width : (depth == ColorDepth::X3 ? width * 3 : width << 2/* X4 */);
		std::vector<float> fdata(height * trimmed_stride
#ifdef __SSE2__
	, 		AlignedAllocator<float,16>()
#endif
		), fdata2(kernel_h.empty() || kernel_v.empty() ? 0 : fdata.size()/* Don't waste memory when just one blur happens */, fdata.get_allocator());
		// Copy data in first FP buffer
		if(stride == trimmed_stride)
			fdata.assign(data, data+fdata.size());
		else{
			const unsigned char* pdata = data;
			for(auto fdata_iter = fdata.begin(); fdata_iter != fdata.end(); fdata_iter = std::copy(pdata, pdata+trimmed_stride, fdata_iter), pdata += stride);
		}
		// Get threads number
		const unsigned remote_threads_n = stdex::hardware_concurrency() - 1;
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
			// Helper values for faster processing in threads
			const unsigned fdata_jump = remote_threads_n * trimmed_stride,
				kernel_h_radius = (kernel_h.size() - 1) >> 1;
			// Select proper horizontal blur function
			if(kernel_v.empty()){
				const unsigned data_jump = stride - trimmed_stride + remote_threads_n * stride;
				switch(depth){
					case ColorDepth::X1:
						blur_task = [&,data_jump](const unsigned thread_i){
							unsigned char* pdata = data + thread_i * stride;
							for(decltype(fdata)::iterator fdata_iter = fdata.begin() + thread_i * trimmed_stride, fdata_iter_row_first, fdata_iter_row_end;
								fdata_iter < fdata.end();
								fdata_iter += fdata_jump, pdata += data_jump)
								for(fdata_iter_row_first = fdata_iter, fdata_iter_row_end = fdata_iter + trimmed_stride; fdata_iter != fdata_iter_row_end; ++fdata_iter)
									*pdata++ = std::inner_product(
										std::max(fdata_iter - kernel_h_radius, fdata_iter_row_first),
										std::min(fdata_iter_row_end, fdata_iter + kernel_h_radius + 1),
										kernel_h.begin() + std::max(0, fdata_iter_row_first - (fdata_iter - kernel_h_radius)),
										0.0f
									);
						};
						break;
					case ColorDepth::X3:
						blur_task = [&,data_jump](const unsigned thread_i){
							unsigned char* pdata = data + thread_i * stride;
#ifdef __SSE2__
							__m128 accum;
							unsigned char tmp[4];
#else
							float accum[3];
#endif
							for(decltype(fdata)::iterator fdata_iter = fdata.begin() + thread_i * trimmed_stride, fdata_iter_row_first, fdata_iter_row_end, fdata_kernel_iter, fdata_kernel_iter_end, kernel_iter;
								fdata_iter < fdata.end();
								fdata_iter += fdata_jump, pdata += data_jump)
								for(fdata_iter_row_first = fdata_iter, fdata_iter_row_end = fdata_iter + trimmed_stride; fdata_iter != fdata_iter_row_end; fdata_iter += 3, pdata += 3){
									for(
#ifdef __SSE2__
										accum = _mm_xor_ps(accum, accum),
#else
										accum[0] = accum[1] = accum[2] = 0,
#endif
										fdata_kernel_iter = std::max(fdata_iter - kernel_h_radius * 3, fdata_iter_row_first), fdata_kernel_iter_end = std::min(fdata_iter_row_end, fdata_iter + (kernel_h_radius + 1) * 3), kernel_iter = kernel_h.begin() + std::max(0, fdata_iter_row_first - (fdata_iter - kernel_h_radius * 3)) / 3; fdata_kernel_iter != fdata_kernel_iter_end; fdata_kernel_iter += 3, ++kernel_iter)
#ifdef __SSE2__
										accum = _mm_add_ps(
											accum,
											_mm_mul_ps(
												_mm_movelh_ps(
													_mm_castpd_ps(_mm_load_sd(reinterpret_cast<double*>(&(*fdata_kernel_iter)))),
													_mm_load_ss(&fdata_kernel_iter[2])
												),
												_mm_set1_ps(*kernel_iter)
											)
										);
									SSE2_STORE_PS_U8(tmp, accum),
									pdata[0] = tmp[0],
									pdata[1] = tmp[1],
									pdata[2] = tmp[2];
#else
										accum[0] += fdata_kernel_iter[0] * *kernel_iter,
										accum[1] += fdata_kernel_iter[1] * *kernel_iter,
										accum[2] += fdata_kernel_iter[2] * *kernel_iter;
									pdata[0] = accum[0],
									pdata[1] = accum[1],
									pdata[2] = accum[2];
#endif
								}
						};
						break;
					case ColorDepth::X4:
						blur_task = [&,data_jump](const unsigned thread_i){
							unsigned char* pdata = data + thread_i * stride;
#ifdef __SSE2__
							__m128 accum;
#else
							float accum[4];
#endif
							for(decltype(fdata)::iterator fdata_iter = fdata.begin() + thread_i * trimmed_stride, fdata_iter_row_first, fdata_iter_row_end, fdata_kernel_iter, fdata_kernel_iter_end, kernel_iter;
								fdata_iter < fdata.end();
								fdata_iter += fdata_jump, pdata += data_jump)
								for(fdata_iter_row_first = fdata_iter, fdata_iter_row_end = fdata_iter + trimmed_stride; fdata_iter != fdata_iter_row_end; fdata_iter += 4, pdata += 4){
									for(
#ifdef __SSE2__
										accum = _mm_xor_ps(accum, accum),
#else
										accum[0] = accum[1] = accum[2] = accum[3] = 0,
#endif
										fdata_kernel_iter = std::max(fdata_iter - (kernel_h_radius << 2), fdata_iter_row_first), fdata_kernel_iter_end = std::min(fdata_iter_row_end, fdata_iter + ((kernel_h_radius + 1) << 2)), kernel_iter = kernel_h.begin() + (std::max(0, fdata_iter_row_first - (fdata_iter - (kernel_h_radius << 2))) >> 2); fdata_kernel_iter != fdata_kernel_iter_end; fdata_kernel_iter += 4, ++kernel_iter)
#ifdef __SSE2__
										accum = _mm_add_ps(
											accum,
											_mm_mul_ps(
												_mm_load_ps(&(*fdata_kernel_iter)),
												_mm_set1_ps(*kernel_iter)
											)
										);
									SSE2_STORE_PS_U8(pdata, accum);
#else
										accum[0] += fdata_kernel_iter[0] * *kernel_iter,
										accum[1] += fdata_kernel_iter[1] * *kernel_iter,
										accum[2] += fdata_kernel_iter[2] * *kernel_iter,
										accum[3] += fdata_kernel_iter[3] * *kernel_iter;
									pdata[0] = accum[0],
									pdata[1] = accum[1],
									pdata[2] = accum[2],
									pdata[3] = accum[3];
#endif
								}
						};
						break;
				}
			}else
				switch(depth){
					case ColorDepth::X1:
						blur_task = [&](const unsigned thread_i){
							for(decltype(fdata)::iterator fdata_iter = fdata.begin() + thread_i * trimmed_stride, fdata2_iter = fdata2.begin() + (fdata_iter - fdata.begin()), fdata_iter_row_first, fdata_iter_row_end;
								fdata_iter < fdata.end();
								fdata_iter += fdata_jump, fdata2_iter += fdata_jump)
								for(fdata_iter_row_first = fdata_iter, fdata_iter_row_end = fdata_iter + trimmed_stride; fdata_iter != fdata_iter_row_end; ++fdata_iter)
									*fdata2_iter++ = std::inner_product(
										std::max(fdata_iter - kernel_h_radius, fdata_iter_row_first),
										std::min(fdata_iter_row_end, fdata_iter + kernel_h_radius + 1),
										kernel_h.begin() + std::max(0, fdata_iter_row_first - (fdata_iter - kernel_h_radius)),
										0.0f
									);
						};
						break;
					case ColorDepth::X3:
						blur_task = [&](const unsigned thread_i){
#ifdef __SSE2__
							__m128 accum;
#else
							float accum[3];
#endif
							for(decltype(fdata)::iterator fdata_iter = fdata.begin() + thread_i * trimmed_stride, fdata2_iter = fdata2.begin() + (fdata_iter - fdata.begin()), fdata_iter_row_first, fdata_iter_row_end, fdata_kernel_iter, fdata_kernel_iter_end, kernel_iter;
								fdata_iter < fdata.end();
								fdata_iter += fdata_jump, fdata2_iter += fdata_jump)
								for(fdata_iter_row_first = fdata_iter, fdata_iter_row_end = fdata_iter + trimmed_stride; fdata_iter != fdata_iter_row_end; fdata_iter += 3, fdata2_iter += 3){
									for(
#ifdef __SSE2__
										accum = _mm_xor_ps(accum, accum),
#else
										accum[0] = accum[1] = accum[2] = 0,
#endif
										fdata_kernel_iter = std::max(fdata_iter - kernel_h_radius * 3, fdata_iter_row_first), fdata_kernel_iter_end = std::min(fdata_iter_row_end, fdata_iter + (kernel_h_radius + 1) * 3), kernel_iter = kernel_h.begin() + std::max(0, fdata_iter_row_first - (fdata_iter - kernel_h_radius * 3)) / 3; fdata_kernel_iter != fdata_kernel_iter_end; fdata_kernel_iter += 3, ++kernel_iter)
#ifdef __SSE2__
										accum = _mm_add_ps(
											accum,
											_mm_mul_ps(
												_mm_movelh_ps(
													_mm_castpd_ps(_mm_load_sd(reinterpret_cast<double*>(&(*fdata_kernel_iter)))),
													_mm_load_ss(&fdata_kernel_iter[2])
												),
												_mm_set1_ps(*kernel_iter)
											)
										);
									_mm_store_sd(reinterpret_cast<double*>(&(*fdata2_iter)), _mm_castps_pd(accum)),
									_mm_store_ss(&fdata2_iter[2], _mm_movehl_ps(accum, accum));
#else
										accum[0] += fdata_kernel_iter[0] * *kernel_iter,
										accum[1] += fdata_kernel_iter[1] * *kernel_iter,
										accum[2] += fdata_kernel_iter[2] * *kernel_iter;
									fdata2_iter[0] = accum[0],
									fdata2_iter[1] = accum[1],
									fdata2_iter[2] = accum[2];
#endif
								}
						};
						break;
					case ColorDepth::X4:
						blur_task = [&](const unsigned thread_i){
#ifdef __SSE2__
							__m128 accum;
#else
							float accum[4];
#endif
							for(decltype(fdata)::iterator fdata_iter = fdata.begin() + thread_i * trimmed_stride, fdata2_iter = fdata2.begin() + (fdata_iter - fdata.begin()), fdata_iter_row_first, fdata_iter_row_end, fdata_kernel_iter, fdata_kernel_iter_end, kernel_iter;
								fdata_iter < fdata.end();
								fdata_iter += fdata_jump, fdata2_iter += fdata_jump)
								for(fdata_iter_row_first = fdata_iter, fdata_iter_row_end = fdata_iter + trimmed_stride; fdata_iter != fdata_iter_row_end; fdata_iter += 4, fdata2_iter += 4){
									for(
#ifdef __SSE2__
										accum = _mm_xor_ps(accum, accum),
#else
										accum[0] = accum[1] = accum[2] = accum[3] = 0,
#endif
										fdata_kernel_iter = std::max(fdata_iter - (kernel_h_radius << 2), fdata_iter_row_first), fdata_kernel_iter_end = std::min(fdata_iter_row_end, fdata_iter + ((kernel_h_radius + 1) << 2)), kernel_iter = kernel_h.begin() + (std::max(0, fdata_iter_row_first - (fdata_iter - (kernel_h_radius << 2))) >> 2); fdata_kernel_iter != fdata_kernel_iter_end; fdata_kernel_iter += 4, ++kernel_iter)
#ifdef __SSE2__
										accum = _mm_add_ps(
											accum,
											_mm_mul_ps(
												_mm_load_ps(&(*fdata_kernel_iter)),
												_mm_set1_ps(*kernel_iter)
											)
										);
									_mm_store_ps(&(*fdata2_iter), accum);
#else
										accum[0] += fdata_kernel_iter[0] * *kernel_iter,
										accum[1] += fdata_kernel_iter[1] * *kernel_iter,
										accum[2] += fdata_kernel_iter[2] * *kernel_iter,
										accum[3] += fdata_kernel_iter[3] * *kernel_iter;
									fdata2_iter[0] = accum[0],
									fdata2_iter[1] = accum[1],
									fdata2_iter[2] = accum[2],
									fdata2_iter[3] = accum[3];
#endif
								}
						};
						break;
				}
			RUN_THREADED_TASK
		}
		// Run threads for vertical blur
		if(!kernel_v.empty()){
			// Helper values for faster processing in threads
			const int fdata_jump = -fdata.size() + (1 + remote_threads_n) * (trimmed_stride / width),
				data_jump = -(height * stride) + (fdata_jump + fdata.size());
			const unsigned kernel_v_radius = ((kernel_v.size() - 1) >> 1) * trimmed_stride;
			// Select proper vertical blur function
			decltype(fdata)& fdata = kernel_h.empty() ? fdata : fdata2;
			switch(depth){
				case ColorDepth::X1:
					blur_task = [&](const unsigned thread_i){
						unsigned char* pdata = data + thread_i;
						float accum;
						for(decltype(fdata.begin()) fdata_iter = fdata.begin() + thread_i, fdata_iter_end = fdata.begin() + trimmed_stride, fdata_iter_col_first, fdata_iter_col_end, fdata_kernel_iter, fdata_kernel_iter_end, kernel_iter;
							fdata_iter < fdata_iter_end;
							fdata_iter += fdata_jump, pdata += data_jump)
							for(fdata_iter_col_first = fdata_iter, fdata_iter_col_end = fdata_iter + fdata.size(); fdata_iter != fdata_iter_col_end; fdata_iter += trimmed_stride, pdata += stride){
								for(accum = 0, fdata_kernel_iter = std::max(fdata_iter - kernel_v_radius, fdata_iter_col_first), fdata_kernel_iter_end = std::min(fdata_iter_col_end, fdata_iter + kernel_v_radius + trimmed_stride), kernel_iter = kernel_v.begin() + std::max(0, fdata_iter_col_first - (fdata_iter - kernel_v_radius)) / trimmed_stride; fdata_kernel_iter != fdata_kernel_iter_end; fdata_kernel_iter += trimmed_stride, ++kernel_iter)
									accum += *fdata_kernel_iter * *kernel_iter;
								*pdata = accum;
							}
					};
					break;
				case ColorDepth::X3:
					blur_task = [&](const unsigned thread_i){
						unsigned char* pdata = data + thread_i * 3;
#ifdef __SSE2__
						__m128 accum;
						unsigned char tmp[4];
#else
						float accum[3];
#endif
						for(decltype(fdata.begin()) fdata_iter = fdata.begin() + thread_i * 3, fdata_iter_end = fdata.begin() + trimmed_stride, fdata_iter_col_first, fdata_iter_col_end, fdata_kernel_iter, fdata_kernel_iter_end, kernel_iter;
							fdata_iter < fdata_iter_end;
							fdata_iter += fdata_jump, pdata += data_jump)
							for(fdata_iter_col_first = fdata_iter, fdata_iter_col_end = fdata_iter + fdata.size(); fdata_iter != fdata_iter_col_end; fdata_iter += trimmed_stride, pdata += stride){
								for(
#ifdef __SSE2__
									accum = _mm_xor_ps(accum, accum),
#else
									accum[0] = accum[1] = accum[2] = 0,
#endif
									fdata_kernel_iter = std::max(fdata_iter - kernel_v_radius, fdata_iter_col_first), fdata_kernel_iter_end = std::min(fdata_iter_col_end, fdata_iter + kernel_v_radius + trimmed_stride), kernel_iter = kernel_v.begin() + std::max(0, fdata_iter_col_first - (fdata_iter - kernel_v_radius)) / trimmed_stride; fdata_kernel_iter != fdata_kernel_iter_end; fdata_kernel_iter += trimmed_stride, ++kernel_iter)
#ifdef __SSE2__
									accum = _mm_add_ps(
										accum,
										_mm_mul_ps(
											_mm_movelh_ps(
												_mm_castpd_ps(_mm_load_sd(reinterpret_cast<double*>(&(*fdata_kernel_iter)))),
												_mm_load_ss(&fdata_kernel_iter[2])
											),
											_mm_set1_ps(*kernel_iter)
										)
									);
								SSE2_STORE_PS_U8(tmp, accum),
								pdata[0] = tmp[0],
								pdata[1] = tmp[1],
								pdata[2] = tmp[2];
#else
									accum[0] += fdata_kernel_iter[0] * *kernel_iter,
									accum[1] += fdata_kernel_iter[1] * *kernel_iter,
									accum[2] += fdata_kernel_iter[2] * *kernel_iter;
								pdata[0] = accum[0],
								pdata[1] = accum[1],
								pdata[2] = accum[2];
#endif
							}
					};
					break;
				case ColorDepth::X4:
					blur_task = [&](const unsigned thread_i){
						unsigned char* pdata = data + (thread_i << 2);
#ifdef __SSE2__
						__m128 accum;
#else
						float accum[4];
#endif
						for(decltype(fdata.begin()) fdata_iter = fdata.begin() + (thread_i << 2), fdata_iter_end = fdata.begin() + trimmed_stride, fdata_iter_col_first, fdata_iter_col_end, fdata_kernel_iter, fdata_kernel_iter_end, kernel_iter;
							fdata_iter < fdata_iter_end;
							fdata_iter += fdata_jump, pdata += data_jump)
							for(fdata_iter_col_first = fdata_iter, fdata_iter_col_end = fdata_iter + fdata.size(); fdata_iter != fdata_iter_col_end; fdata_iter += trimmed_stride, pdata += stride){
								for(
#ifdef __SSE2__
									accum = _mm_xor_ps(accum, accum),
#else
									accum[0] = accum[1] = accum[2] = accum[3] = 0,
#endif
									fdata_kernel_iter = std::max(fdata_iter - kernel_v_radius, fdata_iter_col_first), fdata_kernel_iter_end = std::min(fdata_iter_col_end, fdata_iter + kernel_v_radius + trimmed_stride), kernel_iter = kernel_v.begin() + std::max(0, fdata_iter_col_first - (fdata_iter - kernel_v_radius)) / trimmed_stride; fdata_kernel_iter != fdata_kernel_iter_end; fdata_kernel_iter += trimmed_stride, ++kernel_iter)
#ifdef __SSE2__
									accum = _mm_add_ps(
										accum,
										_mm_mul_ps(
											_mm_load_ps(&(*fdata_kernel_iter)),
											_mm_set1_ps(*kernel_iter)
										)
									);
								SSE2_STORE_PS_U8(pdata, accum);
#else
									accum[0] += fdata_kernel_iter[0] * *kernel_iter,
									accum[1] += fdata_kernel_iter[1] * *kernel_iter,
									accum[2] += fdata_kernel_iter[2] * *kernel_iter,
									accum[3] += fdata_kernel_iter[3] * *kernel_iter;
								pdata[0] = accum[0],
								pdata[1] = accum[1],
								pdata[2] = accum[2],
								pdata[3] = accum[3];
#endif
							}
					};
					break;
			}
			RUN_THREADED_TASK
		}
	}
}
