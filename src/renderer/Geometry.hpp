/*
Project: SSBRenderer
File: Geometry.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "../graphics/gutils.hpp"
#include "../parser/SSBData.hpp"

namespace SSB{
	// Convert SSB points to general path
	std::vector<GUtils::PathSegment> points_to_path(const Points* points, double size);
	// Convert SSB path to general path
	std::vector<GUtils::PathSegment> path_to_path(const Path* path);
        // Calculate 2-dimensional scale by source & target
	void get_2d_scale(unsigned src_width, unsigned src_height, unsigned dst_width, unsigned dst_height, double& scale_x, double& scale_y);
        // Calculate auto position (by alignment, frame+scale and margins)
        Point get_auto_pos(Align::Position align,
			Coord margin_h, Coord margin_v,
			unsigned frame_width, unsigned frame_height,
			double scale_x, double scale_y);
	// Deform path by formula (with progress variable)
	void path_deform(std::vector<GUtils::PathSegment>& path, const std::string& x_formula, const std::string& y_formula, double progress);
        // Calculate geometries sizes & positions as whole and in lines

	// TODO

}
