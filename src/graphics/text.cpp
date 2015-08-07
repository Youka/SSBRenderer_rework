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
		std::wstring ws(MultiByteToWideChar(CP_UTF8, 0x0, s.data(), s.size(), nullptr, 0), L'\0');
		MultiByteToWideChar(CP_UTF8, 0x0, s.data(), s.size(), const_cast<wchar_t*>(ws.data()), ws.length());
		return ws;
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
#else
	Font::Font() : surface(nullptr), context(nullptr), layout(nullptr){}
	Font::Font(std::string family, float size, bool bold, bool italic, bool underline, bool strikeout, bool rtl){
		this->layout = pango_cairo_create_layout(this->context = cairo_create(this->surface = cairo_image_surface_create(CAIRO_FORMAT_A1, 1, 1)));
		PangoFontDescription *font = pango_font_description_new();
		pango_font_description_set_family(font, family.c_str()),
		pango_font_description_set_weight(font, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL),
		pango_font_description_set_style(font, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL),
		pango_font_description_set_absolute_size(font, size * PANGO_SCALE * UPSCALE),
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
#endif
}
