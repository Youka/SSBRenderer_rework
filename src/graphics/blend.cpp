/*
Project: SSBRenderer
File: blend.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "gutils.hpp"
#include <algorithm>
#include <cstdint>
#include <cmath>
#include "simd.hpp"

namespace GUtils{
	bool blend(const unsigned char* src_data, unsigned src_width, unsigned src_height, const unsigned src_stride, const bool src_with_alpha,
		unsigned char* dst_data, const unsigned dst_width, const unsigned dst_height, const unsigned dst_stride, const bool dst_with_alpha,
		const int dst_x, const int dst_y, const BlendOp op){
		// Anything to overlay?
		if(dst_x >= static_cast<int>(dst_width) || dst_y >= static_cast<int>(dst_height) || dst_x+src_width <= 0 || dst_y+src_height <= 0)
			return false;
		// Update source to overlay rectangle
		if(dst_x < 0)
			src_data += src_with_alpha ? -dst_x << 2 : (-dst_x << 1) - dst_x,
			src_width += dst_x;
		if(dst_y < 0)
			src_data += -dst_y * src_stride,
			src_height += dst_y;
		if(dst_x + src_width > dst_width)
			src_width = dst_width - dst_x;
		if(dst_y + src_height > dst_height)
			src_height = dst_height - dst_y;
		// Update destination origin for source overlay
		if(dst_x > 0)
			dst_data += dst_with_alpha ? dst_x << 2 : (dst_x << 1) + dst_x;
		if(dst_y > 0)
			dst_data += dst_y * dst_stride;
		// Data iteration stop pointers
		const unsigned char* src_row_end;
		const unsigned char* const src_data_end = src_data + src_height * src_stride;
		// Offset after row
		const unsigned src_offset = src_stride - (src_with_alpha ? src_width << 2 : (src_width << 1) + src_width),
			dst_offset = dst_stride - (dst_with_alpha ? src_width << 2 : (src_width << 1) + src_width);
		// Often used temporaries
		unsigned char inv_alpha, inv_alpha2,	// Inverted alpha
			tmp[16],	// Storage for non-2^n unpack
			*ptmp = tmp;	// Just to make the compiler stop crying about strict-aliasing
		// Blend by operation
		switch(op){
			// DST = SRC
			case BlendOp::SOURCE:
				if(src_with_alpha && dst_with_alpha)
					while(src_data != src_data_end)
						std::copy(src_data, src_data + (src_width << 2), dst_data),
						src_data += src_stride,
						dst_data += dst_stride;
				else if(!src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end)
						std::copy(src_data, src_data + (src_width << 1) + src_width, dst_data),
						src_data += src_stride,
						dst_data += dst_stride;
				else if(src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end)
							*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(src_data),
							src_data += 2,
							dst_data += 2,
							*dst_data++ = *src_data,
							src_data += 2;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end)
							*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(src_data),
							src_data += 2,
							dst_data += 2,
							*dst_data++ = *src_data++,
							*dst_data++ = 255;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				break;
			// DST = SRC + DST * ~SRCa
			case BlendOp::OVER:
				if(src_with_alpha && dst_with_alpha)
					while(src_data != src_data_end){
#ifdef __SSE2__
						src_row_end = src_data + ((src_width & ~0x3) << 2);
						while(src_data != src_row_end){
							if(src_data[3] == 255 && src_data[7] == 255 && src_data[11] == 255 && src_data[15] == 255)
								_mm_storeu_si128(reinterpret_cast<__m128i*>(dst_data), _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_data)));
							else if(src_data[3] > 0 || src_data[7] > 0 || src_data[11] > 0 || src_data[15] > 0){
								__m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_data)),
									b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(dst_data));
								SSE2_STORE_2X16_U8(
									dst_data,
									_mm_add_epi16(
										_mm_unpacklo_epi8(a, _mm_setzero_si128()),
										SSE2_DIV255_U16(
											_mm_mullo_epi16(
												_mm_unpacklo_epi8(b, _mm_setzero_si128()),
												SSE2_SET2_U16(src_data[7] ^ 0xFF, src_data[3] ^ 0xFF)
											)
										)
									),
									_mm_add_epi16(
										_mm_unpackhi_epi8(a, _mm_setzero_si128()),
										SSE2_DIV255_U16(
											_mm_mullo_epi16(
												_mm_unpackhi_epi8(b, _mm_setzero_si128()),
												SSE2_SET2_U16(src_data[15] ^ 0xFF, src_data[11] ^ 0xFF)
											)
										)
									)
								);
							}
							src_data += 16,
							dst_data += 16;
						}
						if(src_width & 0x2){
							if(src_data[3] == 255 && src_data[7] == 255)
								*reinterpret_cast<int64_t*>(dst_data) = *reinterpret_cast<const int64_t*>(src_data);
							else if(src_data[3] > 0 || src_data[7] > 0)
								SSE2_STORE_16_U8(
									dst_data,
									_mm_add_epi16(
										SSE2_LOAD_U8_16(src_data),
										SSE2_DIV255_U16(
											_mm_mullo_epi16(
												SSE2_LOAD_U8_16(dst_data),
												SSE2_SET2_U16(src_data[7] ^ 0xFF, src_data[3] ^ 0xFF)
											)
										)
									)
								);
							src_data += 8,
							dst_data += 8;
						}
						if(src_width & 0x1){
							if(src_data[3] == 255)
								*reinterpret_cast<int32_t*>(dst_data) = *reinterpret_cast<const int32_t*>(src_data);
							else if(src_data[3] > 0)
								MMX_STORE_16_U8(
									dst_data,
									_mm_add_pi16(
										MMX_LOAD_U8_16(src_data),
										MMX_DIV255_U16(
											_mm_mullo_pi16(
												MMX_LOAD_U8_16(dst_data),
												_mm_set1_pi16(src_data[3] ^ 0xFF)
											)
										)
									)
								);
							src_data += 4,
							dst_data += 4;
						}
#else
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] == 255)
								*reinterpret_cast<int32_t*>(dst_data) = *reinterpret_cast<const int32_t*>(src_data);
							else if(src_data[3] > 0)
								inv_alpha = src_data[3] ^ 0xFF,
								dst_data[0] = src_data[0] + dst_data[0] * inv_alpha / 255,
								dst_data[1] = src_data[1] + dst_data[1] * inv_alpha / 255,
								dst_data[2] = src_data[2] + dst_data[2] * inv_alpha / 255,
								dst_data[3] = src_data[3] + dst_data[3] * inv_alpha / 255;
							src_data += 4,
							dst_data += 4;
						}
#endif
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(!src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end)
						std::copy(src_data, src_data + (src_width << 1) + src_width, dst_data),
						src_data += src_stride,
						dst_data += dst_stride;
				else if(src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
#ifdef __SSE2__
						src_row_end = src_data + ((src_width & ~0x1) << 2);
						while(src_data != src_row_end){
							if(src_data[3] == 255 && src_data[7] == 255)
								*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(src_data),
								dst_data[2] = src_data[2],
								*reinterpret_cast<int16_t*>(dst_data+3) = *reinterpret_cast<const int16_t*>(src_data+4),
								dst_data[5] = src_data[6];
							else if(src_data[3] > 0 || src_data[7] > 0)
								SSE2_STORE_16_U8(
									ptmp,
									_mm_add_epi16(
										SSE2_LOAD_U8_16(src_data),
										SSE2_DIV255_U16(
											_mm_mullo_epi16(
												_mm_set_epi16(0, dst_data[5], dst_data[4], dst_data[3], 0, dst_data[2], dst_data[1], dst_data[0]),
												SSE2_SET2_U16(src_data[7] ^ 0xFF, src_data[3] ^ 0xFF)
											)
										)
									)
								),
								*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(ptmp),
								dst_data[2] = tmp[2],
								*reinterpret_cast<int16_t*>(dst_data+3) = *reinterpret_cast<const int16_t*>(ptmp+4),
								dst_data[5] = tmp[6];
							src_data += 8,
							dst_data += 6;
						}
						if(src_width & 0x1){
							if(src_data[3] == 255)
								*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(src_data),
								dst_data[2] = src_data[2];
							else if(src_data[3] > 0)
								MMX_STORE_16_U8(
									ptmp,
									_mm_add_pi16(
										MMX_LOAD_U8_16(src_data),
										MMX_DIV255_U16(
											_mm_mullo_pi16(
												_mm_set_pi16(0, dst_data[2], dst_data[1], dst_data[0]),
												_mm_set1_pi16(src_data[3] ^ 0xFF)
											)
										)
									)
								),
								*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(ptmp),
								dst_data[2] = tmp[2];
							src_data += 4,
							dst_data += 3;
						}
#else
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] == 255)
								*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(src_data),
								dst_data[2] = src_data[2];
							else if(src_data[3] > 0)
								inv_alpha = src_data[3] ^ 0xFF,
								dst_data[0] = src_data[0] + dst_data[0] * inv_alpha / 255,
								dst_data[1] = src_data[1] + dst_data[1] * inv_alpha / 255,
								dst_data[2] = src_data[2] + dst_data[2] * inv_alpha / 255;
							src_data += 4,
							dst_data += 3;
						}
#endif
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end)
							*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(src_data),
							src_data += 2,
							dst_data += 2,
							*dst_data++ = *src_data++,
							*dst_data++ = 255;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				break;
			// DST = MIN(255, SRC + DST)
			case BlendOp::ADD:
				if(src_with_alpha && dst_with_alpha)
					while(src_data != src_data_end){
#ifdef __SSE2__
						src_row_end = src_data + ((src_width & ~0x3) << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0 || src_data[7] > 0)
								_mm_storeu_si128(
									reinterpret_cast<__m128i*>(dst_data),
									_mm_adds_epu8(
										_mm_loadu_si128(reinterpret_cast<const __m128i*>(dst_data)),
										_mm_loadu_si128(reinterpret_cast<const __m128i*>(src_data))
									)
								);
							src_data += 16,
							dst_data += 16;
						}
						if(src_width & 0x2){
							if(src_data[3] > 0)
								*reinterpret_cast<__m64*>(dst_data) = _mm_adds_pu8(
									_mm_set_pi32(*reinterpret_cast<const int*>(dst_data+4), *reinterpret_cast<const int*>(dst_data)),
									_mm_set_pi32(*reinterpret_cast<const int*>(src_data+4), *reinterpret_cast<const int*>(src_data))
								);
							src_data += 8,
							dst_data += 8;
						}
						if(src_width & 0x1){
							if(src_data[3] > 0)
								*reinterpret_cast<int*>(dst_data) = _mm_cvtsi64_si32(
									_mm_adds_pu8(
										_mm_cvtsi32_si64(*reinterpret_cast<const int*>(dst_data)),
										_mm_cvtsi32_si64(*reinterpret_cast<const int*>(src_data))
									)
								);
							src_data += 4,
							dst_data += 4;
						}
#else
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0)
								dst_data[0] = std::min(255, dst_data[0] + src_data[0]),
								dst_data[1] = std::min(255, dst_data[1] + src_data[1]),
								dst_data[2] = std::min(255, dst_data[2] + src_data[2]),
								dst_data[3] = std::min(255, dst_data[3] + src_data[3]);
							src_data += 4,
							dst_data += 4;
						}
#endif
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(!src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end)
							dst_data[0] = std::min(255, dst_data[0] + src_data[0]),
							dst_data[1] = std::min(255, dst_data[1] + src_data[1]),
							dst_data[2] = std::min(255, dst_data[2] + src_data[2]),
							src_data += 3,
							dst_data += 3;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0)
								dst_data[0] = std::min(255, dst_data[0] + src_data[0]),
								dst_data[1] = std::min(255, dst_data[1] + src_data[1]),
								dst_data[2] = std::min(255, dst_data[2] + src_data[2]);
							src_data += 4,
							dst_data += 3;
						}
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end)
							dst_data[0] = std::min(255, dst_data[0] + src_data[0]),
							dst_data[1] = std::min(255, dst_data[1] + src_data[1]),
							dst_data[2] = std::min(255, dst_data[2] + src_data[2]),
							dst_data[3] = 255,
							src_data += 3,
							dst_data += 4;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				break;
			// DST = MAX(0, DST - SRC)
			case BlendOp::SUB:
				if(src_with_alpha && dst_with_alpha)
					while(src_data != src_data_end){
#ifdef __SSE2__
						src_row_end = src_data + ((src_width & ~0x3) << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0 || src_data[7] > 0)
								_mm_storeu_si128(
									reinterpret_cast<__m128i*>(dst_data),
									_mm_subs_epu8(
										_mm_loadu_si128(reinterpret_cast<const __m128i*>(dst_data)),
										_mm_loadu_si128(reinterpret_cast<const __m128i*>(src_data))
									)
								);
							src_data += 16,
							dst_data += 16;
						}
						if(src_width & 0x2){
							if(src_data[3] > 0)
								*reinterpret_cast<__m64*>(dst_data) = _mm_subs_pu8(
									_mm_set_pi32(*reinterpret_cast<const int*>(dst_data+4), *reinterpret_cast<const int*>(dst_data)),
									_mm_set_pi32(*reinterpret_cast<const int*>(src_data+4), *reinterpret_cast<const int*>(src_data))
								);
							src_data += 8,
							dst_data += 8;
						}
						if(src_width & 0x1){
							if(src_data[3] > 0)
								*reinterpret_cast<int*>(dst_data) = _mm_cvtsi64_si32(
									_mm_subs_pu8(
										_mm_cvtsi32_si64(*reinterpret_cast<const int*>(dst_data)),
										_mm_cvtsi32_si64(*reinterpret_cast<const int*>(src_data))
									)
								);
							src_data += 4,
							dst_data += 4;
						}
#else
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0)
								dst_data[0] = std::max(0, dst_data[0] - src_data[0]),
								dst_data[1] = std::max(0, dst_data[1] - src_data[1]),
								dst_data[2] = std::max(0, dst_data[2] - src_data[2]),
								dst_data[3] = std::max(0, dst_data[3] - src_data[3]);
							src_data += 4,
							dst_data += 4;
						}
#endif
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(!src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end)
							dst_data[0] = std::max(0, dst_data[0] - src_data[0]),
							dst_data[1] = std::max(0, dst_data[1] - src_data[1]),
							dst_data[2] = std::max(0, dst_data[2] - src_data[2]),
							src_data += 3,
							dst_data += 3;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0)
								dst_data[0] = std::max(0, dst_data[0] - src_data[0]),
								dst_data[1] = std::max(0, dst_data[1] - src_data[1]),
								dst_data[2] = std::max(0, dst_data[2] - src_data[2]);
							src_data += 4,
							dst_data += 3;
						}
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end)
							dst_data[0] = std::max(0, dst_data[0] - src_data[0]),
							dst_data[1] = std::max(0, dst_data[1] - src_data[1]),
							dst_data[2] = std::max(0, dst_data[2] - src_data[2]),
							dst_data[3] = 0,
							src_data += 3,
							dst_data += 4;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				break;
			/*
			DSTrgb = (DSTrgb / DSTa) * (SRCrgb / SRCa) * SRCa + DSTrgb * ~SRCa
			DSTa = SRCa + DSTa * ~SRCa
			*/
			case BlendOp::MUL:
				if(src_with_alpha && dst_with_alpha)
					while(src_data != src_data_end){
#ifdef __SSE2__
						src_row_end = src_data + ((src_width & ~0x1) << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0 || src_data[7] > 0){
								if(dst_data[3] == 0)
									SSE2_STORE_16_U8(
										dst_data,
										SSE2_DIV255_U16(
											_mm_mullo_epi16(
												SSE2_LOAD_U8_16(dst_data),
												SSE2_SET2_U16(src_data[7] ^ 0xFF, src_data[3] ^ 0xFF)
											)
										)
									),
									dst_data[3] = src_data[3],
									dst_data[7] = src_data[7];
								else if(src_data[3] == 255 && dst_data[3] == 255)
									SSE2_STORE_16_U8(
										dst_data,
										SSE2_DIV255_U16(
											_mm_mullo_epi16(
												SSE2_LOAD_U8_16(dst_data),
												SSE2_LOAD_U8_16(src_data)
											)
										)
									);
								else{
									__m128i xmm0 = SSE2_LOAD_U8_16(dst_data),
										xmm1 = SSE2_SET2_U16(src_data[7], src_data[3]);
									inv_alpha = src_data[3] ^ 0xFF,
									inv_alpha2 = src_data[7] ^ 0xFF,
									SSE2_STORE_16_U8(
										dst_data,
										_mm_add_epi16(
											SSE2_DIV255_U16(
												_mm_mullo_epi16(
													SSE2_DIV255_U16(
														_mm_mullo_epi16(
															SSE2_DIV_U16(
																SSE2_MUL255_U16_UNSAFE(
																	xmm0
																),
																SSE2_SET2_U16(dst_data[7], dst_data[3])
															),
															SSE2_DIV_U16(
																SSE2_MUL255_U16_UNSAFE(
																	SSE2_LOAD_U8_16(src_data)
																),
																xmm1
															)
														)
													),
													xmm1
												)
											),
											SSE2_DIV255_U16(
												_mm_mullo_epi16(
													xmm0,
													SSE2_SET2_U16(inv_alpha2, inv_alpha)
												)
											)
										)
									),
									dst_data[3] = src_data[3] + dst_data[3] * inv_alpha / 255,
									dst_data[7] = src_data[7] + dst_data[7] * inv_alpha2 / 255;
								}
							}
							src_data += 8,
							dst_data += 8;
						}
						if(src_width & 0x1){
							if(src_data[3] > 0){
								if(dst_data[3] == 0)
									MMX_STORE_16_U8(
										dst_data,
										MMX_DIV255_U16(
											_mm_mullo_pi16(
												MMX_LOAD_U8_16(dst_data),
												_mm_set1_pi16(src_data[3] ^ 0xFF)
											)
										)
									),
									dst_data[3] = src_data[3];
								else if(src_data[3] == 255 && dst_data[3] == 255)
									MMX_STORE_16_U8(
										dst_data,
										MMX_DIV255_U16(
											_mm_mullo_pi16(
												MMX_LOAD_U8_16(dst_data),
												MMX_LOAD_U8_16(src_data)
											)
										)
									);
								else{
									__m64 xmm0 = MMX_LOAD_U8_16(dst_data),
										xmm1 = _mm_set1_pi16(src_data[3]);
									inv_alpha = src_data[3] ^ 0xFF,
									MMX_STORE_16_U8(
										dst_data,
										_mm_add_pi16(
											MMX_DIV255_U16(
												_mm_mullo_pi16(
													MMX_DIV255_U16(
														_mm_mullo_pi16(
															MMX_DIV_U16(
																MMX_MUL255_U16_UNSAFE(
																	xmm0
																),
																_mm_set1_pi16(dst_data[3])
															),
															MMX_DIV_U16(
																MMX_MUL255_U16_UNSAFE(
																	MMX_LOAD_U8_16(src_data)
																),
																xmm1
															)
														)
													),
													xmm1
												)
											),
											MMX_DIV255_U16(
												_mm_mullo_pi16(
													xmm0,
													_mm_set1_pi16(inv_alpha)
												)
											)
										)
									),
									dst_data[3] = src_data[3] + dst_data[3] * inv_alpha / 255;
								}
							}
							src_data += 4,
							dst_data += 4;
						}
#else
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0){
								if(dst_data[3] == 0)
									inv_alpha = src_data[3] ^ 0xFF,
									dst_data[0] = dst_data[0] * inv_alpha / 255,
									dst_data[1] = dst_data[1] * inv_alpha / 255,
									dst_data[2] = dst_data[2] * inv_alpha / 255,
									dst_data[3] = src_data[3];
								else if(src_data[3] == 255 && dst_data[3] == 255)
									dst_data[0] = dst_data[0] * src_data[0] / 255,
									dst_data[1] = dst_data[1] * src_data[1] / 255,
									dst_data[2] = dst_data[2] * src_data[2] / 255;
								else
									inv_alpha = src_data[3] ^ 0xFF,
									dst_data[0] = (dst_data[0] * 255 / dst_data[3]) * (src_data[0] * 255 / src_data[3]) * src_data[3] / 65025 + dst_data[0] * inv_alpha / 255,
									dst_data[1] = (dst_data[1] * 255 / dst_data[3]) * (src_data[1] * 255 / src_data[3]) * src_data[3] / 65025 + dst_data[1] * inv_alpha / 255,
									dst_data[2] = (dst_data[2] * 255 / dst_data[3]) * (src_data[2] * 255 / src_data[3]) * src_data[3] / 65025 + dst_data[2] * inv_alpha / 255,
									dst_data[3] = src_data[3] + dst_data[3] * inv_alpha / 255;
							}
							src_data += 4,
							dst_data += 4;
						}
#endif
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(!src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end)
							dst_data[0] = dst_data[0] * src_data[0] / 255,
							dst_data[1] = dst_data[1] * src_data[1] / 255,
							dst_data[2] = dst_data[2] * src_data[2] / 255,
							src_data += 3,
							dst_data += 3;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0){
								if(src_data[3] == 255)
									dst_data[0] = dst_data[0] * src_data[0] / 255,
									dst_data[1] = dst_data[1] * src_data[1] / 255,
									dst_data[2] = dst_data[2] * src_data[2] / 255;
								else
									inv_alpha = src_data[3] ^ 0xFF,
									dst_data[0] = dst_data[0] * (src_data[0] * 255 / src_data[3]) * src_data[3] / 65025 + dst_data[0] * inv_alpha / 255,
									dst_data[1] = dst_data[1] * (src_data[1] * 255 / src_data[3]) * src_data[3] / 65025 + dst_data[1] * inv_alpha / 255,
									dst_data[2] = dst_data[2] * (src_data[2] * 255 / src_data[3]) * src_data[3] / 65025 + dst_data[2] * inv_alpha / 255;
							}
							src_data += 4,
							dst_data += 3;
						}
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end){
							if(dst_data[3] == 0)
								*reinterpret_cast<int16_t*>(dst_data) = 0,
								dst_data[2] = 0,
								dst_data[3] = 255;
							else if(dst_data[3] == 255)
								dst_data[0] = dst_data[0] * src_data[0] / 255,
								dst_data[1] = dst_data[1] * src_data[1] / 255,
								dst_data[2] = dst_data[2] * src_data[2] / 255;
							else
								dst_data[0] = (dst_data[0] * 255 / dst_data[3]) * src_data[0] / 255,
								dst_data[1] = (dst_data[1] * 255 / dst_data[3]) * src_data[1] / 255,
								dst_data[2] = (dst_data[2] * 255 / dst_data[3]) * src_data[2] / 255,
								dst_data[3] = 255;
							src_data += 3,
							dst_data += 4;
						}
						src_data += src_offset,
						dst_data += dst_offset;
					}
				break;
			/*
			DSTrgb = ~(~(DSTrgb / DSTa) * ~(SRCrgb / SRCa)) * SRCa + DSTrgb * ~SRCa
			DSTa = SRCa + DSTa * ~SRCa
			*/
			case BlendOp::SCR:
				if(src_with_alpha && dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0){
								if(dst_data[3] == 0)
									inv_alpha = src_data[3] ^ 0xFF,
									dst_data[0] = src_data[0] + dst_data[0] * inv_alpha / 255,
									dst_data[1] = src_data[1] + dst_data[1] * inv_alpha / 255,
									dst_data[2] = src_data[2] + dst_data[2] * inv_alpha / 255,
									dst_data[3] = src_data[3];
								else if(src_data[3] == 255 && dst_data[3] == 255)
									dst_data[0] = (dst_data[0] ^ 0xFF) * (src_data[0] ^ 0xFF) / 255 ^ 0xFF,
									dst_data[1] = (dst_data[1] ^ 0xFF) * (src_data[1] ^ 0xFF) / 255 ^ 0xFF,
									dst_data[2] = (dst_data[2] ^ 0xFF) * (src_data[2] ^ 0xFF) / 255 ^ 0xFF;
								else
									inv_alpha = src_data[3] ^ 0xFF,
									dst_data[0] = ((dst_data[0] * 255 / dst_data[3] ^ 0xFF) * (src_data[0] * 255 / src_data[3] ^ 0xFF) / 255 ^ 0xFF) * src_data[3] / 255 + dst_data[0] * inv_alpha / 255,
									dst_data[1] = ((dst_data[1] * 255 / dst_data[3] ^ 0xFF) * (src_data[1] * 255 / src_data[3] ^ 0xFF) / 255 ^ 0xFF) * src_data[3] / 255 + dst_data[1] * inv_alpha / 255,
									dst_data[2] = ((dst_data[2] * 255 / dst_data[3] ^ 0xFF) * (src_data[2] * 255 / src_data[3] ^ 0xFF) / 255 ^ 0xFF) * src_data[3] / 255 + dst_data[2] * inv_alpha / 255,
									dst_data[3] = src_data[3] + dst_data[3] * inv_alpha / 255;
							}
							src_data += 4,
							dst_data += 4;
						}
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(!src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end)
							dst_data[0] = (dst_data[0] ^ 0xFF) * (src_data[0] ^ 0xFF) / 255 ^ 0xFF,
							dst_data[1] = (dst_data[1] ^ 0xFF) * (src_data[1] ^ 0xFF) / 255 ^ 0xFF,
							dst_data[2] = (dst_data[2] ^ 0xFF) * (src_data[2] ^ 0xFF) / 255 ^ 0xFF,
							src_data += 3,
							dst_data += 3;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end)
							if(src_data[3] > 0){
								if(src_data[3] == 255)
									dst_data[0] = (dst_data[0] ^ 0xFF) * (src_data[0] ^ 0xFF) / 255 ^ 0xFF,
									dst_data[1] = (dst_data[1] ^ 0xFF) * (src_data[1] ^ 0xFF) / 255 ^ 0xFF,
									dst_data[2] = (dst_data[2] ^ 0xFF) * (src_data[2] ^ 0xFF) / 255 ^ 0xFF;
								else
									inv_alpha = src_data[3] ^ 0xFF,
									dst_data[0] = ((dst_data[0] ^ 0xFF) * (src_data[0] * 255 / src_data[3] ^ 0xFF) / 255 ^ 0xFF) * src_data[3] / 255 + dst_data[0] * inv_alpha / 255,
									dst_data[1] = ((dst_data[1] ^ 0xFF) * (src_data[1] * 255 / src_data[3] ^ 0xFF) / 255 ^ 0xFF) * src_data[3] / 255 + dst_data[1] * inv_alpha / 255,
									dst_data[2] = ((dst_data[2] ^ 0xFF) * (src_data[2] * 255 / src_data[3] ^ 0xFF) / 255 ^ 0xFF) * src_data[3] / 255 + dst_data[2] * inv_alpha / 255;
							}
							src_data += 4,
							dst_data += 3;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end){
							if(dst_data[3] == 0)
								*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(src_data),
								dst_data[2] = src_data[2],
								dst_data[3] = 255;
							else if(dst_data[3] == 255)
								dst_data[0] = (dst_data[0] ^ 0xFF) * (src_data[0] ^ 0xFF) / 255 ^ 0xFF,
								dst_data[1] = (dst_data[1] ^ 0xFF) * (src_data[1] ^ 0xFF) / 255 ^ 0xFF,
								dst_data[2] = (dst_data[2] ^ 0xFF) * (src_data[2] ^ 0xFF) / 255 ^ 0xFF;
							else
								inv_alpha = 0,
								dst_data[0] = (dst_data[0] * 255 / dst_data[3] ^ 0xFF) * (src_data[0] ^ 0xFF) / 255 ^ 0xFF,
								dst_data[1] = (dst_data[1] * 255 / dst_data[3] ^ 0xFF) * (src_data[1] ^ 0xFF) / 255 ^ 0xFF,
								dst_data[2] = (dst_data[2] * 255 / dst_data[3] ^ 0xFF) * (src_data[2] ^ 0xFF) / 255 ^ 0xFF,
								dst_data[3] = 255;
							src_data += 3,
							dst_data += 4;
						}
						src_data += src_offset,
						dst_data += dst_offset;
					}
				break;
			/*
			DSTrgb = ABS((DSTrgb / DSTa) - (SRCrgb / SRCa)) * SRCa + DSTrgb * ~SRCa
			DSTa = SRCa + DSTa * ~SRCa
			*/
			case BlendOp::DIFF:
				if(src_with_alpha && dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0){
								if(dst_data[3] == 0)
									inv_alpha = src_data[3] ^ 0xFF,
									dst_data[0] = src_data[0] + dst_data[0] * inv_alpha / 255,
									dst_data[1] = src_data[1] + dst_data[1] * inv_alpha / 255,
									dst_data[2] = src_data[2] + dst_data[2] * inv_alpha / 255,
									dst_data[3] = src_data[3];
								else if(src_data[3] == 255 && dst_data[3] == 255)
									dst_data[0] = ::abs(dst_data[0] - src_data[0]),
									dst_data[1] = ::abs(dst_data[1] - src_data[1]),
									dst_data[2] = ::abs(dst_data[2] - src_data[2]);
								else
									inv_alpha = src_data[3] ^ 0xFF,
									dst_data[0] = ::abs(dst_data[0] * 255 / dst_data[3] - src_data[0] * 255 / src_data[3]) * src_data[3] / 255 + dst_data[0] * inv_alpha / 255,
									dst_data[1] = ::abs(dst_data[1] * 255 / dst_data[3] - src_data[1] * 255 / src_data[3]) * src_data[3] / 255 + dst_data[1] * inv_alpha / 255,
									dst_data[2] = ::abs(dst_data[2] * 255 / dst_data[3] - src_data[2] * 255 / src_data[3]) * src_data[3] / 255 + dst_data[2] * inv_alpha / 255,
									dst_data[3] = src_data[3] + dst_data[3] * inv_alpha / 255;
							}
							src_data += 4,
							dst_data += 4;
						}
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(!src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end)
							dst_data[0] = ::abs(dst_data[0] - src_data[0]),
							dst_data[1] = ::abs(dst_data[1] - src_data[1]),
							dst_data[2] = ::abs(dst_data[2] - src_data[2]),
							src_data += 3,
							dst_data += 3;
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else if(src_with_alpha && !dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] > 0){
								if(src_data[3] == 255)
									dst_data[0] = ::abs(dst_data[0] - src_data[0]),
									dst_data[1] = ::abs(dst_data[1] - src_data[1]),
									dst_data[2] = ::abs(dst_data[2] - src_data[2]);
								else
									inv_alpha = src_data[3] ^ 0xFF,
									dst_data[0] = ::abs(dst_data[0] - src_data[0] * 255 / src_data[3]) * src_data[3] / 255 + dst_data[0] * inv_alpha / 255,
									dst_data[1] = ::abs(dst_data[1] - src_data[1] * 255 / src_data[3]) * src_data[3] / 255 + dst_data[1] * inv_alpha / 255,
									dst_data[2] = ::abs(dst_data[2] - src_data[2] * 255 / src_data[3]) * src_data[3] / 255 + dst_data[2] * inv_alpha / 255;
							}
							src_data += 4,
							dst_data += 3;
						}
						src_data += src_offset,
						dst_data += dst_offset;
					}
				else
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 1) + src_width;
						while(src_data != src_row_end){
							if(dst_data[3] == 0)
								*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(src_data),
								dst_data[2] = src_data[2],
								dst_data[3] = 255;
							else if(dst_data[3] == 255)
								dst_data[0] = ::abs(dst_data[0] - src_data[0]),
								dst_data[1] = ::abs(dst_data[1] - src_data[1]),
								dst_data[2] = ::abs(dst_data[2] - src_data[2]);
							else
								inv_alpha = 0,
								dst_data[0] = ::abs(dst_data[0] * 255 / dst_data[3] - src_data[0]),
								dst_data[1] = ::abs(dst_data[1] * 255 / dst_data[3] - src_data[1]),
								dst_data[2] = ::abs(dst_data[2] * 255 / dst_data[3] - src_data[2]),
								dst_data[3] = 255;
							src_data += 3,
							dst_data += 4;
						}
						src_data += src_offset,
						dst_data += dst_offset;
					}
				break;
		}
		return true;
	}
}
