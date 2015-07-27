/*
Project: SSBRenderer
File: blurred.cpp

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
#include <vector>
#include <stdexcept>

int main(){
	if(!write_tga("original1.tga", test_image1.width, test_image1.height, test_image1.stride, test_image1.has_alpha, test_image1.data))
		throw std::domain_error("Couldn't write to first original file!");
	std::vector<unsigned char> buffer(test_image1.data, test_image1.data + test_image1.height*test_image1.stride);
	GUtils::blur(buffer.data(), test_image1.width, test_image1.height, test_image1.stride, test_image1.has_alpha ? GUtils::ColorDepth::X4 : GUtils::ColorDepth::X3, 2.5, 8.2);
	if(!write_tga("blurred1.tga", test_image1.width, test_image1.height, test_image1.stride, test_image1.has_alpha, buffer.data()))
		throw std::domain_error("Couldn't write to blurred first file!");
	return 0;
}
