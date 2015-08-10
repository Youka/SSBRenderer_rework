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

#ifdef _WIN32
	#include <windef.h>
	#include <Stringapiset.h>
#endif

namespace Utf8{
#ifdef _WIN32
	static inline std::wstring to_utf16(std::string s){
		std::wstring ws(MultiByteToWideChar(CP_UTF8, 0x0, s.data(), s.size(), NULL, 0), L'\0');
		MultiByteToWideChar(CP_UTF8, 0x0, s.data(), s.size(), const_cast<wchar_t*>(ws.data()), ws.length());
		return ws;
	}
	static inline std::string from_utf16(std::wstring ws){
		std::string s(WideCharToMultiByte(CP_UTF8, 0x0, ws.data(), ws.size(), NULL, 0, NULL, NULL), '\0');
		WideCharToMultiByte(CP_UTF8, 0x0, ws.data(), ws.size(), const_cast<char*>(s.data()), s.length(), NULL, NULL);
		return s;
	}
#endif
}
