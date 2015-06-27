/*
Project: SSBRenderer
File: gutils.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <type_traits>

namespace GUtils{
	struct vector_features{
		bool sse2, avx;
	};
	vector_features detect_vectorization();

	class Matrix4x4d{
		private:
			// Raw matrix data
			std::aligned_storage<sizeof(double)<<4, 16>::type storage;	// MSVC doesn't support keyword 'alignas'
			double* matrix = reinterpret_cast<decltype(this->matrix)>(&this->storage);
			// Identity matrix template
			constexpr static const double identity_matrix[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
		public:
			// Ctors, dtor & assignments (rule-of-five)
			Matrix4x4d();
			Matrix4x4d(double* matrix);
			Matrix4x4d(const Matrix4x4d& other);
			Matrix4x4d& operator=(const Matrix4x4d& other);
			Matrix4x4d(Matrix4x4d&& other) = delete;	// No movable resources
			Matrix4x4d& operator=(Matrix4x4d&& other) = delete;
			virtual ~Matrix4x4d();
			// Raw data getter
			double* data() const;
			// Reset to identity matrix
			Matrix4x4d& identity();
			// General transformations
			enum class Order{PREPEND, APPEND};
			Matrix4x4d& multiply(const Matrix4x4d& other, Order order = Order::PREPEND);
	};
}
