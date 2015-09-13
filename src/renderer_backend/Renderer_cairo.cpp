/*
Project: SSBRenderer
File: Renderer_cairo.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "Renderer.hpp"
#include <cairo.h>
#include <memory>
#include "../graphics/gutils.hpp"

// Cairo surface+context bounding
struct cairo_image_t{
	cairo_surface_t* surface;
	cairo_t* context;
};
auto cairo_image_destroyer = [](cairo_image_t* img){
	cairo_destroy(img->context),
	cairo_surface_destroy(img->surface),
	delete img;
};

// Renderer private data
using cairo_image_safe_ptr = std::unique_ptr<cairo_image_t, std::function<void(cairo_image_t*)>>;
struct InstanceData{
	// Buffers
	cairo_image_safe_ptr image, stencil;
	// State
	GUtils::Font font;
	bool vertical;
	std::string deform_x, deform_y;
        double deform_progress;
        GUtils::Matrix4x4d matrix;
        enum class Mode{FILL, WIRE, BOXED} mode;
        double fill_color[4][4], line_color[4][4];	// 4-corners, RGBA
	std::string texture_filename;
	double texture_x, texture_y;
	cairo_extend_t texture_wrap;
	bool anti_aliasing;
	// Rest in cairo context or extern
};

namespace Backend{
	Renderer::Renderer() : Renderer::Renderer(1, 1){}

	Renderer::Renderer(unsigned width, unsigned height){
		this->data = new InstanceData{
			cairo_image_safe_ptr(nullptr, cairo_image_destroyer),
			cairo_image_safe_ptr(nullptr, cairo_image_destroyer)
		},
		this->set_size(width, height),
		this->reset();
	}

	void Renderer::set_size(unsigned width, unsigned height){
		InstanceData* data = reinterpret_cast<InstanceData*>(this->data);
		cairo_surface_t* image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height),
			*stencil = cairo_image_surface_create(CAIRO_FORMAT_A8, width, height);
		data->image.reset(new cairo_image_t{image, cairo_create(image)}),
		data->stencil.reset(new cairo_image_t{stencil, cairo_create(stencil)});
	}

	Renderer::~Renderer(){
		delete reinterpret_cast<InstanceData*>(this->data);
	}

	unsigned Renderer::width(){
		return cairo_image_surface_get_width(reinterpret_cast<InstanceData*>(this->data)->image->surface);
	}

	unsigned Renderer::height(){
		return cairo_image_surface_get_height(reinterpret_cast<InstanceData*>(this->data)->image->surface);
	}

	void Renderer::copy_image(unsigned char* image, unsigned padding){
		// Get source data
		cairo_surface_t* image_surface = reinterpret_cast<InstanceData*>(this->data)->image->surface;
		int rowsize = cairo_image_surface_get_width(image_surface) << 2, stride = cairo_image_surface_get_stride(image_surface), height = cairo_image_surface_get_height(image_surface);
		cairo_surface_flush(image_surface);
		unsigned char* src_data = cairo_image_surface_get_data(image_surface);
		// Copy source rows to destination
		for(const unsigned char* const src_data_end = src_data + height * stride; src_data != src_data_end; src_data += stride)
			image = std::copy(src_data, src_data+rowsize, image) + padding;
	}

	void Renderer::reset(){
		// Get instance data
		InstanceData* data = reinterpret_cast<InstanceData*>(this->data);
		// Reset local state
		data->font = GUtils::Font("Arial"),
		data->vertical = false,
                data->deform_x.clear(),
		data->deform_y.clear(),
		data->deform_progress = 0,
		data->matrix.identity(),
		data->mode = InstanceData::Mode::FILL,
		std::fill(&data->fill_color[0][0], &data->fill_color[3][3], 1),
		std::fill(&data->line_color[0][0], &data->line_color[3][3], 0),
		data->line_color[0][3] = data->line_color[1][3] = data->line_color[2][3] = data->line_color[3][3] = 1,
		data->texture_filename.clear(),
		data->texture_x = data->texture_y = 0,
		data->texture_wrap = CAIRO_EXTEND_NONE,
		data->anti_aliasing = true;
		// Get cairo contexts
		cairo_t* image_ctx = data->image->context,
			*stencil_ctx = data->stencil->context;
		// Reset cairo state
		cairo_set_line_width(image_ctx, 4), cairo_set_line_width(stencil_ctx, 4),
		cairo_set_line_join(image_ctx, CAIRO_LINE_JOIN_ROUND), cairo_set_line_join(stencil_ctx, CAIRO_LINE_JOIN_ROUND),
		cairo_set_line_cap(image_ctx, CAIRO_LINE_CAP_ROUND), cairo_set_line_cap(stencil_ctx, CAIRO_LINE_CAP_ROUND),
		cairo_set_dash(image_ctx, nullptr, 0, 0), cairo_set_dash(stencil_ctx, nullptr, 0, 0),
		cairo_set_antialias(image_ctx, CAIRO_ANTIALIAS_BEST), cairo_set_antialias(stencil_ctx, CAIRO_ANTIALIAS_BEST);
	}

	void Renderer::clear_stencil(){
		cairo_t* stencil_context = reinterpret_cast<InstanceData*>(this->data)->stencil->context;
		cairo_set_operator(stencil_context, CAIRO_OPERATOR_SOURCE),
		cairo_set_source_rgba(stencil_context, 0, 0, 0, 0),
		cairo_paint(stencil_context);
	}
}
