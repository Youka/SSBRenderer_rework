/*
Project: SSBRenderer
File: simd.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#ifdef __AVX__
	#include <immintrin.h>
#elif defined __SSE3__
	#include <pmmintrin.h>
	#define _mm_load1_pd _mm_loaddup_pd
	#define _mm_loadu_si128 _mm_lddqu_si128
#elif defined __SSE2__
	#include <emmintrin.h>
#endif

#define MMX_LOAD_U8_16(x) _mm_unpacklo_pi8(_mm_cvtsi32_si64(*reinterpret_cast<const int*>(x)), _mm_setzero_si64())
#define MMX_STORE_16_U8(mem, x) (*reinterpret_cast<int*>(mem) = _mm_cvtsi64_si32(_m_packuswb(x, _mm_setzero_si64())))
#define MMX_DIV255_U16(x) _mm_srli_pi16(_mm_mulhi_pu16(x, _mm_set1_pi16(static_cast<short>(0x8081))), 7)
#define MMX_MUL255_U16_UNSAFE(x) _mm_sub_pi16(_mm_slli_pi16(x, 8), x) // x mustn't be >255 (shift operation may cut away relevant bits)
#define MMX_ABSDIFF_U8(x1, x2) _mm_sub_pi8(_mm_max_pu8(x1, x2), _mm_min_pu8(x1, x2))
#define MMX_DIV_U16(x1, x2) _mm_cvtps_pi16(_mm_div_ps(_mm_cvtpu16_ps(x1), _mm_cvtpu16_ps(x2)))
#define MMX_INV_BITS(x) _mm_andnot_si64(x, x)

#define SSE2_LOAD_U8_16(x) _mm_unpacklo_epi8(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(x)), _mm_setzero_si128())
#define SSE2_STORE_16_U8(mem, x) _mm_storel_epi64(reinterpret_cast<__m128i*>(mem), _mm_packus_epi16(x, _mm_setzero_si128()))
#define SSE2_STORE_2X16_U8(mem, x1, x2) _mm_storeu_si128(reinterpret_cast<__m128i*>(mem), _mm_packus_epi16(x1, x2))
#define SSE2_SET2_U16(x2, x1) _mm_shufflehi_epi16(_mm_shufflelo_epi16(_mm_set_epi32(0, x2, 0, x1), 0x0), 0x0)
#define SSE2_DIV255_U16(x) _mm_srli_epi16(_mm_mulhi_epu16(x, _mm_set1_epi16(static_cast<short>(0x8081))), 7)
#define SSE2_MUL255_U16_UNSAFE(x) _mm_sub_epi16(_mm_slli_epi16(x, 8), x) // x mustn't be >255 (shift operation may cut away relevant bits)
#define SSE2_ABSDIFF_U8(x1, x2) _mm_sub_epi8(_mm_max_epu8(x1, x2), _mm_min_epu8(x1, x2))
#define SSE2_DIV_U16(x1, x2) \
	_mm_packs_epi32( \
		_mm_cvtps_epi32( \
			_mm_div_ps( \
				_mm_cvtepi32_ps(_mm_unpacklo_epi16(x1, _mm_setzero_si128())), \
				_mm_cvtepi32_ps(_mm_unpacklo_epi16(x2, _mm_setzero_si128())) \
			) \
		), \
		_mm_cvtps_epi32( \
			_mm_div_ps( \
				_mm_cvtepi32_ps(_mm_unpackhi_epi16(x1, _mm_setzero_si128())), \
				_mm_cvtepi32_ps(_mm_unpackhi_epi16(x2, _mm_setzero_si128())) \
			) \
		) \
	)
#define SSE2_INV_BITS(x) _mm_andnot_si128(x, x)

/*
Divide unsigned short by constant 255:
> x / 255
> DWORD(x * 0x8081) >> 0x17
> HWORD((x << 15) + (x << 7) + x) >> 7
*/
