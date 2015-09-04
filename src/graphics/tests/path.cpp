/*
Project: SSBRenderer
File: path.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../gutils.hpp"
#include <stdexcept>
#include <iostream>

using namespace GUtils;
using PType = PathSegment::Type;

static void print_path(const std::vector<GUtils::PathSegment>& path){
        for(auto& segment : path)
                switch(segment.type){
			case PType::MOVE: std::cout << "M " << segment.x << '|' << segment.y << std::endl; break;
			case PType::LINE: std::cout << "L " << segment.x << '|' << segment.y << std::endl; break;
			case PType::CURVE: std::cout << "B " << segment.x << '|' << segment.y << std::endl; break;
			case PType::CLOSE: std::cout << 'C' << std::endl; break;
                }
}

int main(){
	std::vector<PathSegment> path{
		{PType::MOVE, 10, 20.5},
		{PType::LINE, 19, 11},
		{PType::CURVE, 14, 3},
		{PType::CURVE, 11, 13},
		{PType::CURVE, 9, 10},
	};
	double x0, y0, x1, y1;
	if(!path_extents(path, &x0, &y0, &x1, &y1) || x0 != 9 || y0 != 3 || x1 != 19 || y1 != 20.5)
		throw std::logic_error("Path extents aren't right");
	path_close(path);
	if(path.back().type != PType::CLOSE)
		throw std::logic_error("Path closing didn't happen");
        print_path(path_flatten(path, 0.035));
	print_path(path_by_arc(0, 30, 30, 30, 3.2));
	return 0;
}
