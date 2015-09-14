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

// Cairo surface+context destroyer
auto cairo_destroyer = [](cairo_t* ctx){
	cairo_surface_t* surface = cairo_get_target(ctx);
	cairo_destroy(ctx),
	cairo_surface_destroy(surface);
};

// Renderer private data
using cairo_t_safe = std::unique_ptr<cairo_t, std::function<void(cairo_t*)>>;
struct InstanceData{
	// Buffers
	cairo_t_safe image, stencil;
	// State
	GUtils::Font font;
	std::string deform_x, deform_y;
	double deform_progress;
	GUtils::Matrix4x4d matrix;
	Backend::Renderer::Mode mode;
	std::vector<double> fill_color, line_color;	// RGBA, solid or 4 corners
	std::string texture_filename;
	double texture_x, texture_y;
	cairo_extend_t texture_wrap;
	// Rest in cairo context or extern
};
#define INST_DATA reinterpret_cast<InstanceData*>(this->data)

namespace Backend{
	Renderer::Renderer() : Renderer::Renderer(1, 1){}

	Renderer::Renderer(unsigned width, unsigned height){
		this->data = new InstanceData{
			cairo_t_safe(nullptr, cairo_destroyer),
			cairo_t_safe(nullptr, cairo_destroyer)
		},
		this->set_size(width, height),
		this->reset();
	}

	void Renderer::set_size(unsigned width, unsigned height){
		INST_DATA->image.reset(cairo_create(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height))),
		INST_DATA->stencil.reset(cairo_create(cairo_image_surface_create(CAIRO_FORMAT_A8, width, height)));
	}

	Renderer::~Renderer(){
		delete INST_DATA;
	}

	unsigned Renderer::width(){
		return cairo_image_surface_get_width(cairo_get_target(INST_DATA->image.get()));
	}

	unsigned Renderer::height(){
		return cairo_image_surface_get_height(cairo_get_target(INST_DATA->image.get()));
	}

	void Renderer::copy_image(unsigned char* image, unsigned padding){
		// Get source data
		cairo_surface_t* image_surface = cairo_get_target(INST_DATA->image.get());
		const int rowsize = cairo_image_surface_get_width(image_surface) << 2, stride = cairo_image_surface_get_stride(image_surface), height = cairo_image_surface_get_height(image_surface);
		cairo_surface_flush(image_surface);
		const unsigned char* src_data = cairo_image_surface_get_data(image_surface);
		// Copy source rows to destination
		for(const unsigned char* const src_data_end = src_data + height * stride; src_data != src_data_end; src_data += stride)
			image = std::copy(src_data, src_data+rowsize, image) + padding;
	}

	void Renderer::reset(){
		// Reset local state
		INST_DATA->font = GUtils::Font("Arial"),
                INST_DATA->deform_x.clear(),
		INST_DATA->deform_y.clear(),
		INST_DATA->deform_progress = 0,
		INST_DATA->matrix.identity(),
		INST_DATA->mode = Renderer::Mode::FILL,
		INST_DATA->fill_color = {1, 1, 1, 1},
		INST_DATA->line_color = {0, 0, 0, 1},
		INST_DATA->texture_filename.clear(),
		INST_DATA->texture_x = INST_DATA->texture_y = 0,
		INST_DATA->texture_wrap = CAIRO_EXTEND_NONE;
		// Get cairo contexts
		cairo_t* image_ctx = INST_DATA->image.get(),
			*stencil_ctx = INST_DATA->stencil.get();
		// Reset cairo state
		cairo_set_line_width(image_ctx, 4), cairo_set_line_width(stencil_ctx, 4),	// Line width = 2x border width
		cairo_set_line_join(image_ctx, CAIRO_LINE_JOIN_ROUND), cairo_set_line_join(stencil_ctx, CAIRO_LINE_JOIN_ROUND),
		cairo_set_line_cap(image_ctx, CAIRO_LINE_CAP_ROUND), cairo_set_line_cap(stencil_ctx, CAIRO_LINE_CAP_ROUND),
		cairo_set_dash(image_ctx, nullptr, 0, 0), cairo_set_dash(stencil_ctx, nullptr, 0, 0),
		cairo_set_antialias(image_ctx, CAIRO_ANTIALIAS_BEST), cairo_set_antialias(stencil_ctx, CAIRO_ANTIALIAS_BEST);
	}

	void Renderer::set_font(const std::string& family, float size, bool bold, bool italic, bool underline, bool strikeout, double spacing){
		INST_DATA->font = GUtils::Font(family, size, bold, italic, underline, strikeout, spacing);
	}

	void Renderer::set_deform(const std::string& x_formula, const std::string& y_formula, double progress){
		INST_DATA->deform_x = x_formula,
		INST_DATA->deform_y = y_formula,
		INST_DATA->deform_progress = progress;
	}

	void Renderer::set_matrix(const GUtils::Matrix4x4d& matrix){
		INST_DATA->matrix = matrix;
	}

	void Renderer::set_mode(Renderer::Mode mode){
		INST_DATA->mode = mode;
	}

	void Renderer::set_fill_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a){
		INST_DATA->fill_color = {r/255.0, g/255.0, b/255.0, a/255.0};
	}

	void Renderer::set_fill_color(unsigned char r0, unsigned char g0, unsigned char b0, unsigned char a0,
			unsigned char r1, unsigned char g1, unsigned char b1, unsigned char a1,
			unsigned char r2, unsigned char g2, unsigned char b2, unsigned char a2,
			unsigned char r3, unsigned char g3, unsigned char b3, unsigned char a3){
		INST_DATA->fill_color = {r0/255.0, g0/255.0, b0/255.0, a0/255.0, r1/255.0, g1/255.0, b1/255.0, a1/255.0, r2/255.0, g2/255.0, b2/255.0, a2/255.0, r3/255.0, g3/255.0, b3/255.0, a3/255.0};
	}

	void Renderer::set_line_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a){
		INST_DATA->fill_color = {r/255.0, g/255.0, b/255.0, a/255.0};
	}

	void Renderer::set_line_color(unsigned char r0, unsigned char g0, unsigned char b0, unsigned char a0,
			unsigned char r1, unsigned char g1, unsigned char b1, unsigned char a1,
			unsigned char r2, unsigned char g2, unsigned char b2, unsigned char a2,
			unsigned char r3, unsigned char g3, unsigned char b3, unsigned char a3){
		INST_DATA->line_color = {r0/255.0, g0/255.0, b0/255.0, a0/255.0, r1/255.0, g1/255.0, b1/255.0, a1/255.0, r2/255.0, g2/255.0, b2/255.0, a2/255.0, r3/255.0, g3/255.0, b3/255.0, a3/255.0};
	}

	void Renderer::set_texture(const std::string& filename){
		INST_DATA->texture_filename = filename;
	}

	void Renderer::set_texture_offset(double x, double y){
		INST_DATA->texture_x = x,
		INST_DATA->texture_y = y;
	}

	void Renderer::set_texture_wrap(Renderer::TexWrap wrap){
		switch(wrap){
			case Renderer::TexWrap::CLAMP: INST_DATA->texture_wrap = CAIRO_EXTEND_NONE; break;
			case Renderer::TexWrap::REPEAT: INST_DATA->texture_wrap = CAIRO_EXTEND_REPEAT; break;
			case Renderer::TexWrap::MIRROR: INST_DATA->texture_wrap = CAIRO_EXTEND_REFLECT; break;
			case Renderer::TexWrap::FLOW: INST_DATA->texture_wrap = CAIRO_EXTEND_PAD; break;
		}
	}

	void Renderer::set_line_width(double width){
		cairo_set_line_width(INST_DATA->image.get(), width), cairo_set_line_width(INST_DATA->stencil.get(), width);
	}

	void Renderer::set_line_join(Renderer::LineJoin join){
		switch(join){
			case Renderer::LineJoin::MITER:
				cairo_set_line_join(INST_DATA->image.get(), CAIRO_LINE_JOIN_MITER),
				cairo_set_line_join(INST_DATA->stencil.get(), CAIRO_LINE_JOIN_MITER);
				break;
			case Renderer::LineJoin::ROUND:
				cairo_set_line_join(INST_DATA->image.get(), CAIRO_LINE_JOIN_ROUND),
				cairo_set_line_join(INST_DATA->stencil.get(), CAIRO_LINE_JOIN_ROUND);
				break;
			case Renderer::LineJoin::BEVEL:
				cairo_set_line_join(INST_DATA->image.get(), CAIRO_LINE_JOIN_BEVEL),
				cairo_set_line_join(INST_DATA->stencil.get(), CAIRO_LINE_JOIN_BEVEL);
				break;
		}
	}

	void Renderer::set_line_cap(Renderer::LineCap cap){
		switch(cap){
			case Renderer::LineCap::FLAT:
				cairo_set_line_cap(INST_DATA->image.get(), CAIRO_LINE_CAP_BUTT),
				cairo_set_line_cap(INST_DATA->stencil.get(), CAIRO_LINE_CAP_BUTT);
				break;
			case Renderer::LineCap::ROUND:
				cairo_set_line_cap(INST_DATA->image.get(), CAIRO_LINE_CAP_ROUND),
				cairo_set_line_cap(INST_DATA->stencil.get(), CAIRO_LINE_CAP_ROUND);
				break;
			case Renderer::LineCap::SQUARE:
				cairo_set_line_cap(INST_DATA->image.get(), CAIRO_LINE_CAP_SQUARE),
				cairo_set_line_cap(INST_DATA->stencil.get(), CAIRO_LINE_CAP_SQUARE);
				break;
		}
	}

	void Renderer::set_line_dash(double offset, std::vector<double> dashes){
		cairo_set_dash(INST_DATA->image.get(), dashes.data(), dashes.size(), offset),
		cairo_set_dash(INST_DATA->stencil.get(), dashes.data(), dashes.size(), offset);
	}

	void Renderer::set_antialiasing(unsigned level){
		if(level)
			cairo_set_antialias(INST_DATA->image.get(), CAIRO_ANTIALIAS_BEST),
			cairo_set_antialias(INST_DATA->stencil.get(), CAIRO_ANTIALIAS_BEST);
		else
			cairo_set_antialias(INST_DATA->image.get(), CAIRO_ANTIALIAS_NONE),
			cairo_set_antialias(INST_DATA->stencil.get(), CAIRO_ANTIALIAS_NONE);
	}

	std::string Renderer::get_font_family(){
		return INST_DATA->font.get_family();
	}

	float Renderer::get_font_size(){
		return INST_DATA->font.get_size();
	}

	bool Renderer::get_font_bold(){
		return INST_DATA->font.get_bold();
	}

	bool Renderer::get_font_italic(){
		return INST_DATA->font.get_italic();
	}

	bool Renderer::get_font_underline(){
		return INST_DATA->font.get_underline();
	}

	bool Renderer::get_font_strikeout(){
		return INST_DATA->font.get_strikeout();
	}

	double Renderer::get_font_spacing(){
		return INST_DATA->font.get_spacing();
	}

	std::string Renderer::get_deform_x(){
		return INST_DATA->deform_x;
	}

	std::string Renderer::get_deform_y(){
		return INST_DATA->deform_y;
	}

	double Renderer::get_deform_progress(){
		return INST_DATA->deform_progress;
	}

	GUtils::Matrix4x4d Renderer::get_matrix(){
		return INST_DATA->matrix;
	}

	Renderer::Mode Renderer::get_mode(){
		return INST_DATA->mode;
	}

	std::vector<double> Renderer::get_fill_color(){
		return INST_DATA->fill_color;
	}

	std::vector<double> Renderer::get_line_color(){
		return INST_DATA->line_color;
	}

	std::string Renderer::get_texture(){
		return INST_DATA->texture_filename;
	}

	double Renderer::get_texture_offset_x(){
		return INST_DATA->texture_x;
	}

	double Renderer::get_texture_offset_y(){
		return INST_DATA->texture_y;
	}

	Renderer::TexWrap Renderer::get_texture_wrap(){
		switch(INST_DATA->texture_wrap){
			case CAIRO_EXTEND_NONE: return Renderer::TexWrap::CLAMP;
			case CAIRO_EXTEND_REPEAT: return Renderer::TexWrap::REPEAT;
			case CAIRO_EXTEND_REFLECT: return Renderer::TexWrap::MIRROR;
			case CAIRO_EXTEND_PAD: return Renderer::TexWrap::FLOW;
			default: return Renderer::TexWrap::CLAMP;
		}
	}

	double Renderer::get_line_width(){
		return cairo_get_line_width(INST_DATA->image.get());
	}

	Renderer::LineJoin Renderer::get_line_join(){
		switch(cairo_get_line_join(INST_DATA->image.get())){
			case CAIRO_LINE_JOIN_MITER: return Renderer::LineJoin::MITER;
			case CAIRO_LINE_JOIN_ROUND: return Renderer::LineJoin::ROUND;
			case CAIRO_LINE_JOIN_BEVEL: return Renderer::LineJoin::BEVEL;
			default: return Renderer::LineJoin::ROUND;
		}
	}

	Renderer::LineCap Renderer::get_line_cap(){
		switch(cairo_get_line_cap(INST_DATA->image.get())){
			case CAIRO_LINE_CAP_BUTT: return Renderer::LineCap::FLAT;
			case CAIRO_LINE_CAP_ROUND: return Renderer::LineCap::ROUND;
			case CAIRO_LINE_CAP_SQUARE: return Renderer::LineCap::SQUARE;
			default: return Renderer::LineCap::ROUND;
		}
	}

	double Renderer::get_line_dash_offset(){
		double offset;
		cairo_get_dash(INST_DATA->image.get(), NULL, &offset);
		return offset;
	}

	std::vector<double> Renderer::get_line_dash(){
		std::vector<double> dashes(cairo_get_dash_count(INST_DATA->image.get()));
		cairo_get_dash(INST_DATA->image.get(), dashes.data(), NULL);
		return dashes;
	}

	unsigned Renderer::get_antialiasing(){
		return cairo_get_antialias(INST_DATA->image.get()) == CAIRO_ANTIALIAS_BEST ? 8 : 0;
	}

	void Renderer::clear_stencil(){
		cairo_t* stencil_context = INST_DATA->stencil.get();
		cairo_set_operator(stencil_context, CAIRO_OPERATOR_SOURCE),
		cairo_set_source_rgba(stencil_context, 0, 0, 0, 0),
		cairo_paint(stencil_context);
	}
}
