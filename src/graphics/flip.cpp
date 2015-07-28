/*
Project: SSBRenderer
File: flip.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "gutils.hpp"
#include <memory>
#include <algorithm>

namespace GUtils{
	void flip(unsigned char* data, const unsigned height, const unsigned stride){
		std::unique_ptr<unsigned char> tmp(new unsigned char[stride]);
		unsigned char* data_tail = data + (height-1) * stride;
		for(const unsigned char* const data_stop = data + (height >> 1) * stride; data != data_stop; data = std::copy(tmp.get(), tmp.get()+stride, data))
			std::copy(data_tail, data_tail+stride, tmp.get()),
			data_tail = std::copy(data, data+stride, data_tail) - (stride << 1);
	}
	void flip(const unsigned char* src_data, const unsigned height, const unsigned stride, unsigned char* dst_data){
		dst_data += (height-1) * stride;
		for(const unsigned char* const src_data_end = src_data + height * stride; src_data != src_data_end; src_data += stride)
			dst_data = std::copy(src_data, src_data+stride, dst_data) - (stride << 1);
	}
}
