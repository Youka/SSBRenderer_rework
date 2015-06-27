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
#include <emmintrin.h>	// SSE2 instrincs
#include <immintrin.h>	// AVX instrincs

namespace GUtils{
	Matrix4x4d::Matrix4x4d(){std::copy(Matrix4x4d::identity_matrix, Matrix4x4d::identity_matrix+16, this->matrix);}
	Matrix4x4d::Matrix4x4d(double* matrix){std::copy(matrix, matrix+16, this->matrix);}
	Matrix4x4d::Matrix4x4d(const Matrix4x4d& other){Matrix4x4d(other.matrix);}
	Matrix4x4d& Matrix4x4d::operator=(const Matrix4x4d& other){Matrix4x4d(other.matrix); return *this;}
	Matrix4x4d::~Matrix4x4d(){}
	double* Matrix4x4d::data() const{return this->matrix;}
	Matrix4x4d& Matrix4x4d::identity(){Matrix4x4d(); return *this;}
	Matrix4x4d& Matrix4x4d::multiply(const Matrix4x4d& other, Matrix4x4d::Order order){
		double* matrix1, *matrix2;
		vector_features features = detect_vectorization();
		if(features.sse2){
			decltype(this->storage) storage;
			if(order == Matrix4x4d::Order::PREPEND)
				std::copy(this->matrix, this->matrix+16, matrix1 = reinterpret_cast<decltype(matrix1)>(&storage)), matrix2 = other.matrix;
			else
				matrix1 = other.matrix, std::copy(this->matrix, this->matrix+16, matrix2 = reinterpret_cast<decltype(matrix2)>(&storage));
			if(features.avx){
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
			}else{	// features.sse2
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
			}
		}else{
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
		}
		return *this;
	}
}
