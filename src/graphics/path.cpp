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

#include "gutils.hpp"

namespace GUtils{
	bool path_extents(const std::vector<PathSegment>& path, double* x0, double* y0, double* x1, double* y1){
		// Temporary result storages
		bool one_found = false;
		double temp_x0, temp_y0, temp_x1, temp_y1;
		// Find extreme points in path
		for(auto& segment : path)
			if(segment.type != PathSegment::Type::CLOSE){
				if(!one_found)
					temp_x0 = temp_x1 = segment.x,
					temp_y0 = temp_y1 = segment.y,
					one_found = true;
				else{
					if(segment.x < temp_x0)
						temp_x0 = segment.x;
					if(segment.y < temp_y0)
						temp_y0 = segment.y;
					if(segment.x > temp_x1)
						temp_x1 = segment.x;
					if(segment.y > temp_y1)
						temp_y1 = segment.y;
				}
			}
		// Anything found for result?
		if(!one_found)
			return false;
		// Assign result to output
		if(x0) *x0 = temp_x0;
		if(y0) *y0 = temp_y0;
		if(x1) *x1 = temp_x1;
		if(y1) *y1 = temp_y1;
		// Output (may) has changed
		return true;
	}
	void path_close(std::vector<PathSegment>& path){
		for(size_t i = 1; i < path.size(); ++i)
			if(path[i].type == PathSegment::Type::MOVE && path[i-1].type != PathSegment::Type::CLOSE)
				path.insert(path.begin()+i, {PathSegment::Type::CLOSE}),
				++i;
		if(!path.empty() && path.back().type != PathSegment::Type::CLOSE)
			path.push_back({PathSegment::Type::CLOSE});
	}
	void path_flatten(std::vector<PathSegment>& path){
		// Buffer for new path
		std::vector<PathSegment> new_path;
		new_path.reserve(path.size());
		// Go through path segments
		for(auto iter = path.begin(); iter != path.end(); ++iter)
			if(iter->type == PathSegment::Type::CURVE && iter+2 < path.end() && (iter+1)->type == PathSegment::Type::CURVE && (iter+2)->type == PathSegment::Type::CURVE){

				// TODO

			}else
				new_path.push_back(*iter);
		// Transfer new path data to old path
		path = std::move(new_path);
	}
	std::vector<PathSegment> path_by_arc(double x, double y, double cx, double cy, double angle){
		std::vector<PathSegment> curve_segments;

		// TODO

		return curve_segments;
	}
}
