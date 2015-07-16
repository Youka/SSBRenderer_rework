/*
Project: SSBRenderer
File: matrix.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "gutils.hpp"
#include <algorithm>
#include "simd.hpp"
#include <cmath>

namespace GUtils{
	Matrix4x4d::Matrix4x4d(){this->identity();}
	Matrix4x4d::Matrix4x4d(const double* matrix){std::copy(matrix, matrix+16, this->matrix);}
	Matrix4x4d::Matrix4x4d(double x11, double x12, double x13, double x14,
				double x21, double x22, double x23, double x24,
				double x31, double x32, double x33, double x34,
				double x41, double x42, double x43, double x44){
		this->matrix[0] = x11, this->matrix[1] = x12, this->matrix[2] = x13, this->matrix[3] = x14,
		this->matrix[4] = x21, this->matrix[5] = x22, this->matrix[6] = x23, this->matrix[7] = x24,
		this->matrix[8] = x31, this->matrix[9] = x32, this->matrix[10] = x33, this->matrix[11] = x34,
		this->matrix[12] = x41, this->matrix[13] = x42, this->matrix[14] = x43, this->matrix[15] = x44;
	}
	Matrix4x4d::Matrix4x4d(const Matrix4x4d& other) : Matrix4x4d(other.matrix){}
	Matrix4x4d& Matrix4x4d::operator=(const Matrix4x4d& other){std::copy(other.matrix, other.matrix+16, this->matrix); return *this;}
	double* Matrix4x4d::data() const{return this->matrix;}
	double& Matrix4x4d::operator[](unsigned index) const{return this->matrix[index];}
	Matrix4x4d& Matrix4x4d::multiply(const Matrix4x4d& other, Matrix4x4d::Order order){
		double* matrix1, *matrix2;
#ifdef __SSE2__
		decltype(this->storage) storage;
		if(order == Matrix4x4d::Order::PREPEND)
			std::copy(this->matrix, this->matrix+16, matrix1 = reinterpret_cast<decltype(matrix1)>(&storage)), matrix2 = other.matrix;
		else
			matrix1 = other.matrix, std::copy(this->matrix, this->matrix+16, matrix2 = reinterpret_cast<decltype(matrix2)>(&storage));
#ifdef __AVX__
		__m256d m0 = _mm256_load_pd(matrix2),
		m1 = _mm256_load_pd(matrix2+4),
		m2 = _mm256_load_pd(matrix2+8),
		m3 = _mm256_load_pd(matrix2+12);
		_mm256_store_pd(
			this->matrix,
			_mm256_add_pd(
				_mm256_mul_pd(_mm256_broadcast_sd(matrix1), m0),
				_mm256_add_pd(
					_mm256_mul_pd(_mm256_broadcast_sd(matrix1+1), m1),
					_mm256_add_pd(
						_mm256_mul_pd(_mm256_broadcast_sd(matrix1+2), m2),
						_mm256_mul_pd(_mm256_broadcast_sd(matrix1+3), m3)
					)
				)
			)
		);
		_mm256_store_pd(
			this->matrix+4,
			_mm256_add_pd(
				_mm256_mul_pd(_mm256_broadcast_sd(matrix1+4), m0),
				_mm256_add_pd(
					_mm256_mul_pd(_mm256_broadcast_sd(matrix1+5), m1),
					_mm256_add_pd(
						_mm256_mul_pd(_mm256_broadcast_sd(matrix1+6), m2),
						_mm256_mul_pd(_mm256_broadcast_sd(matrix1+7), m3)
					)
				)
			)
		);
		_mm256_store_pd(
			this->matrix+8,
			_mm256_add_pd(
				_mm256_mul_pd(_mm256_broadcast_sd(matrix1+8), m0),
				_mm256_add_pd(
					_mm256_mul_pd(_mm256_broadcast_sd(matrix1+9), m1),
					_mm256_add_pd(
						_mm256_mul_pd(_mm256_broadcast_sd(matrix1+10), m2),
						_mm256_mul_pd(_mm256_broadcast_sd(matrix1+11), m3)
					)
				)
			)
		);
		_mm256_store_pd(
			this->matrix+12,
			_mm256_add_pd(
				_mm256_mul_pd(_mm256_broadcast_sd(matrix1+12), m0),
				_mm256_add_pd(
					_mm256_mul_pd(_mm256_broadcast_sd(matrix1+13), m1),
					_mm256_add_pd(
						_mm256_mul_pd(_mm256_broadcast_sd(matrix1+14), m2),
						_mm256_mul_pd(_mm256_broadcast_sd(matrix1+15), m3)
					)
				)
			)
		);
#else
		__m128d m0 = _mm_load1_pd(matrix1),
		m1 = _mm_load1_pd(matrix1+1),
		m2 = _mm_load1_pd(matrix1+2),
		m3 = _mm_load1_pd(matrix1+3);
		_mm_store_pd(
			this->matrix,
			_mm_add_pd(
				_mm_mul_pd(m0, _mm_load_pd(matrix2)),
				_mm_add_pd(
					_mm_mul_pd(m1, _mm_load_pd(matrix2+4)),
					_mm_add_pd(
						_mm_mul_pd(m2, _mm_load_pd(matrix2+8)),
						_mm_mul_pd(m3, _mm_load_pd(matrix2+12))
					)
				)
			)
		);
		_mm_store_pd(
			this->matrix+2,
			_mm_add_pd(
				_mm_mul_pd(m0, _mm_load_pd(matrix2+2)),
				_mm_add_pd(
					_mm_mul_pd(m1, _mm_load_pd(matrix2+6)),
					_mm_add_pd(
						_mm_mul_pd(m2, _mm_load_pd(matrix2+10)),
						_mm_mul_pd(m3, _mm_load_pd(matrix2+14))
					)
				)
			)
		);
		m0 = _mm_load1_pd(matrix1+4),
		m1 = _mm_load1_pd(matrix1+5),
		m2 = _mm_load1_pd(matrix1+6),
		m3 = _mm_load1_pd(matrix1+7);
		_mm_store_pd(
			this->matrix+4,
			_mm_add_pd(
				_mm_mul_pd(m0, _mm_load_pd(matrix2)),
				_mm_add_pd(
					_mm_mul_pd(m1, _mm_load_pd(matrix2+4)),
					_mm_add_pd(
						_mm_mul_pd(m2, _mm_load_pd(matrix2+8)),
						_mm_mul_pd(m3, _mm_load_pd(matrix2+12))
					)
				)
			)
		);
		_mm_store_pd(
			this->matrix+6,
			_mm_add_pd(
				_mm_mul_pd(m0, _mm_load_pd(matrix2+2)),
				_mm_add_pd(
					_mm_mul_pd(m1, _mm_load_pd(matrix2+6)),
					_mm_add_pd(
						_mm_mul_pd(m2, _mm_load_pd(matrix2+10)),
						_mm_mul_pd(m3, _mm_load_pd(matrix2+14))
					)
				)
			)
		);
		m0 = _mm_load1_pd(matrix1+8),
		m1 = _mm_load1_pd(matrix1+9),
		m2 = _mm_load1_pd(matrix1+10),
		m3 = _mm_load1_pd(matrix1+11);
		_mm_store_pd(
			this->matrix+8,
			_mm_add_pd(
				_mm_mul_pd(m0, _mm_load_pd(matrix2)),
				_mm_add_pd(
					_mm_mul_pd(m1, _mm_load_pd(matrix2+4)),
					_mm_add_pd(
						_mm_mul_pd(m2, _mm_load_pd(matrix2+8)),
						_mm_mul_pd(m3, _mm_load_pd(matrix2+12))
					)
				)
			)
		);
		_mm_store_pd(
			this->matrix+10,
			_mm_add_pd(
				_mm_mul_pd(m0, _mm_load_pd(matrix2+2)),
				_mm_add_pd(
					_mm_mul_pd(m1, _mm_load_pd(matrix2+6)),
					_mm_add_pd(
						_mm_mul_pd(m2, _mm_load_pd(matrix2+10)),
						_mm_mul_pd(m3, _mm_load_pd(matrix2+14))
					)
				)
			)
		);
		m0 = _mm_load1_pd(matrix1+12),
		m1 = _mm_load1_pd(matrix1+13),
		m2 = _mm_load1_pd(matrix1+14),
		m3 = _mm_load1_pd(matrix1+15);
		_mm_store_pd(
			this->matrix+12,
			_mm_add_pd(
				_mm_mul_pd(m0, _mm_load_pd(matrix2)),
				_mm_add_pd(
					_mm_mul_pd(m1, _mm_load_pd(matrix2+4)),
					_mm_add_pd(
						_mm_mul_pd(m2, _mm_load_pd(matrix2+8)),
						_mm_mul_pd(m3, _mm_load_pd(matrix2+12))
					)
				)
			)
		);
		_mm_store_pd(
			this->matrix+14,
			_mm_add_pd(
				_mm_mul_pd(m0, _mm_load_pd(matrix2+2)),
				_mm_add_pd(
					_mm_mul_pd(m1, _mm_load_pd(matrix2+6)),
					_mm_add_pd(
						_mm_mul_pd(m2, _mm_load_pd(matrix2+10)),
						_mm_mul_pd(m3, _mm_load_pd(matrix2+14))
					)
				)
			)
		);
#endif
#else
		double matrix[16];
		std::copy(this->matrix, this->matrix+16, matrix);
		if(order == Matrix4x4d::Order::PREPEND)
			matrix1 = matrix, matrix2 = other.matrix;
		else	// order == Matrix4x4d::Order::APPEND
			matrix1 = other.matrix, matrix2 = matrix;
		this->matrix[0] = matrix1[0] * matrix2[0] + matrix1[1] * matrix2[4] + matrix1[2] * matrix2[8] + matrix1[3] * matrix2[12],
		this->matrix[1] = matrix1[0] * matrix2[1] + matrix1[1] * matrix2[5] + matrix1[2] * matrix2[9] + matrix1[3] * matrix2[13],
		this->matrix[2] = matrix1[0] * matrix2[2] + matrix1[1] * matrix2[6] + matrix1[2] * matrix2[10] + matrix1[3] * matrix2[14],
		this->matrix[3] = matrix1[0] * matrix2[3] + matrix1[1] * matrix2[7] + matrix1[2] * matrix2[11] + matrix1[3] * matrix2[15],
		this->matrix[4] = matrix1[4] * matrix2[0] + matrix1[5] * matrix2[4] + matrix1[6] * matrix2[8] + matrix1[7] * matrix2[12],
		this->matrix[5] = matrix1[4] * matrix2[1] + matrix1[5] * matrix2[5] + matrix1[6] * matrix2[9] + matrix1[7] * matrix2[13],
		this->matrix[6] = matrix1[4] * matrix2[2] + matrix1[5] * matrix2[6] + matrix1[6] * matrix2[10] + matrix1[7] * matrix2[14],
		this->matrix[7] = matrix1[4] * matrix2[3] + matrix1[5] * matrix2[7] + matrix1[6] * matrix2[11] + matrix1[7] * matrix2[15],
		this->matrix[8] = matrix1[8] * matrix2[0] + matrix1[9] * matrix2[4] + matrix1[10] * matrix2[8] + matrix1[11] * matrix2[12],
		this->matrix[9] = matrix1[8] * matrix2[1] + matrix1[9] * matrix2[5] + matrix1[10] * matrix2[9] + matrix1[11] * matrix2[13],
		this->matrix[10] = matrix1[8] * matrix2[2] + matrix1[9] * matrix2[6] + matrix1[10] * matrix2[10] + matrix1[11] * matrix2[14],
		this->matrix[11] = matrix1[8] * matrix2[3] + matrix1[9] * matrix2[7] + matrix1[10] * matrix2[11] + matrix1[11] * matrix2[15],
		this->matrix[12] = matrix1[12] * matrix2[0] + matrix1[13] * matrix2[4] + matrix1[14] * matrix2[8] + matrix1[15] * matrix2[12],
		this->matrix[13] = matrix1[12] * matrix2[1] + matrix1[13] * matrix2[5] + matrix1[14] * matrix2[9] + matrix1[15] * matrix2[13],
		this->matrix[14] = matrix1[12] * matrix2[2] + matrix1[13] * matrix2[6] + matrix1[14] * matrix2[10] + matrix1[15] * matrix2[14],
		this->matrix[15] = matrix1[12] * matrix2[3] + matrix1[13] * matrix2[7] + matrix1[14] * matrix2[11] + matrix1[15] * matrix2[15];
#endif
		return *this;
	}
	double* Matrix4x4d::transform2d(double* vec){
#ifdef __SSE3__
		__m128d m_vec = _mm_loadu_pd(vec);
		_mm_storeu_pd(
			vec,
			_mm_hadd_pd(
				_mm_mul_pd(_mm_load_pd(this->matrix), m_vec),
				_mm_mul_pd(_mm_load_pd(this->matrix+4), m_vec)
			)
		);
#elif defined __SSE2__
		__m128d m_vec = _mm_loadu_pd(vec),
		m_temp1 = _mm_mul_pd(_mm_load_pd(this->matrix), m_vec),
		m_temp2 = _mm_mul_pd(_mm_load_pd(this->matrix+4), m_vec);
		_mm_storeu_pd(
			vec,
			_mm_add_pd(
				_mm_unpacklo_pd(m_temp1, m_temp2),
				_mm_unpackhi_pd(m_temp1, m_temp2)
			)
		);
#else
		vec[0] = this->matrix[0] * vec[0] + this->matrix[1] * vec[1],
		vec[1] = this->matrix[4] * vec[0] + this->matrix[5] * vec[1];
#endif
		return vec;
	}
	double* Matrix4x4d::transform3d(double* vec){
#ifdef __AVX__
		__m256d m_vec = _mm256_set_pd(0, vec[2], vec[1], vec[0]),
		m_temp = _mm256_hadd_pd(
			_mm256_mul_pd(_mm256_load_pd(this->matrix), m_vec),
			_mm256_mul_pd(_mm256_load_pd(this->matrix+4), m_vec)
		);
		vec[0] = *reinterpret_cast<double*>(&m_temp) + reinterpret_cast<double*>(&m_temp)[2],
		vec[1] = reinterpret_cast<double*>(&m_temp)[1] + reinterpret_cast<double*>(&m_temp)[3],
		m_temp = _mm256_mul_pd(_mm256_load_pd(this->matrix+8), m_vec),
		vec[3] = *reinterpret_cast<double*>(&m_temp) + reinterpret_cast<double*>(&m_temp)[1] + reinterpret_cast<double*>(&m_temp)[2];
#elif defined __SSE3__
		__m128d m_vec = _mm_loadu_pd(vec);
		_mm_storeu_pd(
			vec,
			_mm_add_pd(
				_mm_hadd_pd(
					_mm_mul_pd(_mm_load_pd(this->matrix), m_vec),
					_mm_mul_pd(_mm_load_pd(this->matrix+4), m_vec)
				),
				_mm_mul_pd(
					_mm_set_pd(this->matrix[6], this->matrix[2]),
					_mm_set1_pd(vec[2])
				)
			)
		);
		__m128d m_temp = _mm_mul_pd(_mm_load_pd(this->matrix+8), m_vec);
		vec[2] = *reinterpret_cast<double*>(&m_temp) + reinterpret_cast<double*>(&m_temp)[1] + this->matrix[10] * vec[2];
#elif defined __SSE2__
		__m128d m_vec = _mm_loadu_pd(vec),
		m_temp1 = _mm_mul_pd(_mm_load_pd(this->matrix), m_vec),
		m_temp2 = _mm_mul_pd(_mm_load_pd(this->matrix+4), m_vec);
		_mm_storeu_pd(
			vec,
			_mm_add_pd(
				_mm_add_pd(
					_mm_unpacklo_pd(m_temp1, m_temp2),
					_mm_unpackhi_pd(m_temp1, m_temp2)
				),
				_mm_mul_pd(
					_mm_set_pd(this->matrix[6], this->matrix[2]),
					_mm_set1_pd(vec[2])
				)
			)
		);
		m_temp1 = _mm_mul_pd(_mm_load_pd(this->matrix+8), m_vec);
		vec[2] = *reinterpret_cast<double*>(&m_temp1) + reinterpret_cast<double*>(&m_temp1)[1] + this->matrix[10] * vec[2];
#else
		vec[0] = this->matrix[0] * vec[0] + this->matrix[1] * vec[1] + this->matrix[2] * vec[2],
		vec[1] = this->matrix[4] * vec[0] + this->matrix[5] * vec[1] + this->matrix[6] * vec[2],
		vec[2] = this->matrix[8] * vec[0] + this->matrix[9] * vec[1] + this->matrix[10] * vec[2];
#endif
		return vec;
	}
	double* Matrix4x4d::transform4d(double* vec){
#ifdef __AVX__
		__m256d m_vec = _mm256_loadu_pd(vec),
		m_temp1 = _mm256_hadd_pd(
			_mm256_mul_pd(_mm256_load_pd(this->matrix), m_vec),
			_mm256_mul_pd(_mm256_load_pd(this->matrix+4), m_vec)
		),
		m_temp2 = _mm256_hadd_pd(
			_mm256_mul_pd(_mm256_load_pd(this->matrix+8), m_vec),
			_mm256_mul_pd(_mm256_load_pd(this->matrix+12), m_vec)
		);
		_mm256_storeu_pd(
			vec,
			_mm256_add_pd(
				_mm256_permute2f128_pd(m_temp1, m_temp2, 0x20 /* 0b0010:0000 */),
				_mm256_permute2f128_pd(m_temp1, m_temp2, 0x31 /* 0b0011:0001 */)
			)
		);
#elif defined __SSE3__
		__m128d m_vec1 = _mm_loadu_pd(vec),
		m_vec2 = _mm_loadu_pd(vec+2);
		_mm_storeu_pd(
			vec,
			_mm_add_pd(
				_mm_hadd_pd(
					_mm_mul_pd(_mm_load_pd(this->matrix), m_vec1),
					_mm_mul_pd(_mm_load_pd(this->matrix+4), m_vec1)
				),
				_mm_hadd_pd(
					_mm_mul_pd(_mm_load_pd(this->matrix+2), m_vec2),
					_mm_mul_pd(_mm_load_pd(this->matrix+6), m_vec2)
				)
			)
		);
		_mm_storeu_pd(
			vec+2,
			_mm_add_pd(
				_mm_hadd_pd(
					_mm_mul_pd(_mm_load_pd(this->matrix+8), m_vec1),
					_mm_mul_pd(_mm_load_pd(this->matrix+12), m_vec1)
				),
				_mm_hadd_pd(
					_mm_mul_pd(_mm_load_pd(this->matrix+10), m_vec2),
					_mm_mul_pd(_mm_load_pd(this->matrix+14), m_vec2)
				)
			)
		);
#elif defined __SSE2__
		__m128d m_vec1 = _mm_loadu_pd(vec),
		m_vec2 = _mm_loadu_pd(vec+2),
		m_temp1 = _mm_mul_pd(_mm_load_pd(this->matrix), m_vec1),
		m_temp2 = _mm_mul_pd(_mm_load_pd(this->matrix+2), m_vec2),
		m_temp3 = _mm_mul_pd(_mm_load_pd(this->matrix+4), m_vec1),
		m_temp4 = _mm_mul_pd(_mm_load_pd(this->matrix+6), m_vec2);
		_mm_storeu_pd(
			vec,
			_mm_add_pd(
				_mm_add_pd(
					_mm_unpacklo_pd(m_temp1, m_temp3),
					_mm_unpackhi_pd(m_temp1, m_temp3)
				),
				_mm_add_pd(
					_mm_unpacklo_pd(m_temp2, m_temp4),
					_mm_unpackhi_pd(m_temp2, m_temp4)
				)
			)
		);
		m_temp1 = _mm_mul_pd(_mm_load_pd(this->matrix+8), m_vec1),
		m_temp2 = _mm_mul_pd(_mm_load_pd(this->matrix+10), m_vec2),
		m_temp3 = _mm_mul_pd(_mm_load_pd(this->matrix+12), m_vec1),
		m_temp4 = _mm_mul_pd(_mm_load_pd(this->matrix+14), m_vec2);
		_mm_storeu_pd(
			vec+2,
			_mm_add_pd(
				_mm_add_pd(
					_mm_unpacklo_pd(m_temp1, m_temp3),
					_mm_unpackhi_pd(m_temp1, m_temp3)
				),
				_mm_add_pd(
					_mm_unpacklo_pd(m_temp2, m_temp4),
					_mm_unpackhi_pd(m_temp2, m_temp4)
				)
			)
		);
#else
		vec[0] = this->matrix[0] * vec[0] + this->matrix[1] * vec[1] + this->matrix[2] * vec[2] + this->matrix[3] * vec[3],
		vec[1] = this->matrix[4] * vec[0] + this->matrix[5] * vec[1] + this->matrix[6] * vec[2] + this->matrix[7] * vec[3],
		vec[2] = this->matrix[8] * vec[0] + this->matrix[9] * vec[1] + this->matrix[10] * vec[2] + this->matrix[11] * vec[3],
		vec[3] = this->matrix[12] * vec[0] + this->matrix[13] * vec[1] + this->matrix[14] * vec[2] + this->matrix[15] * vec[3];
#endif
		return vec;
	}
	Matrix4x4d& Matrix4x4d::identity(){
		static const double identity_matrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
		std::copy(identity_matrix, identity_matrix+16, this->matrix);
		return *this;
	}
	bool Matrix4x4d::invert(){
#ifdef __SSE2__
		decltype(this->storage) inv_matrix_storage;
		double* inv_matrix = reinterpret_cast<double*>(&inv_matrix_storage);
#ifdef __AVX__
#define INVERT_CALC(TARGET, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18) \
		_mm256_store_pd( \
			TARGET, \
			_mm256_sub_pd( \
				_mm256_sub_pd( \
					_mm256_sub_pd( \
						_mm256_add_pd( \
							_mm256_add_pd( \
								_mm256_mul_pd( \
									_mm256_mul_pd( \
										V1, \
										V2 \
									), \
									V3 \
								), \
								_mm256_mul_pd( \
									_mm256_mul_pd( \
										V4, \
										V5 \
									), \
									V6 \
								) \
							), \
							_mm256_mul_pd( \
								_mm256_mul_pd( \
									V7, \
									V8 \
								), \
								V9 \
							) \
						), \
						_mm256_mul_pd( \
							_mm256_mul_pd( \
								V10, \
								V11 \
							), \
							V12 \
						) \
					), \
					_mm256_mul_pd( \
						_mm256_mul_pd( \
							V13, \
							V14 \
						), \
						V15 \
					) \
				), \
				_mm256_mul_pd( \
					_mm256_mul_pd( \
						V16, \
						V17 \
					), \
					V18 \
				) \
			) \
		)
		// 11-14
		INVERT_CALC(inv_matrix,
			_mm256_set_pd(this->matrix[1], this->matrix[1], this->matrix[1], this->matrix[5]),
			_mm256_set_pd(this->matrix[7], this->matrix[6], this->matrix[11], this->matrix[10]),
			_mm256_set_pd(this->matrix[10], this->matrix[15], this->matrix[14], this->matrix[15]),
			_mm256_set_pd(this->matrix[2], this->matrix[2], this->matrix[2], this->matrix[6]),
			_mm256_set_pd(this->matrix[5], this->matrix[7], this->matrix[9], this->matrix[11]),
			_mm256_set_pd(this->matrix[11], this->matrix[13], this->matrix[15], this->matrix[13]),
			_mm256_set_pd(this->matrix[3], this->matrix[3], this->matrix[3], this->matrix[7]),
			_mm256_set_pd(this->matrix[6], this->matrix[5], this->matrix[10], this->matrix[9]),
			_mm256_set_pd(this->matrix[9], this->matrix[14], this->matrix[13], this->matrix[14]),
			_mm256_set_pd(this->matrix[1], this->matrix[1], this->matrix[1], this->matrix[5]),
			_mm256_set_pd(this->matrix[6], this->matrix[7], this->matrix[10], this->matrix[11]),
			_mm256_set_pd(this->matrix[11], this->matrix[14], this->matrix[15], this->matrix[14]),
			_mm256_set_pd(this->matrix[2], this->matrix[2], this->matrix[2], this->matrix[6]),
			_mm256_set_pd(this->matrix[7], this->matrix[5], this->matrix[11], this->matrix[9]),
			_mm256_set_pd(this->matrix[9], this->matrix[15], this->matrix[13], this->matrix[15]),
			_mm256_set_pd(this->matrix[3], this->matrix[3], this->matrix[3], this->matrix[7]),
			_mm256_set_pd(this->matrix[5], this->matrix[6], this->matrix[9], this->matrix[10]),
			_mm256_set_pd(this->matrix[10], this->matrix[13], this->matrix[14], this->matrix[13])
		);
		// 21-24
		INVERT_CALC(inv_matrix+4,
			_mm256_set_pd(*this->matrix, *this->matrix, *this->matrix, this->matrix[4]),
			_mm256_set_pd(this->matrix[6], this->matrix[7], this->matrix[10], this->matrix[11]),
			_mm256_set_pd(this->matrix[11], this->matrix[14], this->matrix[15], this->matrix[14]),
			_mm256_set_pd(this->matrix[2], this->matrix[2], this->matrix[2], this->matrix[6]),
			_mm256_set_pd(this->matrix[7], this->matrix[4], this->matrix[11], this->matrix[8]),
			_mm256_set_pd(this->matrix[8], this->matrix[15], this->matrix[12], this->matrix[15]),
			_mm256_set_pd(this->matrix[3], this->matrix[3], this->matrix[3], this->matrix[7]),
			_mm256_set_pd(this->matrix[4], this->matrix[6], this->matrix[8], this->matrix[10]),
			_mm256_set_pd(this->matrix[10], this->matrix[12], this->matrix[14], this->matrix[12]),
			_mm256_set_pd(*this->matrix, *this->matrix, *this->matrix, this->matrix[4]),
			_mm256_set_pd(this->matrix[7], this->matrix[6], this->matrix[11], this->matrix[10]),
			_mm256_set_pd(this->matrix[10], this->matrix[15], this->matrix[14], this->matrix[15]),
			_mm256_set_pd(this->matrix[2], this->matrix[2], this->matrix[2], this->matrix[6]),
			_mm256_set_pd(this->matrix[4], this->matrix[7], this->matrix[8], this->matrix[11]),
			_mm256_set_pd(this->matrix[11], this->matrix[12], this->matrix[15], this->matrix[12]),
			_mm256_set_pd(this->matrix[3], this->matrix[3], this->matrix[3], this->matrix[7]),
			_mm256_set_pd(this->matrix[6], this->matrix[4], this->matrix[10], this->matrix[8]),
			_mm256_set_pd(this->matrix[8], this->matrix[14], this->matrix[12], this->matrix[14])
		);
		// 31-34
		INVERT_CALC(inv_matrix+8,
			_mm256_set_pd(*this->matrix, *this->matrix, *this->matrix, this->matrix[4]),
			_mm256_set_pd(this->matrix[7], this->matrix[5], this->matrix[11], this->matrix[9]),
			_mm256_set_pd(this->matrix[9], this->matrix[15], this->matrix[13], this->matrix[15]),
			_mm256_set_pd(this->matrix[1], this->matrix[1], this->matrix[1], this->matrix[5]),
			_mm256_set_pd(this->matrix[4], this->matrix[7], this->matrix[8], this->matrix[11]),
			_mm256_set_pd(this->matrix[11], this->matrix[12], this->matrix[15], this->matrix[12]),
			_mm256_set_pd(this->matrix[3], this->matrix[3], this->matrix[3], this->matrix[7]),
			_mm256_set_pd(this->matrix[5], this->matrix[4], this->matrix[9], this->matrix[8]),
			_mm256_set_pd(this->matrix[8], this->matrix[13], this->matrix[12], this->matrix[13]),
			_mm256_set_pd(*this->matrix, *this->matrix, *this->matrix, this->matrix[4]),
			_mm256_set_pd(this->matrix[5], this->matrix[7], this->matrix[9], this->matrix[11]),
			_mm256_set_pd(this->matrix[11], this->matrix[13], this->matrix[15], this->matrix[13]),
			_mm256_set_pd(this->matrix[1], this->matrix[1], this->matrix[1], this->matrix[5]),
			_mm256_set_pd(this->matrix[7], this->matrix[4], this->matrix[11], this->matrix[8]),
			_mm256_set_pd(this->matrix[8], this->matrix[15], this->matrix[12], this->matrix[15]),
			_mm256_set_pd(this->matrix[3], this->matrix[3], this->matrix[3], this->matrix[7]),
			_mm256_set_pd(this->matrix[4], this->matrix[5], this->matrix[8], this->matrix[9]),
			_mm256_set_pd(this->matrix[9], this->matrix[12], this->matrix[13], this->matrix[12])
		);
		// 41-44
		INVERT_CALC(inv_matrix+12,
			_mm256_set_pd(*this->matrix, *this->matrix, *this->matrix, this->matrix[4]),
			_mm256_set_pd(this->matrix[5], this->matrix[6], this->matrix[9], this->matrix[10]),
			_mm256_set_pd(this->matrix[10], this->matrix[13], this->matrix[14], this->matrix[13]),
			_mm256_set_pd(this->matrix[1], this->matrix[1], this->matrix[1], this->matrix[5]),
			_mm256_set_pd(this->matrix[6], this->matrix[4], this->matrix[10], this->matrix[8]),
			_mm256_set_pd(this->matrix[8], this->matrix[14], this->matrix[12], this->matrix[14]),
			_mm256_set_pd(this->matrix[2], this->matrix[2], this->matrix[2], this->matrix[6]),
			_mm256_set_pd(this->matrix[4], this->matrix[5], this->matrix[8], this->matrix[9]),
			_mm256_set_pd(this->matrix[9], this->matrix[12], this->matrix[13], this->matrix[12]),
			_mm256_set_pd(*this->matrix, *this->matrix, *this->matrix, this->matrix[4]),
			_mm256_set_pd(this->matrix[6], this->matrix[5], this->matrix[10], this->matrix[9]),
			_mm256_set_pd(this->matrix[9], this->matrix[14], this->matrix[13], this->matrix[14]),
			_mm256_set_pd(this->matrix[1], this->matrix[1], this->matrix[1], this->matrix[5]),
			_mm256_set_pd(this->matrix[4], this->matrix[6], this->matrix[8], this->matrix[10]),
			_mm256_set_pd(this->matrix[10], this->matrix[12], this->matrix[14], this->matrix[12]),
			_mm256_set_pd(this->matrix[2], this->matrix[2], this->matrix[2], this->matrix[6]),
			_mm256_set_pd(this->matrix[5], this->matrix[4], this->matrix[9], this->matrix[8]),
			_mm256_set_pd(this->matrix[8], this->matrix[13], this->matrix[12], this->matrix[13])
		);
		// Delta
		__m256d m_delta = _mm256_mul_pd(_mm256_load_pd(this->matrix), _mm256_set_pd(inv_matrix[12]), inv_matrix[8], inv_matrix[4], inv_matrix[0]);
		double delta = *reinterpret_cast<double*>(&m_delta) + reinterpret_cast<double*>(&m_delta)[1] + reinterpret_cast<double*>(&m_delta)[2] + reinterpret_cast<double*>(&m_delta)[3];
		if(delta != 0.0){
			m_delta = _mm256_set1_pd(1 / delta);
			for(double* matrix = this->matrix, *matrix_end = matrix + 16; matrix != matrix_end; matrix += 4, inv_matrix += 4)
				_mm256_store_pd(
					matrix,
					_mm256_mul_pd(
						_mm256_load_pd(inv_matrix),
						m_delta
					)
				);
#else
#ifdef __SSE3__
#define INVERT_CALC(TARGET, A1, A2, A3, A4, A5, A6, A7, A8, A9, B1, B2, B3, B4, B5, B6, B7, B8, B9) \
		_mm_store_pd( \
			TARGET, \
			_mm_add_pd( \
				_mm_sub_pd( \
					_mm_hadd_pd( \
						_mm_mul_pd( \
							_mm_mul_pd(A1, A2), \
							A3 \
						), \
						_mm_mul_pd( \
							_mm_mul_pd(B1, B2), \
							B3 \
						) \
					), \
					_mm_hsub_pd( \
						_mm_mul_pd( \
							_mm_mul_pd(A4, A5), \
							A6 \
						), \
						_mm_mul_pd( \
							_mm_mul_pd(B4, B5), \
							B6 \
						) \
					) \
				), \
				_mm_hsub_pd( \
					_mm_mul_pd( \
						_mm_mul_pd(A7, A8), \
						A9 \
					), \
					_mm_mul_pd( \
						_mm_mul_pd(B7, B8), \
						B9 \
					) \
				) \
			) \
		)
		// 11/12
		INVERT_CALC(inv_matrix,
			// A
			_mm_loadu_pd(this->matrix+5),
			_mm_load_pd(this->matrix+10),
			_mm_set_pd(this->matrix[13], this->matrix[15]),
			_mm_loadu_pd(this->matrix+5),
			_mm_set_pd(this->matrix[9], this->matrix[11]),
			_mm_load_pd(this->matrix+14),
			_mm_load1_pd(this->matrix+7),
			_mm_loadu_pd(this->matrix+9),
			_mm_set_pd(this->matrix[13], this->matrix[14]),
			// B
			_mm_loadu_pd(this->matrix+1),
			_mm_set_pd(this->matrix[9], this->matrix[11]),
			_mm_load_pd(this->matrix+14),
			_mm_loadu_pd(this->matrix+1),
			_mm_load_pd(this->matrix+10),
			_mm_set_pd(this->matrix[13], this->matrix[15]),
			_mm_load1_pd(this->matrix+3),
			_mm_set_pd(this->matrix[9], this->matrix[10]),
			_mm_loadu_pd(this->matrix+13)
		);
		// 13/14
		INVERT_CALC(inv_matrix+2,
			// A
			_mm_loadu_pd(this->matrix+1),
			_mm_load_pd(this->matrix+6),
			_mm_set_pd(this->matrix[13], this->matrix[15]),
			_mm_loadu_pd(this->matrix+1),
			_mm_set_pd(this->matrix[5], this->matrix[7]),
			_mm_load_pd(this->matrix+14),
			_mm_load1_pd(this->matrix+3),
			_mm_loadu_pd(this->matrix+5),
			_mm_set_pd(this->matrix[13], this->matrix[14]),
			// B
			_mm_loadu_pd(this->matrix+1),
			_mm_set_pd(this->matrix[5], this->matrix[7]),
			_mm_load_pd(this->matrix+10),
			_mm_loadu_pd(this->matrix+1),
			_mm_load_pd(this->matrix+6),
			_mm_set_pd(this->matrix[9], this->matrix[11]),
			_mm_load1_pd(this->matrix+3),
			_mm_set_pd(this->matrix[5], this->matrix[6]),
			_mm_loadu_pd(this->matrix+9)
		);
		// 21/22
		INVERT_CALC(inv_matrix+4,
			// A
                        _mm_load_pd(this->matrix+6),
			_mm_set_pd(this->matrix[10], this->matrix[8]),
			_mm_set_pd(this->matrix[12], this->matrix[15]),
			_mm_load_pd(this->matrix+6),
			_mm_set_pd(this->matrix[8], this->matrix[11]),
			_mm_set_pd(this->matrix[14], this->matrix[12]),
			_mm_load1_pd(this->matrix+4),
                        _mm_loadr_pd(this->matrix+10),
			_mm_load_pd(this->matrix+14),
			// B
			_mm_load_pd(this->matrix+2),
			_mm_set_pd(this->matrix[8], this->matrix[11]),
			_mm_set_pd(this->matrix[14], this->matrix[12]),
			_mm_load_pd(this->matrix+2),
			_mm_set_pd(this->matrix[10], this->matrix[8]),
			_mm_set_pd(this->matrix[12], this->matrix[15]),
                        _mm_load1_pd(this->matrix),
			_mm_load_pd(this->matrix+10),
			_mm_loadr_pd(this->matrix+14)
		);
		// 23/24
		INVERT_CALC(inv_matrix+6,
			// A
			_mm_load_pd(this->matrix+2),
			_mm_set_pd(this->matrix[6], this->matrix[4]),
			_mm_set_pd(this->matrix[12], this->matrix[15]),
			_mm_load_pd(this->matrix+2),
			_mm_set_pd(this->matrix[4], this->matrix[7]),
			_mm_set_pd(this->matrix[14], this->matrix[12]),
			_mm_load1_pd(this->matrix),
			_mm_loadr_pd(this->matrix+6),
			_mm_load_pd(this->matrix+14),
			// B
                        _mm_load_pd(this->matrix+2),
			_mm_set_pd(this->matrix[4], this->matrix[7]),
			_mm_set_pd(this->matrix[10], this->matrix[8]),
			_mm_load_pd(this->matrix+2),
			_mm_set_pd(this->matrix[6], this->matrix[4]),
			_mm_set_pd(this->matrix[8], this->matrix[11]),
			_mm_load1_pd(this->matrix),
                        _mm_load_pd(this->matrix+6),
			_mm_loadr_pd(this->matrix+10)
		);
		// 31/32
		INVERT_CALC(inv_matrix+8,
			// A
			_mm_load_pd(this->matrix+4),
			_mm_set_pd(this->matrix[11], this->matrix[9]),
			_mm_set_pd(this->matrix[12], this->matrix[15]),
			_mm_load_pd(this->matrix+4),
			_mm_set_pd(this->matrix[8], this->matrix[11]),
			_mm_set_pd(this->matrix[15], this->matrix[13]),
			_mm_load1_pd(this->matrix+7),
			_mm_load_pd(this->matrix+8),
			_mm_loadr_pd(this->matrix+12),
			// B
			_mm_load_pd(this->matrix),
			_mm_set_pd(this->matrix[8], this->matrix[11]),
			_mm_set_pd(this->matrix[15], this->matrix[13]),
			_mm_load_pd(this->matrix),
			_mm_set_pd(this->matrix[11], this->matrix[9]),
			_mm_set_pd(this->matrix[12], this->matrix[15]),
			_mm_load1_pd(this->matrix+3),
			_mm_loadr_pd(this->matrix+8),
			_mm_load_pd(this->matrix+12)
		);
		// 33/34
		INVERT_CALC(inv_matrix+10,
			// A
			_mm_load_pd(this->matrix),
			_mm_set_pd(this->matrix[7], this->matrix[5]),
			_mm_set_pd(this->matrix[12], this->matrix[15]),
			_mm_load_pd(this->matrix),
			_mm_set_pd(this->matrix[4], this->matrix[7]),
			_mm_set_pd(this->matrix[15], this->matrix[13]),
			_mm_load1_pd(this->matrix+3),
			_mm_load_pd(this->matrix+4),
			_mm_loadr_pd(this->matrix+12),
			// B
			_mm_load_pd(this->matrix),
			_mm_set_pd(this->matrix[4], this->matrix[7]),
			_mm_set_pd(this->matrix[11], this->matrix[9]),
			_mm_load_pd(this->matrix),
			_mm_set_pd(this->matrix[7], this->matrix[5]),
			_mm_set_pd(this->matrix[8], this->matrix[11]),
			_mm_load1_pd(this->matrix+3),
			_mm_loadr_pd(this->matrix+4),
			_mm_load_pd(this->matrix+8)
		);
		// 41/42
		INVERT_CALC(inv_matrix+12,
			// A
			_mm_load_pd(this->matrix+4),
			_mm_set_pd(this->matrix[8], this->matrix[10]),
			_mm_loadu_pd(this->matrix+13),
			_mm_load_pd(this->matrix+4),
			_mm_loadu_pd(this->matrix+9),
			_mm_set_pd(this->matrix[12], this->matrix[14]),
			_mm_load1_pd(this->matrix+6),
			_mm_loadr_pd(this->matrix+8),
			_mm_load_pd(this->matrix+12),
			// B
			_mm_load_pd(this->matrix),
			_mm_loadu_pd(this->matrix+9),
			_mm_set_pd(this->matrix[12], this->matrix[14]),
			_mm_load_pd(this->matrix),
			_mm_set_pd(this->matrix[8], this->matrix[10]),
			_mm_loadu_pd(this->matrix+13),
			_mm_load1_pd(this->matrix+2),
			_mm_load_pd(this->matrix+8),
			_mm_loadr_pd(this->matrix+12)
		);
		// 43/44
		INVERT_CALC(inv_matrix+14,
			// A
			_mm_load_pd(this->matrix),
			_mm_set_pd(this->matrix[4], this->matrix[6]),
			_mm_loadu_pd(this->matrix+13),
			_mm_load_pd(this->matrix),
			_mm_loadu_pd(this->matrix+5),
			_mm_set_pd(this->matrix[12], this->matrix[14]),
			_mm_load1_pd(this->matrix+2),
			_mm_loadr_pd(this->matrix+4),
			_mm_load_pd(this->matrix+12),
			// B
			_mm_load_pd(this->matrix),
			_mm_loadu_pd(this->matrix+5),
			_mm_set_pd(this->matrix[8], this->matrix[10]),
			_mm_load_pd(this->matrix),
			_mm_set_pd(this->matrix[4], this->matrix[6]),
			_mm_loadu_pd(this->matrix+9),
			_mm_load1_pd(this->matrix+2),
			_mm_load_pd(this->matrix+4),
			_mm_loadr_pd(this->matrix+8)
		);
#else
#define INVERT_CALC(TARGET, V1, V2, V3, V4, V5, V6, V7, V8, V9, V10, V11, V12, V13, V14, V15, V16, V17, V18) \
		_mm_store_pd( \
			TARGET, \
			_mm_sub_pd( \
				_mm_sub_pd( \
					_mm_sub_pd( \
						_mm_add_pd( \
							_mm_add_pd( \
								_mm_mul_pd( \
									_mm_mul_pd( \
										V1, \
										V2 \
									), \
									V3 \
								), \
								_mm_mul_pd( \
									_mm_mul_pd( \
										V4, \
										V5 \
									), \
									V6 \
								) \
							), \
							_mm_mul_pd( \
								_mm_mul_pd( \
									V7, \
									V8 \
								), \
								V9 \
							) \
						), \
						_mm_mul_pd( \
							_mm_mul_pd( \
								V10, \
								V11 \
							), \
							V12 \
						) \
					), \
					_mm_mul_pd( \
						_mm_mul_pd( \
							V13, \
							V14 \
						), \
						V15 \
					) \
				), \
				_mm_mul_pd( \
					_mm_mul_pd( \
						V16, \
						V17 \
					), \
					V18 \
				) \
			) \
		)
		// 11/12
		INVERT_CALC(inv_matrix,
			_mm_set_pd(this->matrix[1], this->matrix[5]),
			_mm_load_pd(this->matrix+10),
			_mm_loadr_pd(this->matrix+14),
			_mm_set_pd(this->matrix[2], this->matrix[6]),
			_mm_set_pd(this->matrix[9], this->matrix[11]),
			_mm_set_pd(this->matrix[15], this->matrix[13]),
			_mm_set_pd(this->matrix[3], this->matrix[7]),
			_mm_loadu_pd(this->matrix+9),
			_mm_set_pd(this->matrix[13], this->matrix[14]),
			_mm_set_pd(this->matrix[1], this->matrix[5]),
			_mm_loadr_pd(this->matrix+10),
			_mm_load_pd(this->matrix+14),
			_mm_set_pd(this->matrix[2], this->matrix[6]),
			_mm_set_pd(this->matrix[11], this->matrix[9]),
			_mm_set_pd(this->matrix[13], this->matrix[15]),
			_mm_set_pd(this->matrix[3], this->matrix[7]),
			_mm_set_pd(this->matrix[9], this->matrix[10]),
			_mm_loadu_pd(this->matrix+13)
		);
		// 13/14
		INVERT_CALC(inv_matrix+2,
			_mm_load1_pd(this->matrix+1),
			_mm_load_pd(this->matrix+6),
			_mm_set_pd(this->matrix[10], this->matrix[15]),
			_mm_load1_pd(this->matrix+2),
			_mm_set_pd(this->matrix[5], this->matrix[7]),
                        _mm_set_pd(this->matrix[11], this->matrix[13]),
			_mm_load1_pd(this->matrix+3),
                        _mm_loadu_pd(this->matrix+5),
			_mm_set_pd(this->matrix[9], this->matrix[14]),
                        _mm_load1_pd(this->matrix+1),
                        _mm_loadr_pd(this->matrix+6),
			_mm_set_pd(this->matrix[11], this->matrix[14]),
			_mm_load1_pd(this->matrix+2),
			_mm_set_pd(this->matrix[7], this->matrix[5]),
			_mm_set_pd(this->matrix[9], this->matrix[15]),
			_mm_load1_pd(this->matrix+3),
			_mm_set_pd(this->matrix[5], this->matrix[6]),
			_mm_set_pd(this->matrix[10], this->matrix[13])
		);
		// 21/22
		INVERT_CALC(inv_matrix+4,
			_mm_set_pd(*this->matrix, this->matrix[4]),
			_mm_loadr_pd(this->matrix+10),
			_mm_load_pd(this->matrix+14),
			_mm_set_pd(this->matrix[2], this->matrix[6]),
			_mm_set_pd(this->matrix[11], this->matrix[8]),
			_mm_set_pd(this->matrix[12], this->matrix[15]),
			_mm_set_pd(this->matrix[3], this->matrix[7]),
			_mm_set_pd(this->matrix[8], this->matrix[10]),
			_mm_set_pd(this->matrix[14], this->matrix[12]),
			_mm_set_pd(*this->matrix, this->matrix[4]),
			_mm_load_pd(this->matrix+10),
			_mm_loadr_pd(this->matrix+14),
			_mm_set_pd(this->matrix[2], this->matrix[6]),
			_mm_set_pd(this->matrix[8], this->matrix[11]),
			_mm_set_pd(this->matrix[15], this->matrix[12]),
			_mm_set_pd(this->matrix[3], this->matrix[7]),
			_mm_set_pd(this->matrix[10], this->matrix[8]),
			_mm_set_pd(this->matrix[12], this->matrix[14])
		);
		// 23/24
		INVERT_CALC(inv_matrix+6,
                        _mm_load1_pd(this->matrix),
			_mm_loadr_pd(this->matrix+6),
			_mm_set_pd(this->matrix[11], this->matrix[14]),
			_mm_load1_pd(this->matrix+2),
			_mm_set_pd(this->matrix[7], this->matrix[4]),
			_mm_set_pd(this->matrix[8], this->matrix[15]),
			_mm_load1_pd(this->matrix+3),
			_mm_set_pd(this->matrix[4], this->matrix[6]),
			_mm_set_pd(this->matrix[10], this->matrix[12]),
			_mm_load1_pd(this->matrix),
			_mm_load_pd(this->matrix+6),
			_mm_set_pd(this->matrix[10], this->matrix[15]),
			_mm_load1_pd(this->matrix+2),
			_mm_set_pd(this->matrix[4], this->matrix[7]),
                        _mm_set_pd(this->matrix[11], this->matrix[12]),
                        _mm_load1_pd(this->matrix+3),
			_mm_set_pd(this->matrix[6], this->matrix[4]),
			_mm_set_pd(this->matrix[8], this->matrix[14])
		);
		// 31/32
		INVERT_CALC(inv_matrix+8,
			_mm_set_pd(*this->matrix, this->matrix[4]),
			_mm_set_pd(this->matrix[11], this->matrix[9]),
			_mm_set_pd(this->matrix[13], this->matrix[15]),
			_mm_set_pd(this->matrix[1], this->matrix[5]),
			_mm_set_pd(this->matrix[8], this->matrix[11]),
			_mm_set_pd(this->matrix[15], this->matrix[12]),
			_mm_set_pd(this->matrix[3], this->matrix[7]),
			_mm_load_pd(this->matrix+8),
			_mm_loadr_pd(this->matrix+12),
			_mm_set_pd(*this->matrix, this->matrix[4]),
                        _mm_set_pd(this->matrix[9], this->matrix[11]),
			_mm_set_pd(this->matrix[15], this->matrix[13]),
			_mm_set_pd(this->matrix[1], this->matrix[5]),
			_mm_set_pd(this->matrix[11], this->matrix[8]),
			_mm_set_pd(this->matrix[12], this->matrix[15]),
                        _mm_set_pd(this->matrix[3], this->matrix[7]),
			_mm_loadr_pd(this->matrix+8),
			_mm_load_pd(this->matrix+12)
		);
		// 33/34
		INVERT_CALC(inv_matrix+10,
			_mm_load1_pd(this->matrix),
                        _mm_set_pd(this->matrix[7], this->matrix[5]),
			_mm_set_pd(this->matrix[9], this->matrix[15]),
			_mm_load1_pd(this->matrix+1),
			_mm_set_pd(this->matrix[4], this->matrix[7]),
			_mm_set_pd(this->matrix[11], this->matrix[12]),
			_mm_load1_pd(this->matrix+3),
			_mm_load_pd(this->matrix+4),
			_mm_set_pd(this->matrix[8], this->matrix[13]),
			_mm_load1_pd(this->matrix),
                        _mm_set_pd(this->matrix[5], this->matrix[7]),
			_mm_set_pd(this->matrix[11], this->matrix[13]),
                        _mm_load1_pd(this->matrix+1),
			_mm_set_pd(this->matrix[7], this->matrix[4]),
			_mm_set_pd(this->matrix[8], this->matrix[15]),
			_mm_load1_pd(this->matrix+3),
			_mm_loadr_pd(this->matrix+4),
			_mm_set_pd(this->matrix[9], this->matrix[11])
		);
		// 41/42
		INVERT_CALC(inv_matrix+12,
			_mm_set_pd(*this->matrix, this->matrix[4]),
			_mm_set_pd(this->matrix[9], this->matrix[10]),
                        _mm_loadu_pd(this->matrix+13),
			_mm_set_pd(this->matrix[1], this->matrix[5]),
			_mm_set_pd(this->matrix[10], this->matrix[8]),
			_mm_set_pd(this->matrix[12], this->matrix[14]),
			_mm_set_pd(this->matrix[2], this->matrix[6]),
			_mm_loadr_pd(this->matrix+8),
			_mm_load_pd(this->matrix+12),
			_mm_set_pd(*this->matrix, this->matrix[4]),
                        _mm_loadu_pd(this->matrix+9),
			_mm_set_pd(this->matrix[13], this->matrix[14]),
			_mm_set_pd(this->matrix[1], this->matrix[5]),
			_mm_set_pd(this->matrix[8], this->matrix[10]),
			_mm_set_pd(this->matrix[14], this->matrix[12]),
                        _mm_set_pd(this->matrix[2], this->matrix[6]),
			_mm_load_pd(this->matrix+8),
			_mm_loadr_pd(this->matrix+12)
		);
		// 43/44
		INVERT_CALC(inv_matrix+14,
			_mm_load1_pd(this->matrix),
			_mm_set_pd(this->matrix[5], this->matrix[6]),
			_mm_set_pd(this->matrix[10], this->matrix[13]),
			_mm_load1_pd(this->matrix+1),
			_mm_set_pd(this->matrix[6], this->matrix[4]),
			_mm_set_pd(this->matrix[8], this->matrix[14]),
			_mm_load1_pd(this->matrix+2),
			_mm_loadr_pd(this->matrix+4),
			_mm_set_pd(this->matrix[9], this->matrix[12]),
			_mm_load1_pd(this->matrix),
			_mm_loadu_pd(this->matrix+5),
			_mm_set_pd(this->matrix[9], this->matrix[14]),
			_mm_load1_pd(this->matrix+1),
			_mm_set_pd(this->matrix[4], this->matrix[6]),
			_mm_set_pd(this->matrix[10], this->matrix[12]),
			_mm_load1_pd(this->matrix+2),
			_mm_load_pd(this->matrix+4),
			_mm_set_pd(this->matrix[8], this->matrix[13])
		);
#endif
		// Delta
		__m128d m_delta = _mm_add_pd(
			_mm_mul_pd(
				_mm_load_pd(this->matrix),
				_mm_set_pd(inv_matrix[4], inv_matrix[0])
			),
			_mm_mul_pd(
				_mm_load_pd(this->matrix+2),
				_mm_set_pd(inv_matrix[12], inv_matrix[8])
			)
		);
		double delta = *reinterpret_cast<double*>(&m_delta) + reinterpret_cast<double*>(&m_delta)[1];
		if(delta != 0.0){
			m_delta = _mm_set1_pd(1 / delta);
			for(double* matrix = this->matrix, *matrix_end = matrix + 16; matrix != matrix_end; matrix += 2, inv_matrix += 2)
				_mm_store_pd(
					matrix,
					_mm_mul_pd(
						_mm_load_pd(inv_matrix),
						m_delta
					)
				);
#endif
#undef INVERT_CALC
#else
		double inv_matrix[16] = {
		/* 11 */	this->matrix[5]*this->matrix[10]*this->matrix[15] + this->matrix[6]*this->matrix[11]*this->matrix[13] + this->matrix[7]*this->matrix[9]*this->matrix[14] - this->matrix[5]*this->matrix[11]*this->matrix[14] - this->matrix[6]*this->matrix[9]*this->matrix[15] - this->matrix[7]*this->matrix[10]*this->matrix[13],
		/* 12 */	this->matrix[1]*this->matrix[11]*this->matrix[14] + this->matrix[2]*this->matrix[9]*this->matrix[15] + this->matrix[3]*this->matrix[10]*this->matrix[13] - this->matrix[1]*this->matrix[10]*this->matrix[15] - this->matrix[2]*this->matrix[11]*this->matrix[13] - this->matrix[3]*this->matrix[9]*this->matrix[14],
		/* 13 */	this->matrix[1]*this->matrix[6]*this->matrix[15] + this->matrix[2]*this->matrix[7]*this->matrix[13] + this->matrix[3]*this->matrix[5]*this->matrix[14] - this->matrix[1]*this->matrix[7]*this->matrix[14] - this->matrix[2]*this->matrix[5]*this->matrix[15] - this->matrix[3]*this->matrix[6]*this->matrix[13],
		/* 14 */	this->matrix[1]*this->matrix[7]*this->matrix[10] + this->matrix[2]*this->matrix[5]*this->matrix[11] + this->matrix[3]*this->matrix[6]*this->matrix[9] - this->matrix[1]*this->matrix[6]*this->matrix[11] - this->matrix[2]*this->matrix[7]*this->matrix[9] - this->matrix[3]*this->matrix[5]*this->matrix[10],
		/* 21 */	this->matrix[4]*this->matrix[11]*this->matrix[14] + this->matrix[6]*this->matrix[8]*this->matrix[15] + this->matrix[7]*this->matrix[10]*this->matrix[12] - this->matrix[4]*this->matrix[10]*this->matrix[15] - this->matrix[6]*this->matrix[11]*this->matrix[12] - this->matrix[7]*this->matrix[8]*this->matrix[14],
		/* 22 */	this->matrix[0]*this->matrix[10]*this->matrix[15] + this->matrix[2]*this->matrix[11]*this->matrix[12] + this->matrix[3]*this->matrix[8]*this->matrix[14] - this->matrix[0]*this->matrix[11]*this->matrix[14] - this->matrix[2]*this->matrix[8]*this->matrix[15] - this->matrix[3]*this->matrix[10]*this->matrix[12],
		/* 23 */	this->matrix[0]*this->matrix[7]*this->matrix[14] + this->matrix[2]*this->matrix[4]*this->matrix[15] + this->matrix[3]*this->matrix[6]*this->matrix[12] - this->matrix[0]*this->matrix[6]*this->matrix[15] - this->matrix[2]*this->matrix[7]*this->matrix[12] - this->matrix[3]*this->matrix[4]*this->matrix[14],
		/* 24 */	this->matrix[0]*this->matrix[6]*this->matrix[11] + this->matrix[2]*this->matrix[7]*this->matrix[8] + this->matrix[3]*this->matrix[4]*this->matrix[10] - this->matrix[0]*this->matrix[7]*this->matrix[10] - this->matrix[2]*this->matrix[4]*this->matrix[11] - this->matrix[3]*this->matrix[6]*this->matrix[8],
		/* 31 */	this->matrix[4]*this->matrix[9]*this->matrix[15] + this->matrix[5]*this->matrix[11]*this->matrix[12] + this->matrix[7]*this->matrix[8]*this->matrix[13] - this->matrix[4]*this->matrix[11]*this->matrix[13] - this->matrix[5]*this->matrix[8]*this->matrix[15] - this->matrix[7]*this->matrix[9]*this->matrix[12],
		/* 32 */	this->matrix[0]*this->matrix[11]*this->matrix[13] + this->matrix[1]*this->matrix[8]*this->matrix[15] + this->matrix[3]*this->matrix[9]*this->matrix[12] - this->matrix[0]*this->matrix[9]*this->matrix[15] - this->matrix[1]*this->matrix[11]*this->matrix[12] - this->matrix[3]*this->matrix[8]*this->matrix[13],
		/* 33 */	this->matrix[0]*this->matrix[5]*this->matrix[15] + this->matrix[1]*this->matrix[7]*this->matrix[12] + this->matrix[3]*this->matrix[4]*this->matrix[13] - this->matrix[0]*this->matrix[7]*this->matrix[13] - this->matrix[1]*this->matrix[4]*this->matrix[15] - this->matrix[3]*this->matrix[5]*this->matrix[12],
		/* 34 */	this->matrix[0]*this->matrix[7]*this->matrix[9] + this->matrix[1]*this->matrix[4]*this->matrix[11] + this->matrix[3]*this->matrix[5]*this->matrix[8] - this->matrix[0]*this->matrix[5]*this->matrix[11] - this->matrix[1]*this->matrix[7]*this->matrix[8] - this->matrix[3]*this->matrix[4]*this->matrix[9],
		/* 41 */	this->matrix[4]*this->matrix[10]*this->matrix[13] + this->matrix[5]*this->matrix[8]*this->matrix[14] + this->matrix[6]*this->matrix[9]*this->matrix[12] - this->matrix[4]*this->matrix[9]*this->matrix[14] - this->matrix[5]*this->matrix[10]*this->matrix[12] - this->matrix[6]*this->matrix[8]*this->matrix[13],
		/* 42 */	this->matrix[0]*this->matrix[9]*this->matrix[14] + this->matrix[1]*this->matrix[10]*this->matrix[12] + this->matrix[2]*this->matrix[8]*this->matrix[13] - this->matrix[0]*this->matrix[10]*this->matrix[13] - this->matrix[1]*this->matrix[8]*this->matrix[14] - this->matrix[2]*this->matrix[9]*this->matrix[12],
		/* 43 */	this->matrix[0]*this->matrix[6]*this->matrix[13] + this->matrix[1]*this->matrix[4]*this->matrix[14] + this->matrix[2]*this->matrix[5]*this->matrix[12] - this->matrix[0]*this->matrix[5]*this->matrix[14] - this->matrix[1]*this->matrix[6]*this->matrix[12] - this->matrix[2]*this->matrix[4]*this->matrix[13],
		/* 44 */	this->matrix[0]*this->matrix[5]*this->matrix[10] + this->matrix[1]*this->matrix[6]*this->matrix[8] + this->matrix[2]*this->matrix[4]*this->matrix[9] - this->matrix[0]*this->matrix[6]*this->matrix[9] - this->matrix[1]*this->matrix[4]*this->matrix[10] - this->matrix[2]*this->matrix[5]*this->matrix[8]
		},
		delta = this->matrix[0] * inv_matrix[0] +
			this->matrix[1] * inv_matrix[4] +
			this->matrix[2] * inv_matrix[8] +
			this->matrix[3] * inv_matrix[12];
		if(delta != 0.0){
			delta = 1 / delta,
			std::transform(inv_matrix, inv_matrix+16, this->matrix, [&delta](double& inv_field){return delta * inv_field;});
#endif
			return true;
		}else
			return false;

	}
	Matrix4x4d& Matrix4x4d::translate(double x, double y, double z, Matrix4x4d::Order order){
		return this->multiply(
			Matrix4x4d(
				1, 0, 0, x,
				0, 1, 0, y,
				0, 0, 1, z,
				0, 0, 0, 1
			),
			order
		);
	}
	Matrix4x4d& Matrix4x4d::scale(double x, double y, double z, Matrix4x4d::Order order){
		return this->multiply(
			Matrix4x4d(
				x, 0, 0, 0,
				0, y, 0, 0,
				0, 0, z, 0,
				0, 0, 0, 1
			),
			order
		);
	}
	Matrix4x4d& Matrix4x4d::rotate_x(double rad, Matrix4x4d::Order order){
		return this->multiply(
			Matrix4x4d(
				1, 0, 0, 0,
				0, ::cos(rad), -::sin(rad), 0,
				0, ::sin(rad), ::cos(rad), 0,
				0, 0, 0, 1
			),
			order
		);
	}
	Matrix4x4d& Matrix4x4d::rotate_y(double rad, Matrix4x4d::Order order){
		return this->multiply(
			Matrix4x4d(
				::cos(rad), 0, ::sin(rad), 0,
				0, 1, 0, 0,
				-::sin(rad), 0, ::cos(rad), 0,
				0, 0, 0, 1
			),
			order
		);
	}
	Matrix4x4d& Matrix4x4d::rotate_z(double rad, Matrix4x4d::Order order){
		return this->multiply(
			Matrix4x4d(
				::cos(rad), -::sin(rad), 0, 0,
				::sin(rad), ::cos(rad), 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			),
			order
		);
	}
}
