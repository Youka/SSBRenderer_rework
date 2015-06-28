# Include C source run checker
include(CheckCSourceRuns)

# Uses compiler to check SSE2 support from CPU
macro(CHECK_SSE2)
	if(MSVC)
		set(CMAKE_REQUIRED_FLAGS /arch:SSE2)
	else()
		set(CMAKE_REQUIRED_FLAGS -msse2)
	endif()
	CHECK_C_SOURCE_RUNS(
		"#include <emmintrin.h>
		int main(){
			__m128d m;
			_mm_xor_pd(m, m);
			return 0;
		}"
		HAS_SSE2)
endmacro()

# Uses compiler to check SSE3 support from CPU
macro(CHECK_SSE3)
	if(MSVC)
		set(CMAKE_REQUIRED_FLAGS /arch:SSE3)
	else()
		set(CMAKE_REQUIRED_FLAGS -msse3)
	endif()
	CHECK_C_SOURCE_RUNS(
		"#include <pmmintrin.h>
		int main(){
			__m128d m;
			_mm_hadd_pd(m, m);
			return 0;
		}"
		HAS_SSE3)
endmacro()

# Uses compiler to check AVX support from CPU
macro(CHECK_AVX)
	if(MSVC)
		set(CMAKE_REQUIRED_FLAGS /arch:AVX)
	else()
		set(CMAKE_REQUIRED_FLAGS -mavx)
	endif()
	CHECK_C_SOURCE_RUNS(
		"#include <immintrin.h>
		int main(){
			__m256d m;
			_mm256_xor_pd(m, m);
			return 0;
		}"
		HAS_AVX)
endmacro()