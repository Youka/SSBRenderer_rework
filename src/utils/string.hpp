/*
Project: SSBRenderer
File: string.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <sstream>
#include <vector>

namespace stdex{
	// Converts string to number
	template<typename T>
	inline bool string_to_number(const std::string& src, T& dst){
		std::istringstream s(src);
		return s >> std::noskipws >> dst && s.eof();
	}

	// Converts string to number pair
	template<typename T>
	inline bool string_to_number(const std::string& src, T& dst1, T& dst2){
		std::string::size_type pos;
		return (pos = src.find(',')) != std::string::npos &&
				string_to_number(src.substr(0, pos), dst1) &&
				string_to_number(src.substr(pos+1), dst2);
	}

	// Converts hex string to number
	template<typename T>
	inline bool hex_string_to_number(const std::string& src, T& dst){
		std::istringstream s(src);
		return s >> std::noskipws >> std::hex >> dst && s.eof();
	}
	// Converts hex string to number pair
	template<typename T>
	inline bool hex_string_to_number(const std::string& src, T& dst1, T& dst2){
		std::string::size_type pos;
		return (pos = src.find(',')) != std::string::npos &&
				hex_string_to_number(src.substr(0, pos), dst1) &&
				hex_string_to_number(src.substr(pos+1), dst2);
	}
	// Converts hex string to four numbers
	template<typename T>
	inline bool hex_string_to_number(const std::string& src, T& dst1, T& dst2, T& dst3, T& dst4){
		std::string::size_type pos1, pos2;
		return (pos1 = src.find(',')) != std::string::npos &&
				hex_string_to_number(src.substr(0, pos1), dst1) &&
				(pos2 = src.find(',', pos1+1)) != std::string::npos &&
				hex_string_to_number(src.substr(pos1+1, pos2-(pos1+1)), dst2) &&
				(pos1 = src.find(',', pos2+1)) != std::string::npos &&
				hex_string_to_number(src.substr(pos2+1, pos1-(pos2+1)), dst3) &&
				hex_string_to_number(src.substr(pos1+1), dst4);
	}

	// Find character in string which isn't escaped by character '\'
	inline std::string::size_type find_non_escaped_character(const std::string& s, const char c, const std::string::size_type pos_start = 0){
		std::string::size_type pos_end;
		for(auto search_pos_start = pos_start;
			(pos_end = s.find(c, search_pos_start)) != std::string::npos && pos_end > 0 && s[pos_end-1] == '\\';
			search_pos_start = pos_end + 1);
		return pos_end;
	}

	// Replaces subtext
	inline std::string& string_replace(std::string& s, std::string find, std::string repl){
		for(std::string::size_type pos = 0; (pos = s.find(find, pos)) != std::string::npos; pos+=repl.length())
			s.replace(pos, find.length(), repl);
		return s;
	}

	// Splits text to words
	struct WordData{
		size_t prespace;
		std::string text;
	};
	inline std::vector<WordData> get_words(const std::string& s){
		std::vector<WordData> words;
		if(!s.empty()){
			std::string::size_type pos_begin = 0, pos_end;
			bool search_prespace = true;
			do{
				if(search_prespace)
					pos_end = s.find_first_not_of(" \t\n\r", pos_begin),
					words.push_back({pos_end == std::string::npos ? s.length() - pos_begin : pos_end - pos_begin, ""});
				else
					pos_end = s.find_first_of(" \t\n\r", pos_end),
					words.back().text = s.substr(pos_begin, pos_end == std::string::npos ? std::string::npos : pos_end - pos_begin);
				search_prespace = !search_prespace,
				pos_begin = pos_end;
			}while(pos_end != std::string::npos);
		}
		return words;
	}

	// Checks stream for the last line after delimiter being empty
	inline bool has_empty_last_line(std::istream& str, const char delimiter){
		if(!str.bad()){
			if(str.eof()){
				str.clear();
				if(str.unget() && str.get() == delimiter)
					return true;
			}else{
				auto pos = str.tellg();
				str.seekg(0, std::istream::end);
				bool result = str.unget() && str.get() == delimiter;
				str.clear(),
				str.seekg(pos);
				return result;
			}
		}
		return false;
	}
}

#define STR_LIT_EQU_FIRST(s, s2) (s.compare(0, sizeof(s2)-1, s2) == 0)
