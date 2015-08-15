/*
Project: SSBRenderer
File: FilterBase.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../plugins/FilterBase.hpp"
#include <config.h>

#ifdef _MSC_VER
#include <initguid.h>
// {2C80A3B9-6A38-4416-A6E1-8982A37FD3FA}
DEFINE_GUID(CLSID_Filter,
0x2c80a3b9, 0x6a38, 0x4416, 0xa6, 0xe1, 0x89, 0x82, 0xa3, 0x7f, 0xd3, 0xfa);

// {534FD455-C4F5-4719-8196-F315868E02A4}
DEFINE_GUID(IID_Config,
0x534fd455, 0xc4f5, 0x4719, 0x81, 0x96, 0xf3, 0x15, 0x86, 0x8e, 0x2, 0xa4);
#endif // _MSC_VER

namespace FilterBase{
	const char* get_id(){
		return "com.subtitle." PROJECT_NAME;
	}
	const char* get_namespace(){
		return "subtitle";
	}
#ifdef _MSC_VER
	const CLSID* get_filter_guid(){
		return &CLSID_Filter;
	}
	const CLSID* get_filter_config_guid(){
		return &IID_Config;
	}
#endif
	const char* get_name(){
		return PROJECT_NAME;
	}
	const wchar_t* get_namew(){
		return L"" PROJECT_NAME;
	}
	const char* get_description(){
		return PROJECT_DESCRIPTION;
	}
	const char* get_version(){
		return PROJECT_VERSION_STRING;
	}
	const char* get_author(){
		return PROJECT_AUTHOR;
	}
	const char* get_copyright(){
		return PROJECT_AUTHOR",© 2015";
	}
	namespace AVS{
		std::vector<std::pair<std::string, ArgType>> get_args(){
			std::vector<std::pair<std::string, ArgType>> args;

			// TODO

			return args;
		}
		void init(VideoInfo vinfo, std::vector<Variant> args, void** userdata) throw (const char*){

			// TODO

		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long ms, void** userdata){

			// TODO

		}
		void deinit(void* userdata){

			// TODO

		}
	}
	namespace CSRI{
		bool init(const char* filename, void** userdata){

			// TODO

			return true;
		}
		bool init(std::istream& stream, void** userdata){

			// TODO

			return true;
		}
		void setup(VideoInfo vinfo, void** userdata){

			// TODO

		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long ms, void** userdata){

			// TODO

		}
		void deinit(void* userdata){

			// TODO

		}
	}
#ifdef _WIN32
	namespace VDub{
		void init(void** userdata) throw (const char*){

			// TODO

		}
		std::string gen_args_desc(void* userdata){

			// TODO

			return "";
		}
		int request_config(HWND wnd, void** userdata){

			// TODO

			return 0;
		}
		void start(VideoInfo vinfo, void** userdata) throw (const char*){

			// TODO

		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long start_ms, unsigned long end_ms, void** userdata){

			// TODO

		}
		void end(void** userdata){

			// TODO

		}
		void deinit(void* userdata){

			// TODO

		}
	}
#ifdef _MSC_VER
	namespace DShow{
		void init(IFilterConfig* config) throw (const char*){

			// TODO

		}
		void start(VideoInfo vinfo, IFilterConfig* config) throw (const char*){

			// TODO

		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long start_ms, unsigned long end_ms, IFilterConfig* config){

			// TODO

		}
		void end(IFilterConfig* config){

			// TODO

		}
		void deinit(IFilterConfig* config){

			// TODO

		}
	}
#endif
#endif
}