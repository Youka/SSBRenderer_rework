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
		// Blend by operation
		switch(op){
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
			case BlendOp::OVER:


				// TODO: change following instructions for both color formats


#ifdef __SSE2__
#define OVER_CALC16 \
	if(src_data[3] == 255 && src_data[7] == 255 && src_data[11] == 255 && src_data[15] == 255) \
		_mm_storeu_si128(reinterpret_cast<__m128i*>(dst_data), _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_data))); \
	else if(src_data[3] > 0 || src_data[7] > 0 || src_data[11] > 0 || src_data[15] > 0){ \
		__m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_data)), \
			b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(dst_data)); \
		SSE2_STORE_2X16_U8( \
			dst_data, \
			_mm_add_epi16( \
				_mm_unpacklo_epi8(a, _mm_setzero_si128()), \
				SSE2_DIV255_U16( \
					_mm_mullo_epi16( \
						_mm_unpacklo_epi8(b, _mm_setzero_si128()), \
						SSE2_SET2_U16(src_data[7] ^ 0xFF, src_data[3] ^ 0xFF) \
					) \
				) \
			), \
			_mm_add_epi16( \
				_mm_unpackhi_epi8(a, _mm_setzero_si128()), \
				SSE2_DIV255_U16( \
					_mm_mullo_epi16( \
						_mm_unpackhi_epi8(b, _mm_setzero_si128()), \
						SSE2_SET2_U16(src_data[15] ^ 0xFF, src_data[11] ^ 0xFF) \
					) \
				) \
			) \
		); \
	}
#define OVER_CALC8 \
	if(src_data[3] == 255 && src_data[7] == 255) \
		*reinterpret_cast<int64_t*>(dst_data) = *reinterpret_cast<const int64_t*>(src_data); \
	else if(src_data[3] > 0 || src_data[7] > 0) \
		SSE2_STORE_16_U8( \
			dst_data, \
			_mm_add_epi16( \
				SSE2_LOAD_U8_16(src_data), \
				SSE2_DIV255_U16( \
					_mm_mullo_epi16( \
						SSE2_LOAD_U8_16(dst_data), \
						SSE2_SET2_U16(src_data[7] ^ 0xFF, src_data[3] ^ 0xFF) \
					) \
				) \
			) \
		);
#define OVER_CALC4 \
	if(src_data[3] == 255) \
		*reinterpret_cast<int32_t*>(dst_data) = *reinterpret_cast<const int32_t*>(src_data); \
	else if(src_data[3] > 0) \
		MMX_STORE_16_U8( \
			dst_data, \
			_mm_add_pi16( \
				MMX_LOAD_U8_16(src_data), \
				MMX_DIV255_U16( \
					_mm_mullo_pi16( \
						MMX_LOAD_U8_16(dst_data), \
						_mm_set1_pi16(src_data[3] ^ 0xFF) \
					) \
				) \
			) \
		);
				switch(src_width & 0x3){
					case 0x0:
						while(src_data != src_data_end){
							src_row_end = src_data + (src_width << 2);
							while(src_data != src_row_end){
								OVER_CALC16
								src_data += 16,
								dst_data += 16;
							}
							src_data += src_offset,
							dst_data += dst_offset;
						}
						break;
					case 0x1:
						while(src_data != src_data_end){
							src_row_end = src_data + ((src_width & ~0x1) << 2);
							while(src_data != src_row_end){
								OVER_CALC16
								src_data += 16,
								dst_data += 16;
							}
							OVER_CALC4
							src_data += 4 + src_offset,
							dst_data += 4 + dst_offset;
						}
						break;
					case 0x2:
						while(src_data != src_data_end){
							src_row_end = src_data + ((src_width & ~0x2) << 2);
							while(src_data != src_row_end){
								OVER_CALC16
								src_data += 16,
								dst_data += 16;
							}
							OVER_CALC8
							src_data += 8 + src_offset,
							dst_data += 8 + dst_offset;
						}
						break;
					case 0x3:
						while(src_data != src_data_end){
							src_row_end = src_data + ((src_width & ~0x3) << 2);
							while(src_data != src_row_end){
								OVER_CALC16
								src_data += 16,
								dst_data += 16;
							}
							OVER_CALC8
							src_data += 8,
							dst_data += 8;
							OVER_CALC4
							src_data += 4 + src_offset,
							dst_data += 4 + dst_offset;
						}
						break;
				}
#undef OVER_CALC4
#undef OVER_CALC8
#undef OVER_CALC16
#else
				if(src_with_alpha && dst_with_alpha)
					while(src_data != src_data_end){
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] == 255)
								*reinterpret_cast<int32_t*>(dst_data) = *reinterpret_cast<const int32_t*>(src_data);
							else if(src_data[3] > 0){
								unsigned char inv_alpha = src_data[3] ^ 0xFF;
								dst_data[0] = src_data[0] + dst_data[0] * inv_alpha / 255,
								dst_data[1] = src_data[1] + dst_data[1] * inv_alpha / 255,
								dst_data[2] = src_data[2] + dst_data[2] * inv_alpha / 255,
								dst_data[3] = src_data[3] + dst_data[3] * inv_alpha / 255;
							}
							src_data += 4,
							dst_data += 4;
						}
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
						src_row_end = src_data + (src_width << 2);
						while(src_data != src_row_end){
							if(src_data[3] == 255)
								*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(src_data),
								dst_data[2] = src_data[2];
							else if(src_data[3] > 0){
								unsigned char inv_alpha = src_data[3] ^ 0xFF;
								dst_data[0] = src_data[0] + dst_data[0] * inv_alpha / 255,
								dst_data[1] = src_data[1] + dst_data[1] * inv_alpha / 255,
								dst_data[2] = src_data[2] + dst_data[2] * inv_alpha / 255;
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
						while(src_data != src_row_end)
							*reinterpret_cast<int16_t*>(dst_data) = *reinterpret_cast<const int16_t*>(src_data),
							src_data += 2,
							dst_data += 2,
							*dst_data++ = *src_data++,
							*dst_data++ = 255;
						src_data += src_offset,
						dst_data += dst_offset;
					}
#endif
				break;
			case BlendOp::ADD: break;
			case BlendOp::SUB: break;
			case BlendOp::MUL: break;
			case BlendOp::SCR: break;
			case BlendOp::DIFF: break;
		}
		return true;
	}
}
