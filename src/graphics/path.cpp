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

#define _USE_MATH_DEFINES
#include <cmath>
#include "gutils.hpp"
#include <array>
#include "simd.h"

static void rotate(double& x, double& y, double angle){
	const double temp_x = x;
        x = ::cos(angle) * x - ::sin(angle) * y,
        y = ::sin(angle) * temp_x + ::cos(angle) * y;
}
static void rotate90(double& x, double& y, bool neg = false){
	const double temp_x = x;
	if(neg)
		x = y,
		y = -temp_x;
	else
		x = -y,
		y = temp_x;
}
static void rotate45(double& x, double& y, bool neg = false){
	const double temp_x = x;
	if(neg)
		x = M_SQRT1_2 * x + M_SQRT1_2 * y,
		y = -M_SQRT1_2 * temp_x + M_SQRT1_2 * y;
	else
		x = M_SQRT1_2 * x - M_SQRT1_2 * y,
		y = M_SQRT1_2 * temp_x + M_SQRT1_2 * y;
}
static std::array<double,16> curve_split(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3){
#ifdef __SSE2__
	__m128d split_operand = _mm_set1_pd(0.5),
		xy0 = _mm_set_pd(y0, x0),
		xy1 = _mm_set_pd(y1, x1),
		xy2 = _mm_set_pd(y2, x2),
		xy3 = _mm_set_pd(y3, x3);
#ifdef __AVX__
	__m256d split_operand256 = _mm256_set1_pd(0.5),
		xy01_12 = _mm256_mul_pd(
			_mm256_add_pd(
				_mm256_set_m128d(xy1, xy0),
				_mm256_set_m128d(xy2, xy1)
			),
			split_operand256
		);
	__m128d xy01 = _mm256_extractf128_pd(xy01_12, 0x0),
		xy12 = _mm256_extractf128_pd(xy01_12, 0x1),
#else
	__m128d xy01 = _mm_mul_pd(
			_mm_add_pd(
				xy0,
				xy1
			),
			split_operand
		),
		xy12 = _mm_mul_pd(
			_mm_add_pd(
				xy1,
				xy2
			),
			split_operand
		),
#endif
		xy23 = _mm_mul_pd(
			_mm_add_pd(
				xy2,
				xy3
			),
			split_operand
		);
#ifdef __AVX__
	__m256d xy012_123 = _mm256_mul_pd(
		_mm256_add_pd(
			xy01_12,
			_mm256_set_m128d(xy23, xy12)
),		),
		split_operand256
	);
	__m128d xy012 = _mm256_extractf128_pd(xy012_123, 0x0),
		xy123 = _mm256_extractf128_pd(xy012_123, 0x1),
#else
	__m128d xy012 = _mm_mul_pd(
			_mm_add_pd(
				xy01,
				xy12
			),
			split_operand
		),
		xy123 = _mm_mul_pd(
			_mm_add_pd(
				xy12,
				xy23
			),
			split_operand
		),
#endif
		xy0123 = _mm_mul_pd(
			_mm_add_pd(
				xy012,
				xy123
			),
			split_operand
		);
	std::array<double,16> result;
	_mm_storeu_pd(&result[0], xy0),
	_mm_storeu_pd(&result[2], xy01),
	_mm_storeu_pd(&result[4], xy012),
	_mm_storeu_pd(&result[6], xy0123),
	_mm_storeu_pd(&result[8], xy0123),
	_mm_storeu_pd(&result[10], xy123),
	_mm_storeu_pd(&result[12], xy23),
	_mm_storeu_pd(&result[14], xy3);
	return result;
#else
	const double x01 = (x0+x1) / 2,
		y01 = (y0+y1) / 2,
		x12 = (x1+x2) / 2,
		y12 = (y1+y2) / 2,
		x23 = (x2+x3) / 2,
		y23 = (y2+y3) / 2,
		x012 = (x01+x12) / 2,
		y012 = (y01+y12) / 2,
		x123 = (x12+x23) / 2,
		y123 = (y12+y23) / 2,
		x0123 = (x012+x123) / 2,
		y0123 = (y012+y123) / 2;
	return {x0, y0, x01, y01, x012, y012, x0123, y0123,
		x0123, y0123, x123, y123, x23, y23, x3, y3};
#endif
}
static inline bool vec_zero_length(double vx, double vy){
	return !(vx || vy);
}
static double angle_vec_x_vec(double v0x, double v0y, double v1x, double v1y){
	// Check for zero-length vectors
	if(vec_zero_length(v0x, v0y) || vec_zero_length(v1x, v1y))
		return 0;
	// Calculate angle between vectors
	return ::acos((v0x * v1x + v0y * v1y) / (::hypot(v0x, v0y) * ::hypot(v1x, v1y)));
}
static bool curve_is_flat(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, double tolerance_angle){
	// Vectors between curve points
	const double v0x = x1 - x0,
		v0y = y1 - y0,
		v1x = x2 - x1,
		v1y = y2 - y1,
		v2x = x3 - x2,
		v2y = y3 - y2;
	// Sort out zero-length vectors
	std::vector<double> vecs;
	vecs.reserve(6);
	if(!vec_zero_length(v0x, v0y))
		vecs.push_back(v0x),
		vecs.push_back(v0y);
	if(!vec_zero_length(v1x, v1y))
		vecs.push_back(v1x),
		vecs.push_back(v1y);
	if(!vec_zero_length(v2x, v2y))
		vecs.push_back(v2x),
		vecs.push_back(v2y);
	// Check vectors angles against given tolerance
        for(auto iter = vecs.begin()+2; iter < vecs.end(); iter+=2)
		if(angle_vec_x_vec(*(iter-2), *(iter-1), *iter, *(iter+1)) > tolerance_angle)
			return false;
	return true;
}

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
		// Result buffer
		std::vector<PathSegment> curve_segments;
		// Anything to do?
		if(angle != 0.0 && (x != cx || y != cy)){
			// Constant for curve point offset
			static constexpr const double kappa = 4 * (::sqrt(2) - 1) / 3;
			// Angle direction
			const bool neg_angle = angle < 0;
			// Process 90 degree chunks
			double rx0 = x - cx,
				ry0 = y - cy,
				angle_passed = 0;
			const double abs_angle = ::abs(angle);
			do{
				// Get curve end point
				const double cur_angle = std::min(abs_angle - angle_passed, M_PI_2);
				double rx3 = rx0,
					ry3 = ry0;
				if(cur_angle == M_PI_2)
					rotate90(rx3, ry3, neg_angle);
				else if(cur_angle == M_PI_4)
					rotate45(rx3, ry3, neg_angle);
				else
					rotate(rx3, ry3, neg_angle ? -cur_angle : cur_angle);
				// Get curve start-to-end vector
				double rx03 = rx3 - rx0,
					ry03 = ry3 - ry0;
				// Scale vector to control point length
				const double len = ::hypot(rx03, ry03),
					scale = ::sqrt((len*len) / 2) * kappa / len;
				rx03 *= scale,
				ry03 *= scale;
				// Get curve control points
				double rx1 = rx03,
					ry1 = ry03,
					rx2 = -rx03,
					ry2 = -ry03;
				if(cur_angle == M_PI_2)
					rotate45(rx1, ry1, !neg_angle),
					rotate45(rx2, ry2, neg_angle);
				else{
					const double cur_angle_2 = cur_angle / 2;
					rotate(rx1, ry1, !neg_angle ? -cur_angle_2 : cur_angle_2),
					rotate(rx2, ry2, neg_angle ? -cur_angle_2 : cur_angle_2);
				}
                                rx1 += rx0,
                                ry1 += ry0,
                                rx2 += rx3,
                                ry2 += ry3;
                                // Insert curve to result
                                curve_segments.push_back({PathSegment::Type::CURVE, cx+rx1, cy+ry1}),
                                curve_segments.push_back({PathSegment::Type::CURVE, cx+rx2, cy+ry2}),
                                curve_segments.push_back({PathSegment::Type::CURVE, cx+rx3, cy+ry3});
				// Update current curve start point & passed angle of previous
				rx0 = rx3,
				ry0 = ry3,
				angle_passed += M_PI_2;
			}while(angle_passed < abs_angle);
		}
		// Return constructed result
		return curve_segments;
	}
}
