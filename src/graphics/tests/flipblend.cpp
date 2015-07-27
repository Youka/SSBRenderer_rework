/*
Project: SSBRenderer
File: flipblend.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../gutils.hpp"
#include "tga.hpp"
#include "test_images.h"
#include <memory>
#include <stdexcept>

int main(){
	// Write original images as reference
	if(!write_tga("original1.tga", test_image1.width, test_image1.height, test_image1.stride, test_image1.has_alpha, test_image1.data))
		throw std::domain_error("Couldn't write to first original file!");
	if(!write_tga("original2.tga", test_image2.width, test_image2.height, test_image2.stride, test_image2.has_alpha, test_image2.data))
		throw std::domain_error("Couldn't write to second original file!");
	// Write flipped image2
	std::unique_ptr<unsigned char> buffer(new unsigned char[test_image2.height * test_image2.stride]);
	GUtils::flip(const_cast<const unsigned char*>(test_image2.data), test_image2.height, test_image2.stride, buffer.get());
	if(!write_tga("flip2.tga", test_image2.width, test_image2.height, test_image2.stride, test_image2.has_alpha, buffer.get()))
		throw std::domain_error("Couldn't write to flipped second file!");
	// Write image2 blended on image1
	const unsigned test_image1_size = test_image1.height * test_image1.stride;
	buffer.reset(new unsigned char[test_image1_size]);
	std::copy(test_image1.data, test_image1.data+test_image1_size, buffer.get());
	GUtils::blend(test_image2.data, test_image2.width, test_image2.height, test_image2.stride, test_image2.has_alpha,
		buffer.get(), test_image1.width, test_image1.height, test_image1.stride, test_image1.has_alpha,
		100, 50, GUtils::BlendOp::DIFF);
	if(!write_tga("blend.tga", test_image1.width, test_image1.height, test_image1.stride, test_image1.has_alpha, buffer.get()))
		throw std::domain_error("Couldn't write to blend result file!");
	return 0;
}
