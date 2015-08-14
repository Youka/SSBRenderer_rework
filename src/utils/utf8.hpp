/*
Project: SSBRenderer
File: utf8.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <string>
#include <vector>
#ifdef _WIN32
	#include <windef.h>
	#include <Stringapiset.h>
#endif
#include <fstream>
#ifdef __MINGW32__
	#include <memory>
	#include <ext/stdio_filebuf.h>
#endif

namespace Utf8{
	// Byte length of utf-8 character at given position in string
	static inline size_t clen(const std::string& s, const size_t pos){
		const unsigned char c = s[pos];
		if(c & 0x3<<6){
			if(c & 0x1<<5){
				if(c & 0x1<<4){
					if(c & 0x1<<3){
						if(c & 0x1<<2){
							if(c & 0x1<<1){
								if(c & 0x1)
									return 8;
								return 7;
							}
							return 6;
						}
						return 5;
					}
					return 4;
				}
				return 3;
			}
			return 2;
		}
		return 1;
	}

	// Number of utf-8 characters in string
	static inline size_t slen(const std::string& s){
		size_t n = 0, i;
		for(i = 0; i < s.length(); i += clen(s, i))
			n++;
		if(i != s.length()) n--;	// Last character wasn't complete
		return n;
	}

	// Collection of utf-8 characters in string
	static inline std::vector<std::string> chars(const std::string& s){
		std::vector<std::string> tokens;
		tokens.reserve(slen(s));
		for(size_t i = 0, len; i < s.length(); i += len){
			len = clen(s, i);
			if(i+len <= s.length())
				tokens.push_back(s.substr(i, len));
		}
		return tokens;
	}
#ifdef _WIN32
	// Utf-8 to utf-16 string conversion (native windows)
	static inline std::wstring to_utf16(const std::string& s){
		std::wstring ws(MultiByteToWideChar(CP_UTF8, 0x0, s.data(), s.size(), NULL, 0), L'\0');
		MultiByteToWideChar(CP_UTF8, 0x0, s.data(), s.size(), const_cast<wchar_t*>(ws.data()), ws.length());
		return ws;
	}

	// Utf-16 to utf-8 string conversion (native windows)
	static inline std::string from_utf16(const std::wstring& ws){
		std::string s(WideCharToMultiByte(CP_UTF8, 0x0, ws.data(), ws.size(), NULL, 0, NULL, NULL), '\0');
		WideCharToMultiByte(CP_UTF8, 0x0, ws.data(), ws.size(), const_cast<char*>(s.data()), s.length(), NULL, NULL);
		return s;
	}
#endif

	// Cross-platform fstream which accepts utf-8 filename
	class fstream : public std::
#if defined(_WIN32) && !defined(__MINGW32__)
	wfstream
#else
	fstream
#endif
	{
#ifdef __MINGW32__
		private:
			std::unique_ptr<FILE,std::function<void(FILE*)>> file = decltype(file)(nullptr, [](FILE* file){fclose(file);});
#endif
		public:
#ifdef _WIN32
			fstream() = default;
			explicit fstream(const std::string& filename, ios_base::openmode mode = ios_base::in|ios_base::out){this->open(filename, mode);}
			void open(const char *filename, ios_base::openmode mode = ios_base::in|ios_base::out){this->open(std::string(filename), mode);}
			void open(const std::string& filename, ios_base::openmode mode = ios_base::in|ios_base::out){
				const std::wstring& wfilename = to_utf16(filename);
#ifdef __MINGW32__
				std::wstring wmode;
				wmode.reserve(3);
				if(mode & ios_base::app){
					wmode = L'a';
					if(mode & ios_base::in)
						wmode.push_back(L'+');
				}else if(mode & ios_base::in && mode & ios_base::out)
					wmode = mode & ios_base::trunc ? L"w+" : L"r+";
				else if(mode & ios_base::in)
					wmode = L'r';
				else if(mode & ios_base::out)
					wmode = L'w';
				if(mode & ios_base::binary)
					wmode.push_back(L'b');
				FILE* file = _wfopen(wfilename.c_str(), wmode.c_str());
				if(file){
					this->file.reset(file);
					delete std::ios::rdbuf(new __gnu_cxx::stdio_filebuf<char>(file, mode));
					if(mode & ios_base::ate)
						this->seekg(0, std::ios_base::end);
				}else
					this->setstate(std::ios_base::failbit);
#else
				std::wfstream::open(wfilename, mode);
#endif
			}
#else
			using std::fstream::fstream;
#endif
	};





}
