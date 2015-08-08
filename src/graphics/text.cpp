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
#include "config.h"
#ifdef _WIN32
	#include <Wingdi.h>
	#include <Stringapiset.h>

	static inline std::wstring utf8_to_utf16(std::string s){
		std::wstring ws(MultiByteToWideChar(CP_UTF8, 0x0, s.data(), s.size(), NULL, 0), L'\0');
		MultiByteToWideChar(CP_UTF8, 0x0, s.data(), s.size(), const_cast<wchar_t*>(ws.data()), ws.length());
		return ws;
	}
	static inline std::string utf16_to_utf8(std::wstring ws){
		std::string s(WideCharToMultiByte(CP_UTF8, 0x0, ws.data(), ws.size(), NULL, 0, NULL, NULL), '\0');
		WideCharToMultiByte(CP_UTF8, 0x0, ws.data(), ws.size(), const_cast<char*>(s.data()), s.length(), NULL, NULL);
		return s;
	}
#else
	#include <pango/pangocairo.h>
#endif

namespace GUtils{
#ifdef _WIN32
	Font::Font() : dc(NULL), font(NULL), old_font(NULL){}
	Font::Font(std::string family, float size, bool bold, bool italic, bool underline, bool strikeout, bool rtl)
	: Font(utf8_to_utf16(family), size, bold, italic, underline, strikeout, rtl){}
	Font::Font(std::wstring family, float size, bool bold, bool italic, bool underline, bool strikeout, bool rtl){
		this->dc = CreateCompatibleDC(NULL),
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
		lf.lfFaceName[family.copy(lf.lfFaceName, LF_FACESIZE - 1)] = L'\0',
		this->font = CreateFontIndirectW(&lf),
		this->old_font = SelectObject(this->dc, this->font);
	}
	Font::~Font(){
		if(this->dc)
			SelectObject(this->dc, this->old_font),
			DeleteObject(this->font),
			DeleteDC(this->dc);
	}
	Font::Font(const Font& other){
		if(!other.dc)
			this->dc = NULL,
			this->font = NULL,
			this->old_font = NULL;
		else{
			this->dc = CreateCompatibleDC(NULL),
			SetMapMode(this->dc, MM_TEXT),
			SetBkMode(this->dc, TRANSPARENT),
			SetTextAlign(this->dc, GetTextAlign(other.dc));
			LOGFONTW lf;	// I trust in Spongebob that it has 4-byte boundary like required by GetObject
			GetObjectW(other.font, sizeof(lf), &lf),
			this->font = CreateFontIndirectW(&lf),
			this->old_font = SelectObject(this->dc, this->font);
		}
	}
	Font& Font::operator=(const Font& other){
		this->~Font();
		if(!other.dc)
			this->dc = NULL,
			this->font = NULL,
			this->old_font = NULL;
		else{
			this->dc = CreateCompatibleDC(NULL),
			SetMapMode(this->dc, MM_TEXT),
			SetBkMode(this->dc, TRANSPARENT),
			SetTextAlign(this->dc, GetTextAlign(other.dc));
			LOGFONTW lf;
			GetObjectW(other.font, sizeof(lf), &lf),
			this->font = CreateFontIndirectW(&lf),
			this->old_font = SelectObject(this->dc, this->font);
		}
		return *this;
	}
	Font::Font(Font&& other){
		if(!other.dc)
			this->dc = NULL,
			this->font = NULL,
			this->old_font = NULL;
		else
			this->dc = other.dc,
			this->font = other.font,
			this->old_font = other.old_font,
			other.dc = NULL,
			other.font = NULL,
			other.old_font = NULL;
	}
	Font& Font::operator=(Font&& other){
		this->~Font();
		if(!other.dc)
			this->dc = NULL,
			this->font = NULL,
			this->old_font = NULL;
		else
			this->dc = other.dc,
			this->font = other.font,
			this->old_font = other.old_font,
			other.dc = NULL,
			other.font = NULL,
			other.old_font = NULL;
		return *this;
	}
	std::string Font::get_family(){
		return utf16_to_utf8(this->get_family_unicode());
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
	bool Font::get_rtl(){
		return GetTextAlign(this->dc) == TA_RTLREADING;
	}
	Font::operator bool() const{
		return this->dc;
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
	double Font::text_width(std::string text){
		return this->text_width(utf8_to_utf16(text));
	}
	double Font::text_width(std::wstring text){
		SIZE sz;
		GetTextExtentPoint32W(this->dc, text.data(), text.length(), &sz);
		return static_cast<double>(sz.cx) / FONT_UPSCALE;
	}
#else
	Font::Font() : surface(nullptr), context(nullptr), layout(nullptr){}
	Font::Font(std::string family, float size, bool bold, bool italic, bool underline, bool strikeout, bool rtl){
		this->layout = pango_cairo_create_layout(this->context = cairo_create(this->surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1)));
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
	Font::Font(const Font& other){
		if(!other.surface)
			this->surface = this->context = this->layout = nullptr;
		else
			this->layout = pango_cairo_create_layout(this->context = cairo_create(this->surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1))),
			pango_layout_set_font_description(this->layout, pango_layout_get_font_description(other.layout)),
			pango_layout_set_attributes(this->layout, pango_layout_get_attributes(other.layout)),
			pango_layout_set_auto_dir(this->layout, pango_layout_get_auto_dir(other.layout));
	}
	Font& Font::operator=(const Font& other){
		this->~Font();
		if(!other.surface)
			this->surface = this->context = this->layout = nullptr;
		else
			this->layout = pango_cairo_create_layout(this->context = cairo_create(this->surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1))),
			pango_layout_set_font_description(this->layout, pango_layout_get_font_description(other.layout)),
			pango_layout_set_attributes(this->layout, pango_layout_get_attributes(other.layout)),
			pango_layout_set_auto_dir(this->layout, pango_layout_get_auto_dir(other.layout));
		return *this;
	}
	}
	Font::Font(Font&& other){
		if(!other.surface)
			this->surface = this->context = this->layout = nullptr;
		else
			this->surface = other.surface,
			this->context = other.context,
			this->layout = other.layout,
			other.surface = other.context = other.layout = nullptr;
	}
	Font& Font::operator=(Font&& other){
		this->~Font();
		if(!other.surface)
			this->surface = this->context = this->layout = nullptr;
		else
			this->surface = other.surface,
			this->context = other.context,
			this->layout = other.layout,
			other.surface = other.context = other.layout = nullptr;
		return *this;
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
		PangoAttribute* attr = pango_attr_underline_new(PANGO_UNDERLINE_SINGLE);
		bool result = pango_attribute_equal(pango_attr_iterator_get(attr_list_iter, PANGO_ATTR_UNDERLINE), attr);
		pango_attribute_destroy(attr),
		pango_attr_iterator_destroy(attr_list_iter);
		return result;
	}
	bool Font::get_strikeout(){
		PangoAttrIterator* attr_list_iter = pango_attr_list_get_iterator(pango_layout_get_attributes(this->layout));
		PangoAttribute* attr = pango_attr_strikethrough_new(true);
		bool result = pango_attribute_equal(pango_attr_iterator_get(attr_list_iter, PANGO_ATTR_STRIKETHROUGH), attr);
		pango_attribute_destroy(attr),
		pango_attr_iterator_destroy(attr_list_iter);
		return result;
	}
	bool Font::get_rtl(){
		return pango_layout_get_auto_dir(this->layout);
	}
	Font::operator bool() const{
		return this->surface;
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
	double Font::text_width(std::string text){
		pango_layout_set_text(this->layout, text.data(), text.length());
		PangoRectangle rect;
		pango_layout_get_pixel_extents(this->layout, NULL, &rect);
		return static_cast<double>(rect.width) / FONT_UPSCALE;
	}
#endif
}
