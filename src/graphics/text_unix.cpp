/*
Project: SSBRenderer
File: text_unix.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "gutils.hpp"
#include <config.h>
#include <pango/pangocairo.h>
#include <memory>

namespace GUtils{
	Font::Font() : surface(nullptr), context(nullptr), layout(nullptr){}
	Font::Font(const std::string& family, float size, bool bold, bool italic, bool underline, bool strikeout, double spacing, bool rtl) throw(FontException){
		if(size < 0)
			throw FontException("Size <0");
		if(!(this->surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1)))
			throw FontException("Couldn't create surface");
		if(!(this->context = cairo_create(this->surface))){
			cairo_surface_destroy(this->surface);
			throw FontException("Couldn't create context");
		}
		if(!(this->layout = pango_cairo_create_layout(this->context))){
			cairo_destroy(this->context),
			cairo_surface_destroy(this->surface);
			throw FontException("Couldn't create layout");
		}
		PangoFontDescription *font = pango_font_description_new();
		pango_font_description_set_family(font, family.c_str()),
		pango_font_description_set_weight(font, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL),
		pango_font_description_set_style(font, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL),
		pango_font_description_set_absolute_size(font, size * PANGO_SCALE * FONT_UPSCALE),
		pango_layout_set_font_description(this->layout, font),
		pango_font_description_free(font);
		PangoAttrList* attr_list = pango_attr_list_new();
		pango_attr_list_insert(attr_list, pango_attr_underline_new(underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE)),
		pango_attr_list_insert(attr_list, pango_attr_strikethrough_new(strikeout)),
		pango_attr_list_insert(attr_list, pango_attr_letter_spacing_new(spacing * PANGO_SCALE * FONT_UPSCALE)),
		pango_layout_set_attributes(this->layout, attr_list),
		pango_attr_list_unref(attr_list),
		pango_layout_set_auto_dir(this->layout, rtl);
	}
	Font::~Font(){
		if(this->surface)
			g_object_unref(this->layout),
			cairo_destroy(this->context),
			cairo_surface_destroy(this->surface);
	}
	void Font::copy(const Font& other){
		if(!other.surface)
			this->surface = this->context = this->layout = nullptr;
		else
			this->layout = pango_cairo_create_layout(this->context = cairo_create(this->surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1))),
			pango_layout_set_font_description(this->layout, pango_layout_get_font_description(other.layout)),
			pango_layout_set_attributes(this->layout, pango_layout_get_attributes(other.layout)),
			pango_layout_set_auto_dir(this->layout, pango_layout_get_auto_dir(other.layout));
	}
	Font::Font(const Font& other){
		this->copy(other);
	}
	Font& Font::operator=(const Font& other){
		this->~Font(),
		this->copy(other);
		return *this;
	}
	void Font::move(Font&& other){
		if(!other.surface)
			this->surface = this->context = this->layout = nullptr;
		else
			this->surface = other.surface,
			this->context = other.context,
			this->layout = other.layout,
			other.surface = other.context = other.layout = nullptr;
	}
	Font::Font(Font&& other){
		this->move(std::forward<Font>(other));
	}
	Font& Font::operator=(Font&& other){
		this->~Font(),
		this->move(std::forward<Font>(other));
		return *this;
	}
	Font::operator bool() const{
		return this->surface;
	}
	std::string Font::get_family(){
		return pango_font_description_get_family(pango_layout_get_font_description(this->layout));
	}
	float Font::get_size(){
		return static_cast<float>(pango_font_description_get_size(pango_layout_get_font_description(this->layout))) / FONT_UPSCALE / PANGO_SCALE;
	}
	bool Font::get_bold(){
		return pango_font_description_get_weight(pango_layout_get_font_description(this->layout)) == PANGO_WEIGHT_BOLD;
	}
	bool Font::get_italic(){
		return pango_font_description_get_style(pango_layout_get_font_description(this->layout)) == PANGO_STYLE_ITALIC;
	}
	bool Font::get_underline(){
		PangoAttrIterator* attr_list_iter = pango_attr_list_get_iterator(pango_layout_get_attributes(this->layout));
		bool result = reinterpret_cast<PangoAttrInt*>(pango_attr_iterator_get(attr_list_iter, PANGO_ATTR_UNDERLINE))->value == PANGO_UNDERLINE_SINGLE;
		pango_attr_iterator_destroy(attr_list_iter);
		return result;
	}
	bool Font::get_strikeout(){
		PangoAttrIterator* attr_list_iter = pango_attr_list_get_iterator(pango_layout_get_attributes(this->layout));
		bool result = reinterpret_cast<PangoAttrInt*>(pango_attr_iterator_get(attr_list_iter, PANGO_ATTR_STRIKETHROUGH))->value;
		pango_attr_iterator_destroy(attr_list_iter);
		return result;
	}
	double Font::get_spacing(){
		PangoAttrIterator* attr_list_iter = pango_attr_list_get_iterator(pango_layout_get_attributes(this->layout));
		double result = static_cast<double>(reinterpret_cast<PangoAttrInt*>(pango_attr_iterator_get(attr_list_iter, PANGO_ATTR_LETTER_SPACING))->value) / FONT_UPSCALE / PANGO_SCALE;
		pango_attr_iterator_destroy(attr_list_iter);
		return result;
	}
	bool Font::get_rtl(){
		return pango_layout_get_auto_dir(this->layout);
	}
	Font::Metrics Font::metrics(){
		FontMetrics result;
		PangoFontMetrics* metrics = pango_context_get_metrics(pango_layout_get_context(this->layout), pango_layout_get_font_description(this->layout), NULL);
		result.ascent = static_cast<double>(pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE / FONT_UPSCALE,
		result.descent = static_cast<double>(pango_font_metrics_get_descent(metrics)) / PANGO_SCALE / FONT_UPSCALE,
		result.height = result.ascent + result.descent,
		result.internal_lead = 0, // HEIGHT - ASCENT - DESCENT
		result.external_lead = static_cast<double>(pango_layout_get_spacing(this->layout)) / PANGO_SCALE / FONT_UPSCALE,
		pango_font_metrics_unref(metrics);
		return result;
	}
	std::vector<GlyphRun> Font::text_glyphs(const std::string& text){
		// Output storage
		std::vector<GlyphRun> runs;
		// Itemize text
		std::unique_ptr<GList, std::function<void(GList*)>> items(
			pango_itemize(
				pango_layout_get_context(this->layout),
				text.data(),
				0,
				text.length(),
				NULL,
				NULL
			),
			[](GList* l){
				g_list_foreach(l, [](gpointer data, gpointer){pango_item_free(reinterpret_cast<PangoItem*>(data));}, NULL);
				g_list_free(l);
			}
		);
		if(items){
			// Shape items
			std::unique_ptr<PangoGlyphString, std::function<void(PangoGlyphString*)>> glyphstring(
				pango_glyph_string_new(),
				[](PangoGlyphString* s){pango_glyph_string_free(s);}
			);
			for(GList* pitems = items.get(); pitems != NULL; pitems = pitems->next){
				PangoItem* item = reinterpret_cast<PangoItem*>(pitems->data);
				pango_shape(
					text.data() + item->offset,
					item->length,
					item->analysis,
					glyphstring.get()
				);
				// Save glyphs + direction
				std::vector<Glyph_t> glyphs(glyphstring->num_glyphs);
				for(unsigned glyph_i = 0; glyph_i < glyphs.size(); ++glyph_i)
					glyphs[glyph_i] = glyphstring->glyphs[glyph_i].glyph;
				runs.push_back({glyphs, glyphstring->num_glyphs > 1 && glyphstring->log_clusters[0] > glyphstring->log_clusters[1] ? GlyphDir::RTL : GlyphDir::LTR});
			}
		}
		// Return what we got
		return runs;
	}
	double Font::text_width(const std::vector<Glyph_t>& glyphs){
		std::unique_ptr<PangoFont, std::function<void(PangoFont*)>> font(
			pango_context_load_font(pango_layout_get_context(this->layout), pango_layout_get_font_description(this->layout)),
			[](PangoFont* p){g_object_unref(p);}
		);
		int width = 0;
		PangoRectangle rect;
		for(auto glyph : glyphs)
			pango_font_get_glyph_extents(font.get(), glyph, NULL, &rect),
			width += rect.width;
		return static_cast<double>(width) / PANGO_SCALE / FONT_UPSCALE + glyphs.size() * this->get_spacing();
	}
	double Font::text_width(const std::string& text){
		pango_layout_set_text(this->layout, text.data(), text.length());
		PangoRectangle rect;
		pango_layout_get_pixel_extents(this->layout, NULL, &rect);
		return static_cast<double>(rect.width) / FONT_UPSCALE;
	}
	inline std::vector<PathSegment> Font::extract_path() throw(FontException){
		// Get path points
		std::unique_ptr<cairo_path_t, std::function<void(cairo_path_t*)>> cpath(cairo_copy_path(this->context), [](cairo_path_t* path){cairo_path_destroy(path);});
		if(cpath->status != CAIRO_STATUS_SUCCESS){
			cairo_new_path(this->context);
			throw FontException("Couldn't get valid path");
		}
		// Pack points in output
		std::vector<PathSegment> path;
		for(cairo_path_data_t* pdata = cpath->data, *data_end = pdata + cpath->num_data; pdata != data_end; pdata += pdata->header.length)
			switch(pdata->header.type){
				case CAIRO_PATH_MOVE_TO:
					path.push_back({PathSegment::Type::MOVE, pdata[1].point.x, pdata[1].point.y});
					break;
				case CAIRO_PATH_LINE_TO:
					path.push_back({PathSegment::Type::LINE, pdata[1].point.x, pdata[1].point.y});
					break;
				case CAIRO_PATH_CURVE_TO:
					path.push_back({PathSegment::Type::CURVE, pdata[1].point.x, pdata[1].point.y});
					path.push_back({PathSegment::Type::CURVE, pdata[2].point.x, pdata[2].point.y});
					path.push_back({PathSegment::Type::CURVE, pdata[3].point.x, pdata[3].point.y});
					break;
				case CAIRO_PATH_CLOSE_PATH:
					path.push_back({PathSegment::Type::CLOSE});
					break;
			}
		// Clear context from path...
		cairo_new_path(this->context);
		// ...and return collected points
		return path;
	}
	std::vector<PathSegment> Font::text_path(const std::vector<Glyph_t>& glyphs) throw(FontException){
		// Get layout font
		std::unique_ptr<PangoFont, std::function<void(PangoFont*)>> font(
			pango_context_load_font(pango_layout_get_context(this->layout), pango_layout_get_font_description(this->layout)),
			[](PangoFont* p){g_object_unref(p);}
		);
		// Construct glyph string
		std::unique_ptr<PangoGlyphString, std::function<void(PangoGlyphString*)>> str(
			pango_glyph_string_new(),
			[](PangoGlyphString* p){pango_glyph_string_free(p);}
		);
		pango_glyph_string_set_size(str.get(), glyphs.size());
		PangoRectangle rect;
		for(unsigned i = 0; i < str->num_glyphs; ++i)
			pango_font_get_glyph_extents(font.get(), glyphs[i], NULL, &rect),
			str->glyphs[i].geometry.width = rect.width,
			str->glyphs[i].glyph = glyphs[i];

		// TODO: Check glyphs offset
		// TODO: Check attributes: underline, strikeout, letter spacing

		// Add glyphs path to context
		cairo_save(this->context),
		cairo_scale(ctx, 1.0 / FONT_UPSCALE, 1.0 / FONT_UPSCALE),
		pango_cairo_glyph_string_path(this->context, font.get(), str.get()),
		cairo_restore(this->context);
		// Extract & return path
		return this->extract_path();
	}
	std::vector<PathSegment> Font::text_path(const std::string& text) throw(FontException){
		// Add text path to context
		pango_layout_set_text(this->layout, text.data(), text.length()),
		cairo_save(this->context),
		cairo_scale(ctx, 1.0 / FONT_UPSCALE, 1.0 / FONT_UPSCALE),
		pango_cairo_layout_path(this->context, this->layout),
		cairo_restore(this->context);
		// Extract & return path
		return this->extract_path();
	}
}
