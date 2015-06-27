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

				// TODO: AVX matrix multiplication

			}else{	// features.sse2
				__m128d a = _mm_load1_pd(matrix);

				// TODO: SSE2 matrix multiplication

			}
		}else{
			double matrix[16];
			std::copy(this->matrix, this->matrix+16, matrix);
			if(order == Matrix4x4d::Order::PREPEND)
				matrix1 = matrix, matrix2 = other.matrix;
			else	// order == Matrix4x4d::Order::APPEND
				matrix1 = other.matrix, matrix2 = matrix;
			this->matrix[0] = matrix[0] * matrix2[0] + matrix[1] * matrix2[4] + matrix[2] * matrix2[8] + matrix[3] * matrix2[12],
			this->matrix[1] = matrix[0] * matrix2[1] + matrix[1] * matrix2[5] + matrix[2] * matrix2[9] + matrix[3] * matrix2[13],
			this->matrix[2] = matrix[0] * matrix2[2] + matrix[1] * matrix2[6] + matrix[2] * matrix2[10] + matrix[3] * matrix2[14],
			this->matrix[3] = matrix[0] * matrix2[3] + matrix[1] * matrix2[7] + matrix[2] * matrix2[11] + matrix[3] * matrix2[15],
			this->matrix[4] = matrix[4] * matrix2[0] + matrix[5] * matrix2[4] + matrix[6] * matrix2[8] + matrix[7] * matrix2[12],
			this->matrix[5] = matrix[4] * matrix2[1] + matrix[5] * matrix2[5] + matrix[6] * matrix2[9] + matrix[7] * matrix2[13],
			this->matrix[6] = matrix[4] * matrix2[2] + matrix[5] * matrix2[6] + matrix[6] * matrix2[10] + matrix[7] * matrix2[14],
			this->matrix[7] = matrix[4] * matrix2[3] + matrix[5] * matrix2[7] + matrix[6] * matrix2[11] + matrix[7] * matrix2[15],
			this->matrix[8] = matrix[8] * matrix2[0] + matrix[9] * matrix2[4] + matrix[10] * matrix2[8] + matrix[11] * matrix2[12],
			this->matrix[9] = matrix[8] * matrix2[1] + matrix[9] * matrix2[5] + matrix[10] * matrix2[9] + matrix[11] * matrix2[13],
			this->matrix[10] = matrix[8] * matrix2[2] + matrix[9] * matrix2[6] + matrix[10] * matrix2[10] + matrix[11] * matrix2[14],
			this->matrix[11] = matrix[8] * matrix2[3] + matrix[9] * matrix2[7] + matrix[10] * matrix2[11] + matrix[11] * matrix2[15],
			this->matrix[12] = matrix[12] * matrix2[0] + matrix[13] * matrix2[4] + matrix[14] * matrix2[8] + matrix[15] * matrix2[12],
			this->matrix[13] = matrix[12] * matrix2[1] + matrix[13] * matrix2[5] + matrix[14] * matrix2[9] + matrix[15] * matrix2[13],
			this->matrix[14] = matrix[12] * matrix2[2] + matrix[13] * matrix2[6] + matrix[14] * matrix2[10] + matrix[15] * matrix2[14],
			this->matrix[15] = matrix[12] * matrix2[3] + matrix[13] * matrix2[7] + matrix[14] * matrix2[11] + matrix[15] * matrix2[15];
		}
		return *this;
	}
}
