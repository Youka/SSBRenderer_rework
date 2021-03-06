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
#include "Renderer.hpp"

#ifdef USE_AEGISUB_INTERFACE
#include <regex>
#include "../utils/string.hpp"
#include <iomanip>
#endif

#ifdef _WIN32
#define WIN_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include "vdub_resources.h"
#include "../utils/utf8.hpp"
#include <sstream>

#include <initguid.h>
// {2C80A3B9-6A38-4416-A6E1-8982A37FD3FA}
DEFINE_GUID(CLSID_Filter,
0x2c80a3b9, 0x6a38, 0x4416, 0xa6, 0xe1, 0x89, 0x82, 0xa3, 0x7f, 0xd3, 0xfa);
// {534FD455-C4F5-4719-8196-F315868E02A4}
DEFINE_GUID(IID_FilterConfig,
0x534fd455, 0xc4f5, 0x4719, 0x81, 0x96, 0xf3, 0x15, 0x86, 0x8e, 0x2, 0xa4);
#endif

namespace FilterBase{
	const char* get_id(){
		return "com.subtitle." PROJECT_NAME;
	}
	const char* get_namespace(){
		return "subtitle";
	}
#ifdef _WIN32
	const CLSID* get_filter_guid(){
		return &CLSID_Filter;
	}
	const CLSID* get_filter_config_guid(){
		return &IID_FilterConfig;
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
			return {{"script", ArgType::STRING}, {"warnings", ArgType::BOOL}};
		}
		void init(VideoInfo vinfo, std::vector<Variant> args, void** userdata) throw(std::string){
			if(args[0].type == ArgType::NONE)
				throw std::string("Script filename expected");
			SSB::Colorspace color_space;
			switch(vinfo.format){
				case ColorType::BGR: color_space = SSB::Colorspace::BGR; break;
				case ColorType::BGRA: color_space = SSB::Colorspace::BGRA; break;
				case ColorType::BGRX:
				case ColorType::UNKNOWN:
				default: throw std::string("Invalid color format");	// Should never happen
			}
			try{
				*userdata = new SSB::Renderer(vinfo.width, vinfo.height, color_space, args[0].s, args[1].type != ArgType::NONE && args[1].b);
			}catch(SSB::Exception e){
				throw std::string(e.what());
			}
		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long ms, void** userdata){
			reinterpret_cast<SSB::Renderer*>(*userdata)->render(image_data, stride, ms);
		}
		void deinit(void* userdata){
			delete reinterpret_cast<SSB::Renderer*>(userdata);
		}
	}
	namespace CSRI{
#ifdef USE_AEGISUB_INTERFACE
		static std::iostream& convert_ass_ssb(std::istream& in, std::iostream& out){
			// Regex pattern
			static const std::regex_constants::syntax_option_type setting = std::regex_constants::optimize|std::regex_constants::ECMAScript;
			static const std::regex playres_x("^PlayResX: (\\d+)\\s*$", setting),
				playres_y("^PlayResY: (\\d+)\\s*$", setting),
				ssbstyle("^SSBStyle: ([^,]*),(.*)$", setting),
				style("^Style: ([^,]+),([^,]+),(\\d+),"
					"&H([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}),&H[0-9a-fA-F]{2}([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}),&H([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}),&H[0-9a-fA-F]{2}[0-9a-fA-F]{2}[0-9a-fA-F]{2}[0-9a-fA-F]{2},"
					"(0|-1),(0|-1),(0|-1),(0|-1),(\\d+\\.?\\d*),(\\d+\\.?\\d*),(\\d+\\.?\\d*),(-?\\d+\\.?\\d*),(1|3),(\\d+\\.?\\d*),\\d+\\.?\\d*,([1-9]),(\\d+),\\d+,(\\d+),\\d+\\s*$", setting),
				dialog("^(Dialogue|Comment): \\d+,(\\d:[0-5]\\d:[0-5]\\d\\.\\d{2}),(\\d:[0-5]\\d:[0-5]\\d\\.\\d{2}),([^,]*),[^,]*,\\d+,\\d+,\\d+,[^,]*,(.*)$", setting);
			std::smatch m;
			// SSB state
			SSB::Data::Section current_section = SSB::Data::Section::NONE;
			// Go through text lines
			std::string line;
			while(std::getline(in, line))
				if(std::regex_search(line, m, playres_x)){
					if(current_section != SSB::Data::Section::FRAME)
						out << "#FRAME\n",
						current_section = SSB::Data::Section::FRAME;
					out << "Width: " << m[1] << '\n';
				}else if(std::regex_search(line, m, playres_y)){
					if(current_section != SSB::Data::Section::FRAME)
						out << "#FRAME\n",
						current_section = SSB::Data::Section::FRAME;
					out << "Height: " << m[1] << '\n';
				}else if(std::regex_search(line, m, ssbstyle)){
					if(current_section != SSB::Data::Section::STYLES)
						out << "#STYLES\n",
						current_section = SSB::Data::Section::STYLES;
					out << m[1] << ": " << m[2] << '\n';
				}else if(std::regex_search(line, m, style)){
					if(current_section != SSB::Data::Section::STYLES)
						out << "#STYLES\n",
						current_section = SSB::Data::Section::STYLES;
					short al, lal;
					float scx, scy;
					stdex::hex_string_to_number(m[4], al),
					stdex::hex_string_to_number(m[11], lal),
					stdex::string_to_number(m[19], scx),
					stdex::string_to_number(m[20], scy),
					out << m[1] << ": " << "{ff=" << m[2] << ";fs=" << m[3]
						<< ";cl=" << m[7] << m[6] << m[5]
						<< ";kc=" << m[10] << m[9] << m[8]
						<< ";lcl=" << m[14] << m[13] << m[12]
						<< ";al=" << std::hex << std::setfill('0') << std::setw(2) << 255-al << ";lal=" << std::setw(2) << 255-lal << std::dec
						<< ";fst=" << (m[15] == "-1" ? "b" : "") << (m[16] == "-1" ? "i" : "") << (m[17] == "-1" ? "u" : "") << (m[18] == "-1" ? "s" : "")
						<< ";scx=" << scx / 100 << ";scy=" << scy / 100
						<< ";fsph=" << m[21] << ";rz=" << m[22] << ";md=" << (m[23].str()[0] == '3' ? 'b' : 'f')
						<< ";lw=" << m[24] << ";an=" << m[25] << ";mgh=" << m[26] << ";mgv=" << m[27] << "}\n";
				}else if(std::regex_search(line, m, dialog)){
					if(current_section != SSB::Data::Section::EVENTS)
						out << "#EVENTS\n",
						current_section = SSB::Data::Section::EVENTS;
					if(m[1] == "Comment")
						out << "// ";
					out << m[2] << "0-" << m[3] << "0|" << m[4] << "||" << m[5] << '\n';
				}
			// Rewind output/new input
			out.seekg(0);
			return out;
		}
#endif
		bool init(const char* filename, void** userdata){
			try{
				*userdata = new SSB::Renderer(0, 0, SSB::Colorspace::BGR, filename, false);
			}catch(SSB::Exception){
				return false;
			}
			return true;
		}
		bool init(std::istream& stream, void** userdata){
#ifdef USE_AEGISUB_INTERFACE
			std::stringstream ssb_stream;
			try{
				*userdata = new SSB::Renderer(0, 0, SSB::Colorspace::BGR, convert_ass_ssb(stream, ssb_stream), false);
#else
			try{
				*userdata = new SSB::Renderer(0, 0, SSB::Colorspace::BGR, stream, false);
#endif
			}catch(SSB::Exception){
				return false;
			}
			return true;
		}
		void setup(VideoInfo vinfo, void** userdata){
			SSB::Colorspace color_space;
			switch(vinfo.format){
				case ColorType::BGR: color_space = SSB::Colorspace::BGR; break;
				case ColorType::BGRA: color_space = SSB::Colorspace::BGRA; break;
				case ColorType::BGRX: color_space = SSB::Colorspace::BGRX; break;
				case ColorType::UNKNOWN:
				default: throw SSB::Exception("Something terrible happened with CSRI interface");	// Should never happen
			}
			reinterpret_cast<SSB::Renderer*>(*userdata)->set_target(vinfo.width, vinfo.height, color_space);
		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long ms, void** userdata){
			reinterpret_cast<SSB::Renderer*>(*userdata)->render(image_data, stride, ms);
		}
		void deinit(void* userdata){
			delete reinterpret_cast<SSB::Renderer*>(userdata);
		}
	}
#ifdef _WIN32
	HANDLE module;
	namespace VDub{
		struct Userdata{
			std::string script;
			bool warnings;
			std::unique_ptr<SSB::Renderer> renderer;
		};
		void init(void** userdata) throw(std::string){
			*userdata = new Userdata{"", true, nullptr};
		}
		std::string gen_args_desc(void* userdata){
			Userdata* myuserdata = reinterpret_cast<Userdata*>(userdata);
			std::ostringstream desc;
			desc << "    Script: \"" << myuserdata->script << "\" - Warnings: " << (myuserdata->warnings ? "ON" : "OFF");
			return desc.str();
		}
		INT_PTR CALLBACK config_callback(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam){
			// Evaluate window message
			switch(msg){
				// Window initialization
				case WM_INITDIALOG:
					{
						Userdata* userdata = reinterpret_cast<Userdata*>(lParam);
						// Set window default contents
						HWND edit = GetDlgItem(wnd, VDUB_DIALOG_FILENAME);
						SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(Utf8::to_utf16(userdata->script).c_str())),
						SendMessageW(edit, EM_SETSEL, 0, -1),
						SendMessageW(GetDlgItem(wnd, VDUB_DIALOG_CHECK), BM_SETCHECK, userdata->warnings, 0),
						// Store userdata to window
						SetWindowLongPtrA(wnd, DWLP_USER, reinterpret_cast<LONG_PTR>(userdata));
					}
					break;
				// Window action
				case WM_COMMAND:
					switch(wParam){
						// '...' button pressed
						case VDUB_DIALOG_FILENAME_CHOOSE:
							{
								// Target buffer (& default) for file dialog
								wchar_t filename[MAX_PATH]; filename[0] = '\0';
								// Describe file dialog
								OPENFILENAMEW ofn = {0};
								ofn.lStructSize = sizeof(OPENFILENAMEW),
								ofn.hwndOwner = wnd,
								ofn.hInstance = reinterpret_cast<HINSTANCE>(module),
								ofn.lpstrFilter = L"SSB file (*.ssb)\0*.ssb\0\0",
								ofn.nFilterIndex = 1,
								ofn.lpstrFile = filename,
								ofn.nMaxFile = sizeof(filename),
								ofn.Flags = OFN_FILEMUSTEXIST;
								// Show file dialog
								if(GetOpenFileNameW(&ofn)){
									// Save filename input of dialog
									HWND edit = GetDlgItem(wnd, VDUB_DIALOG_FILENAME);
									SendMessageW(edit, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(ofn.lpstrFile));
									SendMessageW(edit, EM_SETSEL, 0, -1);
								}
							}
							break;
						// 'OK' button pressed
						case IDOK:
							{
								Userdata* userdata = reinterpret_cast<Userdata*>(GetWindowLongPtrA(wnd, DWLP_USER));
								// Save window contents to userdata
								HWND edit = GetDlgItem(wnd, VDUB_DIALOG_FILENAME);
								std::wstring script(static_cast<int>(SendMessageW(edit, WM_GETTEXTLENGTH, 0, 0))+1, L'\0'); // Space for text + null-termination
								SendMessageW(edit, WM_GETTEXT, script.length(), reinterpret_cast<LPARAM>(script.data())), // Fills space with text + null-termination
								script.pop_back(), // Remove null-termination
								userdata->script = Utf8::from_utf16(script),
								userdata->warnings = SendMessageW(GetDlgItem(wnd, VDUB_DIALOG_CHECK), BM_GETCHECK, 0, 0),
								// Successful end
								EndDialog(wnd, 0);
							}
							break;
						// 'Cancel' button pressed
						case IDCANCEL:
							// Unsuccessful end
							EndDialog(wnd, 1);
							break;
					}
					break;
				// Window should close
				case WM_CLOSE:
					// Unsuccessful end
					EndDialog(wnd, 1);
					break;
				// Message not handled (continue with default behaviour)
				default:
					return FALSE;
			}
			// Message handled
			return TRUE;
		}
		int request_config(HWND wnd, void** userdata){
			return DialogBoxParamW(reinterpret_cast<HINSTANCE>(module), MAKEINTRESOURCEW(VDUB_DIALOG), wnd, config_callback, reinterpret_cast<LPARAM>(*userdata));
		}
		void start(VideoInfo vinfo, void** userdata) throw(std::string){
			Userdata* myuserdata = reinterpret_cast<Userdata*>(*userdata);
			SSB::Colorspace color_space;
			switch(vinfo.format){
				case ColorType::BGRX: color_space = SSB::Colorspace::BGRX; break;
				case ColorType::BGR:
				case ColorType::BGRA:
				case ColorType::UNKNOWN:
				default: throw std::string("Invalid color format");	// Should never happen
			}
			try{
				myuserdata->renderer.reset(new SSB::Renderer(vinfo.width, vinfo.height, color_space, myuserdata->script, myuserdata->warnings));
			}catch(SSB::Exception e){
				throw std::string(e.what());
			}
		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long start_ms, unsigned long, void** userdata){
			reinterpret_cast<Userdata*>(*userdata)->renderer->render(image_data, stride, start_ms);
		}
		void end(void** userdata){
			reinterpret_cast<Userdata*>(*userdata)->renderer.reset();
		}
		void deinit(void* userdata){
			delete reinterpret_cast<Userdata*>(userdata);
		}
	}
	namespace MediaF{
		struct Userdata{
			std::string script;
			bool warnings;
			std::unique_ptr<SSB::Renderer> renderer;
		};
		void init(IFilterConfig* config) throw(std::string){
			*config->LockData() = new Userdata{"", true, nullptr},
			config->UnlockData();
		}
		void start(VideoInfo vinfo, IFilterConfig* config) throw(std::string){
			Userdata* myuserdata = reinterpret_cast<Userdata*>(*config->LockData());
			SSB::Colorspace color_space;
			switch(vinfo.format){
				case ColorType::BGRX: color_space = SSB::Colorspace::BGRX; break;
				case ColorType::BGR:
				case ColorType::BGRA:
				case ColorType::UNKNOWN:
				default: throw std::string("Invalid color format");	// Should never happen
			}
			try{
				myuserdata->renderer.reset(new SSB::Renderer(vinfo.width, vinfo.height, color_space, myuserdata->script, myuserdata->warnings));
			}catch(SSB::Exception e){
				config->UnlockData();
				throw std::string(e.what());
			}
			config->UnlockData();
		}
		void filter_frame(unsigned char* image_data, int stride, unsigned long start_ms, unsigned long, IFilterConfig* config){
			reinterpret_cast<Userdata*>(*config->LockData())->renderer->render(image_data, stride, start_ms),
			config->UnlockData();
		}
		void end(IFilterConfig* config){
			reinterpret_cast<Userdata*>(*config->LockData())->renderer.reset(),
			config->UnlockData();
		}
		void deinit(IFilterConfig* config){
			delete reinterpret_cast<Userdata*>(*config->LockData());
			config->UnlockData();
		}
	}
#endif
}

#ifdef _WIN32
#ifdef __cplusplus
extern "C"
#endif
BOOL APIENTRY DllMain(HANDLE dll_module, DWORD reason, LPVOID){
	if(reason == DLL_PROCESS_ATTACH)
		FilterBase::module = dll_module;
	return TRUE;
}
#endif
