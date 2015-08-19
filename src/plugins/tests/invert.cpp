/*
Project: SSBRenderer
File: invert.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include "../FilterBase.hpp"
#include <iostream>
#include <algorithm>
#include <iterator>

#ifdef _WIN32
#include <initguid.h>
// {7607EC1E-B547-4780-8217-F407C9D2C78F}
DEFINE_GUID(CLSID_Filter,
0x7607ec1e, 0xb547, 0x4780, 0x82, 0x17, 0xf4, 0x7, 0xc9, 0xd2, 0xc7, 0x8f);
// {69F30E60-8F45-4c99-8315-0598CAE1E395}
DEFINE_GUID(IID_Config,
0x69f30e60, 0x8f45, 0x4c99, 0x83, 0x15, 0x5, 0x98, 0xca, 0xe1, 0xe3, 0x95);
#endif

namespace FilterBase{
	const char* get_id(){
		return "generic test id: 21305870";
	}
	const char* get_namespace(){
		return "tests";
	}
	#ifdef _WIN32
	const CLSID* get_filter_guid(){
		return &CLSID_Filter;
	}
	const CLSID* get_filter_config_guid(){
		return &IID_Config;
	}
	#endif
	const char* get_name(){
		return "Test";
	}
	const wchar_t* get_namew(){
		return L"Test";
	}
	const char* get_description(){
		return "Just a test, nothing more!";
	}
	const char* get_version(){
		return "0";
	}
	const char* get_author(){
		return "Youka";
	}
	const char* get_copyright(){
		return "...";
	}
	namespace AVS{
		std::vector<std::pair<std::string, ArgType>> get_args(){
			std::cout << "AVS - Tried to get arguments." << std::endl;
			return std::vector<std::pair<std::string, ArgType>>();
		}
		void init(VideoInfo vinfo, std::vector<Variant>, void** userdata) throw(std::string){
			std::cout << "AVS - Tried to initialize: " << vinfo.width << "x" << vinfo.height;
			switch(vinfo.format){
				case ColorType::BGR: std::cout << " BGR"; break;
				case ColorType::BGRA: std::cout << " BGRA"; break;
				case ColorType::BGRX: std::cout << " BGRX"; break;
				case ColorType::UNKNOWN: std::cout << " UNKNOWN"; break;
			}
			std::cout << ' ' << vinfo.fps << "fps #" << vinfo.frames << std::endl;
			*userdata = new int(vinfo.height);
		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long ms, void** userdata){
			std::cout << "AVS - Filter on time: " << ms << "ms" << std::endl;
			std::transform(image_data, image_data + ::abs(*(reinterpret_cast<int*>(*userdata))) * stride,
					image_data,
					[](unsigned char elem){return 255 - elem;});
		}
		void deinit(void* userdata){
			std::cout << "AVS - Tried to close." << std::endl;
			delete reinterpret_cast<int*>(userdata);
		}
	}
	namespace CSRI{
		bool init(const char* filename, void** userdata){
			std::cout << "CSRI - Tried to load file: " << filename << std::endl;
			*userdata = new int(0);
			return true;
		}
		bool init(std::istream& stream, void** userdata){
			std::cout << "### CSRI - Tried to load data ###\n--------------------------\n";
			std::copy(std::istream_iterator<char>(stream), std::istream_iterator<char>(), std::ostream_iterator<char>(std::cout));
			std::cout << std::endl;
			*userdata = new int(0);
			return true;
		}
		void setup(VideoInfo vinfo, void** userdata){
			std::cout << "CSRI - Tried to setup: " << vinfo.width << "x" << vinfo.height;
			switch(vinfo.format){
				case ColorType::BGR: std::cout << " BGR"; break;
				case ColorType::BGRA: std::cout << " BGRA"; break;
				case ColorType::BGRX: std::cout << " BGRX"; break;
				case ColorType::UNKNOWN: std::cout << " UNKNOWN"; break;
			}
			std::cout << ' ' << vinfo.fps << "fps #" << vinfo.frames << std::endl;
			*(reinterpret_cast<int*>(*userdata)) = vinfo.height;
		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long ms, void** userdata){
			std::cout << "CSRI - Filter on time: " << ms << "ms" << std::endl;
			std::transform(image_data, image_data + ::abs(*(reinterpret_cast<int*>(*userdata))) * stride,
					image_data,
					[](unsigned char elem){return 255 - elem;});
		}
		void deinit(void* userdata){
			std::cout << "CSRI - Tried to close." << std::endl;
			delete reinterpret_cast<int*>(userdata);
		}
	}
#ifdef _WIN32
	namespace VDub{
		void init(void**) throw(std::string){
			std::cout << "VDub - Tried to initialize." << std::endl;
		}
		std::string gen_args_desc(void*){
			std::cout << "VDub - Tried to generate arguments description." << std::endl;
			return "";
		}
		int request_config(HWND wnd, void**){
			std::cout << "VDub - Tried to request configuration with window handle " << wnd << " ." << std::endl;
			return 0;
		}
		void start(VideoInfo vinfo, void** userdata) throw(std::string){
			std::cout << "VDub - Tried to start: " << vinfo.width << "x" << vinfo.height;
			switch(vinfo.format){
				case ColorType::BGR: std::cout << " BGR"; break;
				case ColorType::BGRA: std::cout << " BGRA"; break;
				case ColorType::BGRX: std::cout << " BGRX"; break;
				case ColorType::UNKNOWN: std::cout << " UNKNOWN"; break;
			}
			std::cout << ' ' << vinfo.fps << "fps #" << vinfo.frames << std::endl;
			*userdata = new int(vinfo.height);
		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long start_ms, unsigned long end_ms, void** userdata){
			std::cout << "VDub - Filter on time: " << start_ms << "ms -> " << end_ms << "ms" << std::endl;
			std::transform(image_data, image_data + ::abs(*(reinterpret_cast<int*>(*userdata))) * stride,
					image_data,
					[](unsigned char elem){return 255 - elem;});
		}
		void end(void** userdata){
			std::cout << "VDub - Tried to end." << std::endl;
			delete reinterpret_cast<int*>(*userdata);
		}
		void deinit(void*){
			std::cout << "VDub - Tried to close." << std::endl;
		}
	}
	namespace MediaF{
		void init(IFilterConfig*) throw(std::string){
			std::cout << "MediaF - Tried to initialize." << std::endl;
		}
		void start(VideoInfo vinfo, IFilterConfig* config) throw(std::string){
			std::cout << "MediaF - Tried to start: " << vinfo.width << "x" << vinfo.height;
			switch(vinfo.format){
				case ColorType::BGR: std::cout << " BGR"; break;
				case ColorType::BGRA: std::cout << " BGRA"; break;
				case ColorType::BGRX: std::cout << " BGRX"; break;
				case ColorType::UNKNOWN: std::cout << " UNKNOWN"; break;
			}
			std::cout << ' ' << vinfo.fps << "fps #" << vinfo.frames << std::endl;
			int** ppheight = reinterpret_cast<int**>(config->LockData());
			delete *ppheight;
			*ppheight = new int(vinfo.height);
			config->UnlockData();
		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long start_ms, unsigned long end_ms, IFilterConfig* config){
			std::cout << "MediaF - Filter on time: " << start_ms << "ms -> " << end_ms << "ms" << std::endl;
			int* pheight = reinterpret_cast<int*>(*config->LockData());
			if(pheight){
				int height = *pheight;
				config->UnlockData();
				std::transform(image_data, image_data + ::abs(height) * stride,
						image_data,
						[](unsigned char elem){return 255 - elem;});
			}else
				config->UnlockData();
		}
		void end(IFilterConfig* config){
			std::cout << "MediaF - Tried to end." << std::endl;
			int** ppheight = reinterpret_cast<int**>(config->LockData());
			delete *ppheight;
			*ppheight = nullptr;
			config->UnlockData();
		}
		void deinit(IFilterConfig*){
			std::cout << "MediaF - Tried to close." << std::endl;
		}
	}
#endif
}
