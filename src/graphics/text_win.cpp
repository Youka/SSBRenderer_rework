/*
Project: SSBRenderer
File: text_win.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "gutils.hpp"
#include <config.h>
#include <Wingdi.h>
#include "../utils/utf8.hpp"


namespace GUtils{
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
	double Font::text_width(const std::string& text){
		return this->text_width(Utf8::to_utf16(text));
	}
	double Font::text_width(const std::wstring& text){
		SIZE sz;
		GetTextExtentPoint32W(this->dc, text.data(), text.length(), &sz);
		return static_cast<double>(sz.cx) / FONT_UPSCALE + text.length() * this->spacing;
	}
	std::vector<PathSegment> Font::text_path(const std::string& text) throw(FontException){
		return this->text_path(Utf8::to_utf16(text));
	}
	std::vector<PathSegment> Font::text_path(const std::wstring& text) throw(FontException){
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
		// Collect path points
		std::vector<PathSegment> path;
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
						path.push_back({PathSegment::Type::MOVE, static_cast<double>(points[point_i].x) / FONT_UPSCALE, static_cast<double>(points[point_i].y) / FONT_UPSCALE});
						break;
					case PT_LINETO:
					case PT_LINETO|PT_CLOSEFIGURE:
						path.push_back({PathSegment::Type::LINE, static_cast<double>(points[point_i].x) / FONT_UPSCALE, static_cast<double>(points[point_i].y) / FONT_UPSCALE});
						break;
					case PT_BEZIERTO:
					case PT_BEZIERTO|PT_CLOSEFIGURE:
						path.push_back({PathSegment::Type::CURVE, static_cast<double>(points[point_i].x) / FONT_UPSCALE, static_cast<double>(points[point_i].y) / FONT_UPSCALE});
						break;
				}
				if(types[point_i]&PT_CLOSEFIGURE)
					path.push_back({PathSegment::Type::CLOSE});
			}
		}
		// Clear context from path...
		AbortPath(this->dc);
		// ...and return collected points
		return path;
	}
}
