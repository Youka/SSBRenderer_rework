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
#ifdef __SSE3__
	#include <pmmintrin.h>
	#define _mm_loadu_si128 _mm_lddqu_si128
#elif defined __SSE2__
	#include <emmintrin.h>
#endif

namespace GUtils{
	bool blend_rgba_rgba(const unsigned char* src_data, unsigned src_width, unsigned src_height, const unsigned src_stride,
		unsigned char* dst_data, const unsigned dst_width, const unsigned dst_height, const unsigned dst_stride,
		const int dst_x, const int dst_y, const BlendOp op){
		// Anything to overlay?
		if(dst_x >= static_cast<int>(dst_width) || dst_y >= static_cast<int>(dst_height) || dst_x+src_width <= 0 || dst_y+src_height <= 0)
			return false;
		// Update source to overlay rectangle
		if(dst_x < 0)
			src_data += -dst_x << 2,
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
			dst_data += dst_x << 2;
		if(dst_y > 0)
			dst_data += dst_y * dst_stride;
		// Data iteration stop pointers
		const unsigned char* src_row_end;
		const unsigned char* const src_data_end = src_data + src_height * src_stride;
		// Offset after row
		const unsigned src_offset = src_stride - (src_width << 2),
			dst_offset = dst_stride - (src_width << 2);
		// Blend by operation
		switch(op){
			case BlendOp::SOURCE:
				while(src_data < src_data_end)
					std::copy(src_data, src_data+(src_width << 2), dst_data),
					src_data += src_stride,
					dst_data += dst_stride;
				break;
			case BlendOp::OVER:
#ifdef __SSE2__
#define OVER_CALC16 \
	if(src_data[3] == 255 && src_data[7] == 255 && src_data[11] == 255 && src_data[15] == 255) \
		_mm_storeu_si128(reinterpret_cast<__m128i*>(dst_data), _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_data))); \
	else if(src_data[3] > 0 || src_data[7] > 0 || src_data[11] > 0 || src_data[15] > 0){ \
		__m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src_data)), \
			b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(dst_data)); \
		_mm_storeu_si128( \
			reinterpret_cast<__m128i*>(dst_data), \
			_mm_packus_epi16( \
				_mm_add_epi16( \
					_mm_unpacklo_epi8(a, _mm_setzero_si128()), \
					_mm_srli_epi16( \
						_mm_mulhi_epu16( \
							_mm_mullo_epi16( \
								_mm_unpacklo_epi8(b, _mm_setzero_si128()), \
								_mm_shufflehi_epi16(_mm_shufflelo_epi16(_mm_set_epi32(0, src_data[7] ^ 0xFF, 0, src_data[3] ^ 0xFF), 0x0), 0x0) \
							), \
							_mm_set1_epi16(static_cast<short>(0x8081)) \
						), \
						7 \
					) \
				), \
				_mm_add_epi16( \
					_mm_unpackhi_epi8(a, _mm_setzero_si128()), \
					_mm_srli_epi16( \
						_mm_mulhi_epu16( \
							_mm_mullo_epi16( \
								_mm_unpackhi_epi8(b, _mm_setzero_si128()), \
								_mm_shufflehi_epi16(_mm_shufflelo_epi16(_mm_set_epi32(0, src_data[15] ^ 0xFF, 0, src_data[11] ^ 0xFF), 0x0), 0x0) \
							), \
							_mm_set1_epi16(static_cast<short>(0x8081)) \
						), \
						7 \
					) \
				) \
			) \
		); \
	}
#define OVER_CALC8 \
	if(src_data[3] == 255 && src_data[7] == 255) \
		*reinterpret_cast<int64_t*>(dst_data) = *reinterpret_cast<const int64_t*>(src_data); \
	else if(src_data[3] > 0 || src_data[7] > 0) \
		_mm_storel_epi64( \
			reinterpret_cast<__m128i*>(dst_data), \
			_mm_packus_epi16( \
				_mm_add_epi16( \
					_mm_unpacklo_epi8(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(src_data)), _mm_setzero_si128()), \
					_mm_srli_epi16( \
						_mm_mulhi_epu16( \
							_mm_mullo_epi16( \
								_mm_unpacklo_epi8(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(dst_data)), _mm_setzero_si128()), \
								_mm_shufflehi_epi16(_mm_shufflelo_epi16(_mm_set_epi32(0, src_data[7] ^ 0xFF, 0, src_data[3] ^ 0xFF), 0x0), 0x0) \
							), \
							_mm_set1_epi16(static_cast<short>(0x8081)) \
						), \
						7 \
					) \
				), \
				_mm_setzero_si128() \
			) \
		);
#define OVER_CALC4 \
	if(src_data[3] == 255) \
		*reinterpret_cast<int32_t*>(dst_data) = *reinterpret_cast<const int32_t*>(src_data); \
	else if(src_data[3] > 0) \
		*reinterpret_cast<int*>(dst_data) = _m_to_int( \
			_m_packuswb( \
				_mm_add_pi16( \
					_mm_unpacklo_pi8(_m_from_int(*reinterpret_cast<const int*>(src_data)), _mm_setzero_si64()), \
					_mm_srli_pi16( \
						_mm_mulhi_pu16( \
							_mm_mullo_pi16( \
								_mm_unpacklo_pi8(_m_from_int(*reinterpret_cast<const int*>(dst_data)), _mm_setzero_si64()), \
								_mm_set1_pi16(src_data[3] ^ 0xFF) \
							), \
							_mm_set1_pi16(static_cast<short>(0x8081)) \
						), \
						7 \
					) \
				), \
				_mm_setzero_si64() \
			) \
		);
				switch(src_width & 0x3){
					case 0x0:
						while(src_data < src_data_end){
							src_row_end = src_data + (src_width << 2);
							while(src_data < src_row_end){
								OVER_CALC16
								src_data += 16,
								dst_data += 16;
							}
							src_data += src_offset,
							dst_data += dst_offset;
						}
						break;
					case 0x1:
						while(src_data < src_data_end){
							src_row_end = src_data + ((src_width & ~0x1) << 2);
							while(src_data < src_row_end){
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
						while(src_data < src_data_end){
							src_row_end = src_data + ((src_width & ~0x2) << 2);
							while(src_data < src_row_end){
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
						while(src_data < src_data_end){
							src_row_end = src_data + ((src_width & ~0x3) << 2);
							while(src_data < src_row_end){
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
				while(src_data < src_data_end){
					src_row_end = src_data + (src_width << 2);
					while(src_data < src_row_end){
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
