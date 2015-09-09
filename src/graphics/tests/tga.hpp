/*
Project: SSBRenderer
File: tga.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <fstream>
#include <cmath>
#include <cstdint>

static inline bool write_tga(const std::string filename, const int width, const int height, const unsigned stride, const bool has_alpha, const unsigned char* data){
	// Open & check target file
	std::ofstream file(filename, std::ios::binary);
	if(!file)
		return false;
	// Write TGA header
	const unsigned rwidth = ::abs(width), rheight = ::abs(height);
#pragma pack(push,1)
	struct{
		uint8_t id_length, has_palette, type;
		uint16_t palette_begin, palette_length;
		uint8_t palette_entry_size;
		uint16_t x, y, width, height;
		uint8_t depth, attribute_bits : 4, attribue_horizontal_rtl : 1, attribue_vertical__ttb : 1, attribute_reserved : 2;
	}header = {
		0,	// id_length
		0,	// has_palette
		2,	// type
		0,	// palette_begin
		0,	// palette_length
		0,	// palette_entry_size
		0,	// x
		0,	// y
		static_cast<uint16_t>(rwidth),	// width
		static_cast<uint16_t>(rheight),	// height
		static_cast<uint8_t>(has_alpha ? 32 : 24),	// depth
		0,	// attribute_bits
		static_cast<uint8_t>(width < 0 ? 1 : 0),	// attribue_horizontal_rtl
		static_cast<uint8_t>(height >= 0 ? 1 : 0),	// attribue_vertical_ttb
		0	// attribute_reserved
	};
#pragma pack(pop)
	file.write(reinterpret_cast<const char*>(&header), sizeof(header));
	// Write TGA image data
        const unsigned rowsize = has_alpha ? rwidth << 2 : rwidth * 3;
	for(const unsigned char* const data_end = data + rheight * stride; data != data_end; data += stride)
		file.write(reinterpret_cast<const char*>(data), rowsize);
	// Writing successful
	return true;
}
