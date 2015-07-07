/*
Project: SSBRenderer
File: transform.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../gutils.hpp"
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <stdexcept>

int main(){
	double vec[] = {1, 0, 0}, *vec_end = vec+(sizeof(vec)/sizeof(*vec)),
		transformed_vec[] = {-3, 0, 0};
	if(std::equal(GUtils::Matrix4x4d().scale(3, 1, 1).rotate_z(M_PI).transform3d(vec), vec_end, transformed_vec))
		std::copy(vec, vec_end, std::ostream_iterator<double>(std::cout, " "));
	else
		throw std::logic_error("Transformation failed");
	return 0;
}
