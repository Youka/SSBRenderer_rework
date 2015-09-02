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
#include <algorithm>

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
	cairo_image_safe_ptr image, stencil;
};

Renderer::Renderer() : Renderer::Renderer(1, 1){}

Renderer::Renderer(unsigned width, unsigned height){
	this->data = nullptr,
	this->set_size(width, height);
}

void Renderer::set_size(unsigned width, unsigned height){
	delete reinterpret_cast<InstanceData*>(this->data);
	cairo_surface_t* image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height),
		*stencil = cairo_image_surface_create(CAIRO_FORMAT_A8, width, height);
	this->data = new InstanceData{
		cairo_image_safe_ptr(new cairo_image_t{image, cairo_create(image)}, cairo_image_destroyer),
		cairo_image_safe_ptr(new cairo_image_t{stencil, cairo_create(stencil)}, cairo_image_destroyer)
	};
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
