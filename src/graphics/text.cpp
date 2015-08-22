/*
Project: SSBRenderer
File: text.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "gutils.hpp"
#include <config.h>
#ifdef _WIN32
	#include <Wingdi.h>
	#include <Usp10.h>
	#include "../utils/utf8.hpp"
#else
	#include <pango/pangocairo.h>
	#include <memory>
#endif

namespace GUtils{
#ifdef _WIN32
	Font::Font() : dc(NULL), font(NULL), old_font(NULL), spacing(0){}
	Font::Font(const std::string& family, float size, bool bold, bool italic, bool underline, bool strikeout, double spacing, bool rtl) throw(FontException)
	: Font(Utf8::to_utf16(family), size, bold, italic, underline, strikeout, spacing, rtl){}
	Font::Font(const std::wstring& family, float size, bool bold, bool italic, bool underline, bool strikeout, double spacing, bool rtl) throw(FontException) : spacing(spacing){
		if(family.length() > 31)	// See LOGFONT limitation
			throw FontException("Family length exceeds 31");
		if(size < 0)
			throw FontException("Size <0");
		if(!(this->dc = CreateCompatibleDC(NULL)))
			throw FontException("Couldn't create context");
		SetMapMode(this->dc, MM_TEXT),
		SetBkMode(this->dc, TRANSPARENT);
		if(rtl)
			SetTextAlign(this->dc, TA_RTLREADING);
		LOGFONTW lf = {0};
		lf.lfHeight = size * FONT_UPSCALE,
		lf.lfWeight = bold ? FW_BOLD : FW_NORMAL,
		lf.lfItalic = italic,
		lf.lfUnderline = underline,
		lf.lfStrikeOut = strikeout,
		lf.lfCharSet = DEFAULT_CHARSET,
		lf.lfOutPrecision = OUT_TT_PRECIS,
		lf.lfQuality = ANTIALIASED_QUALITY,
		lf.lfFaceName[family.copy(lf.lfFaceName, LF_FACESIZE - 1)] = L'\0';
		if(!(this->font = CreateFontIndirectW(&lf))){
			DeleteDC(this->dc);
			throw FontException("Couldn't create font");
		}
		this->old_font = SelectObject(this->dc, this->font);
	}
	Font::~Font(){
		if(this->dc)
			SelectObject(this->dc, this->old_font),
			DeleteObject(this->font),
			DeleteDC(this->dc);
	}
	void Font::copy(const Font& other){
		if(!other.dc)
			this->dc = NULL,
			this->font = NULL,
			this->old_font = NULL,
			this->spacing = 0;
		else{
			this->dc = CreateCompatibleDC(NULL),
			SetMapMode(this->dc, MM_TEXT),
			SetBkMode(this->dc, TRANSPARENT),
			SetTextAlign(this->dc, GetTextAlign(other.dc));
			LOGFONTW lf;	// I trust in Spongebob that it has 4-byte boundary like required by GetObject
			GetObjectW(other.font, sizeof(lf), &lf),
			this->font = CreateFontIndirectW(&lf),
			this->old_font = SelectObject(this->dc, this->font),
			this->spacing = other.spacing;
		}
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
		if(!other.dc)
			this->dc = NULL,
			this->font = NULL,
			this->old_font = NULL,
			this->spacing = 0;
		else
			this->dc = other.dc,
			this->font = other.font,
			this->old_font = other.old_font,
			this->spacing = other.spacing,
			other.dc = NULL,
			other.font = NULL,
			other.old_font = NULL,
			other.spacing = 0;
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
		return this->dc;
	}
	std::string Font::get_family(){
		return Utf8::from_utf16(this->get_family_unicode());
	}
	std::wstring Font::get_family_unicode(){
		LOGFONTW lf;
		GetObjectW(this->font, sizeof(lf), &lf);
		return lf.lfFaceName;
	}
	float Font::get_size(){
		LOGFONTW lf;
		GetObjectW(this->font, sizeof(lf), &lf);
		return static_cast<float>(lf.lfHeight) / FONT_UPSCALE;
	}
	bool Font::get_bold(){
		LOGFONTW lf;
		GetObjectW(this->font, sizeof(lf), &lf);
		return lf.lfWeight == FW_BOLD;
	}
	bool Font::get_italic(){
		LOGFONTW lf;
		GetObjectW(this->font, sizeof(lf), &lf);
		return lf.lfItalic;
	}
	bool Font::get_underline(){
		LOGFONTW lf;
		GetObjectW(this->font, sizeof(lf), &lf);
		return lf.lfUnderline;
	}
	bool Font::get_strikeout(){
		LOGFONTW lf;
		GetObjectW(this->font, sizeof(lf), &lf);
		return lf.lfStrikeOut;
	}
	double Font::get_spacing(){
		return this->spacing;
	}
	bool Font::get_rtl(){
		return GetTextAlign(this->dc) == TA_RTLREADING;
	}
	Font::Metrics Font::metrics(){
		TEXTMETRICW metrics;
		GetTextMetricsW(this->dc, &metrics);
		return {
			static_cast<double>(metrics.tmHeight) / FONT_UPSCALE,
			static_cast<double>(metrics.tmAscent) / FONT_UPSCALE,
			static_cast<double>(metrics.tmDescent) / FONT_UPSCALE,
			static_cast<double>(metrics.tmInternalLeading) / FONT_UPSCALE,
			static_cast<double>(metrics.tmExternalLeading) / FONT_UPSCALE
		};
	}
	std::vector<GlyphRun> Font::text_glyphs(const std::string& text){
		return this->text_glyphs(Utf8::to_utf16(text));
	}
	std::vector<GlyphRun> Font::text_glyphs(const std::wstring& text){

		// TODO

	}
	double Font::text_width(const std::vector<Glyph_t>& glyphs){
		SIZE sz;
		GetTextExtentPointI(this->dc, const_cast<Glyph_t*>(glyphs.data()), glyphs.size(), &sz);
		return static_cast<double>(sz.cx) / FONT_UPSCALE + glyphs.size() * this->spacing;
	}
	double Font::text_width(const std::string& text){
		return this->text_width(Utf8::to_utf16(text));
	}
	double Font::text_width(const std::wstring& text){
		SIZE sz;
		GetTextExtentPoint32W(this->dc, text.data(), text.length(), &sz);
		return static_cast<double>(sz.cx) / FONT_UPSCALE + text.length() * this->spacing;
	}
	inline std::vector<Font::PathSegment> Font::extract_path()  throw(FontException){
		// Collect path points
		std::vector<Font::PathSegment> path;
		const int points_n = GetPath(this->dc, NULL, NULL, 0);
		if(points_n){
			std::vector<POINT> points;
			std::vector<BYTE> types;
			points.reserve(points_n),
			types.reserve(points_n),
			path.reserve(points_n),
			GetPath(this->dc, points.data(), types.data(), points_n);
			// Pack points in output
			for(int point_i = 0; point_i < points_n; ++point_i){
				switch(types[point_i]){
					case PT_MOVETO:
						path.push_back({Font::PathSegment::Type::MOVE, static_cast<double>(points[point_i].x) / FONT_UPSCALE, static_cast<double>(points[point_i].y) / FONT_UPSCALE});
						break;
					case PT_LINETO:
					case PT_LINETO|PT_CLOSEFIGURE:
						path.push_back({Font::PathSegment::Type::LINE, static_cast<double>(points[point_i].x) / FONT_UPSCALE, static_cast<double>(points[point_i].y) / FONT_UPSCALE});
						break;
					case PT_BEZIERTO:
					case PT_BEZIERTO|PT_CLOSEFIGURE:
						path.push_back({Font::PathSegment::Type::CURVE, static_cast<double>(points[point_i].x) / FONT_UPSCALE, static_cast<double>(points[point_i].y) / FONT_UPSCALE});
						break;
				}
				if(types[point_i]&PT_CLOSEFIGURE)
					path.push_back({Font::PathSegment::Type::CLOSE});
			}
		}
		// Clear context from path...
		AbortPath(this->dc);
		// ...and return collected points
		return path;
	}
	std::vector<Font::PathSegment> Font::text_path(const std::vector<Glyph_t>& glyphs) throw(FontException){
		// Check valid glyphs number
		if(glyphs.size() > 8192)	// See ExtTextOut limitation
			throw FontException("Glyphs number exceeds 8192");
		// Get glyphs width
		std::vector<int> distance_x;
		if(this->spacing != 0){
			distance_x.resize(glyphs.size()),
			GetCharWidthI(this->dc, 0, glyphs.size(), const_cast<LPWORD>(glyphs.data()), distance_x.data());
			const int spacing_upscaled = this->spacing * FONT_UPSCALE;
			for(auto& x : distance_x)
				x += spacing_upscaled;
		}
		// Add glyphs path to context
		BeginPath(this->dc);
		ExtTextOutW(this->dc, 0, 0, ETO_GLYPH_INDEX, NULL, reinterpret_cast<LPCWSTR>(glyphs.data()), glyphs.size(), distance_x.empty() ? NULL : distance_x.data());
		EndPath(this->dc);
		// Extract & return path
		return this->extract_path();
	}
	std::vector<Font::PathSegment> Font::text_path(const std::string& text) throw(FontException){
		return this->text_path(Utf8::to_utf16(text));
	}
	std::vector<Font::PathSegment> Font::text_path(const std::wstring& text) throw(FontException){
		// Check valid text length
		if(text.length() > 8192)	// See ExtTextOut limitation
			throw FontException("Text length exceeds 8192");
		// Get characters width
		std::vector<int> distance_x;
		if(this->spacing != 0){
			distance_x.reserve(text.length());
			INT width;
			const int spacing_upscaled = this->spacing * FONT_UPSCALE;
			for(auto c : text)
				GetCharWidth32W(this->dc, c, c, &width),
				distance_x.push_back(width + spacing_upscaled);
		}
		// Add text path to context
		BeginPath(this->dc);
		ExtTextOutW(this->dc, 0, 0, 0x0, NULL, text.data(), text.length(), distance_x.empty() ? NULL : distance_x.data());
		EndPath(this->dc);
		// Extract & return path
		return this->extract_path();
	}
#else
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

		// TODO

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
	inline std::vector<Font::PathSegment> Font::extract_path() throw(FontException){
		// Get path points
		std::unique_ptr<cairo_path_t, std::function<void(cairo_path_t*)>> cpath(cairo_copy_path(this->context), [](cairo_path_t* path){cairo_path_destroy(path);});
		if(cpath->status != CAIRO_STATUS_SUCCESS){
			cairo_new_path(this->context);
			throw FontException("Couldn't get valid path");
		}
		// Pack points in output
		std::vector<Font::PathSegment> path;
		for(cairo_path_data_t* pdata = cpath->data, *data_end = pdata + cpath->num_data; pdata != data_end; pdata += pdata->header.length)
			switch(pdata->header.type){
				case CAIRO_PATH_MOVE_TO:
					path.push_back({Font::PathSegment::Type::MOVE, pdata[1].point.x, pdata[1].point.y});
					break;
				case CAIRO_PATH_LINE_TO:
					path.push_back({Font::PathSegment::Type::LINE, pdata[1].point.x, pdata[1].point.y});
					break;
				case CAIRO_PATH_CURVE_TO:
					path.push_back({Font::PathSegment::Type::CURVE, pdata[1].point.x, pdata[1].point.y});
					path.push_back({Font::PathSegment::Type::CURVE, pdata[2].point.x, pdata[2].point.y});
					path.push_back({Font::PathSegment::Type::CURVE, pdata[3].point.x, pdata[3].point.y});
					break;
				case CAIRO_PATH_CLOSE_PATH:
					path.push_back({Font::PathSegment::Type::CLOSE});
					break;
			}
		// Clear context from path...
		cairo_new_path(this->context);
		// ...and return collected points
		return path;
	}
	std::vector<Font::PathSegment> Font::text_path(const std::vector<Glyph_t>& glyphs) throw(FontException){
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
	std::vector<Font::PathSegment> Font::text_path(const std::string& text) throw(FontException){
		// Add text path to context
		pango_layout_set_text(this->layout, text.data(), text.length()),
		cairo_save(this->context),
		cairo_scale(ctx, 1.0 / FONT_UPSCALE, 1.0 / FONT_UPSCALE),
		pango_cairo_layout_path(this->context, this->layout),
		cairo_restore(this->context);
		// Extract & return path
		return this->extract_path();
	}
#endif
}
