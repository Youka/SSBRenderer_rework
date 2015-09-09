/*
Project: SSBRenderer
File: Geometry.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "Geometry.hpp"

#define DEG_TO_RAD(x) (x * M_PI / 180.0)

namespace SSB{
	std::vector<GUtils::PathSegment> points_to_path(const Points* points, double size){
		std::vector<GUtils::PathSegment> result;
		if(size == 1.0)	// Special rule: size-1 points become pixels/1x1 rectangles
			for(const Point& point : points->points)
				result.push_back({GUtils::PathSegment::Type::MOVE, point.x-0.5, point.y-0.5}),
				result.push_back({GUtils::PathSegment::Type::LINE, point.x+0.5, point.y-0.5}),
				result.push_back({GUtils::PathSegment::Type::LINE, point.x+0.5, point.y+0.5}),
				result.push_back({GUtils::PathSegment::Type::LINE, point.x-0.5, point.y+0.5}),
				result.push_back({GUtils::PathSegment::Type::CLOSE});
		else
			for(const Point& point : points->points){
				static constexpr const double pi_2 = M_PI * 2;
				const GUtils::PathSegment start = {GUtils::PathSegment::Type::MOVE, point.x - size / 2, point.y};
				const std::vector<GUtils::PathSegment> curves = GUtils::path_by_arc(start.x, start.y, point.x, point.y, pi_2);
				result.push_back(start),
				result.insert(result.end(), curves.begin(), curves.end()),
				result.push_back({GUtils::PathSegment::Type::CLOSE});
			}
		return result;
	}

	std::vector<GUtils::PathSegment> path_to_path(const Path* path){
		std::vector<GUtils::PathSegment> result;
		result.reserve(path->segments.size());
		for(auto segment_iter = path->segments.begin(), segment_iter_end = path->segments.end(); segment_iter != segment_iter_end; ++segment_iter){
			const Path::Segment& segment = *segment_iter;
			switch(segment.type){
				case Path::SegmentType::MOVE_TO:
					result.push_back({GUtils::PathSegment::Type::MOVE, segment.point.x, segment.point.y});
					break;
				case Path::SegmentType::LINE_TO:
					result.push_back({GUtils::PathSegment::Type::LINE, segment.point.x, segment.point.y});
					break;
				case Path::SegmentType::CURVE_TO:
					result.push_back({GUtils::PathSegment::Type::CURVE, segment.point.x, segment.point.y});
					break;
				case Path::SegmentType::ARC_TO:
					if(!result.empty() && result.back().type != GUtils::PathSegment::Type::CLOSE &&
					segment_iter+1 != segment_iter_end && (segment_iter+1)->type == Path::SegmentType::ARC_TO){
						const GUtils::PathSegment& last_segment = result.back();
						const Point& cur_point = segment.point;
						const std::vector<GUtils::PathSegment> curves = GUtils::path_by_arc(last_segment.x, last_segment.y, cur_point.x, cur_point.y, DEG_TO_RAD((++segment_iter)->angle));
						result.insert(result.end(), curves.begin(), curves.end());
					}
					break;
				case Path::SegmentType::CLOSE:
					result.push_back({GUtils::PathSegment::Type::CLOSE});
					break;
			}
		}
		return result;
	}

	void get_2d_scale(unsigned src_width, unsigned src_height, unsigned dst_width, unsigned dst_height, double& scale_x, double& scale_y){
		if(dst_width > 0 && dst_height > 0)
			scale_x = static_cast<double>(src_width) / dst_width,
			scale_y = static_cast<double>(src_height) / dst_height;
		else
			scale_x = scale_y = 0;
        }

        Point get_auto_pos(Align::Position align,
			Coord margin_h, Coord margin_v,
			unsigned frame_width, unsigned frame_height,
			double scale_x, double scale_y){
		if(scale_x && scale_y)
			switch(align){
				case Align::Position::LEFT_BOTTOM: return {margin_h * scale_x, frame_height - margin_v * scale_y};
				case Align::Position::CENTER_BOTTOM: return {frame_width / 2.0, frame_height - margin_v * scale_y};
				case Align::Position::RIGHT_BOTTOM: return {frame_width - margin_h * scale_x, frame_height - margin_v * scale_y};
				case Align::Position::LEFT_MIDDLE: return {margin_h * scale_x, frame_height / 2.0};
				case Align::Position::CENTER_MIDDLE: return {frame_width / 2.0, frame_height / 2.0};
				case Align::Position::RIGHT_MIDDLE: return {frame_width - margin_h * scale_x, frame_height / 2.0};
				case Align::Position::LEFT_TOP: return {margin_h * scale_x, margin_v * scale_y};
				case Align::Position::CENTER_TOP: return {frame_width / 2.0, margin_v * scale_y};
				case Align::Position::RIGHT_TOP: return {frame_width - margin_h * scale_x, margin_v * scale_y};
				default: return {0, 0};
			}
		else
			switch(align){
				case Align::Position::LEFT_BOTTOM: return {margin_h, frame_height - margin_v};
				case Align::Position::CENTER_BOTTOM: return {frame_width / 2.0, frame_height - margin_v};
				case Align::Position::RIGHT_BOTTOM: return {frame_width - margin_h, frame_height - margin_v};
				case Align::Position::LEFT_MIDDLE: return {margin_h, frame_height / 2.0};
				case Align::Position::CENTER_MIDDLE: return {frame_width / 2.0, frame_height / 2.0};
				case Align::Position::RIGHT_MIDDLE: return {frame_width - margin_h, frame_height / 2.0};
				case Align::Position::LEFT_TOP: return {margin_h, margin_v};
				case Align::Position::CENTER_TOP: return {frame_width / 2.0, margin_v};
				case Align::Position::RIGHT_TOP: return {frame_width - margin_h, margin_v};
				default: return {0, 0};
			}
        }

        void path_deform(std::vector<GUtils::PathSegment>& path, const std::string& x_formula, const std::string& y_formula, double progress){

		// TODO

        }

	// TODO

}
