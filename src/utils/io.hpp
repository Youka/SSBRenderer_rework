/*
Project: SSBRenderer
File: io.hpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <fstream>
#ifdef __MINGW32__
	#include <memory>
	#include <ext/stdio_filebuf.h>
#endif
#include "utf8.hpp"
#ifndef _WIN32
	#include <limits.h> // PATH_MAX
	#include <libgen.h> // dirname
#endif

namespace stdex{
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
				const std::wstring& wfilename = Utf8::to_utf16(filename);
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

	// Extracts absolute directory path from filepath
	std::string get_file_dir(const std::string& filename){
#ifdef _WIN32
		wchar_t path[_MAX_PATH];
		if(_wfullpath(path, Utf8::to_utf16(filename).c_str(), sizeof(path)/sizeof(path[0]))){
			wchar_t drive[_MAX_DRIVE], dir[_MAX_DIR];
			_wsplitpath(path, drive, dir, NULL, NULL);
			return Utf8::from_utf16(std::wstring(drive) + dir);
		}
#else
		char path[PATH_MAX], *dir;
		if(realpath(filename.c_str(), path) && (dir = dirname(path)))
                        return dir;
#endif
		return "";
	}
}
