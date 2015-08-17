/*
Project: SSBRenderer
File: mediaf.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#define _WIN32_WINNT _WIN32_WINNT_VISTA // Enables RegDeleteTree function

#include "FilterBase.hpp"
#include <mftransform.h>
#include <mfapi.h>
#include <atomic>

// Filter class
class MyFilter : public IMFTransform{
	private:
		static std::atomic_uint locks; // Hopefully zero-set by default initialization
	public:
		static bool locked(){return locks != 0;}

		// TODO

};

static inline std::wstring gen_clsid_keyname(const GUID& guid){
	wchar_t guid_str[40];
	return StringFromGUID2(guid, guid_str, 40) ? std::wstring(L"CLSID\\") + guid_str : L"";
}
static std::wstring get_module_name(){
	HMODULE module;
	wchar_t module_name[MAX_PATH];
	return GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCSTR>(&module), &module) && GetModuleFileNameW(module, module_name, MAX_PATH) ? module_name : L"";
}
STDAPI __declspec(dllexport) DllRegisterServer(){
	// Create CLSID for CoCreateInstance entry in registry
	HKEY key;
	HRESULT status = __HRESULT_FROM_WIN32(
		RegCreateKeyExW(
			HKEY_CLASSES_ROOT,
			gen_clsid_keyname(*FilterBase::get_filter_guid()).c_str(),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&key,
			NULL
		)
	);
	if(SUCCEEDED(status)){
		const wchar_t* key_value = FilterBase::get_namew();
		LONG result = RegSetValueExW(key, NULL, 0, REG_SZ, reinterpret_cast<const BYTE*>(key_value), (wcslen(key_value)+1)*sizeof(wchar_t));
		status = result == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(result);
		if(SUCCEEDED(status)){
			HKEY subkey;
			status = __HRESULT_FROM_WIN32(
				RegCreateKeyExW(
					key,
					L"InprocServer32",
					0,
					NULL,
					REG_OPTION_NON_VOLATILE,
					KEY_ALL_ACCESS,
					NULL,
					&subkey,
					NULL
				)
			);
			if(SUCCEEDED(status)){
				std::wstring module_name = get_module_name();
				result = RegSetValueExW(subkey, NULL, 0, REG_SZ, reinterpret_cast<const BYTE*>(module_name.c_str()), (module_name.length()+1)*sizeof(wchar_t)),
				status = result == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(result);
				if(SUCCEEDED(status)){
					result = RegSetValueExW(subkey, L"ThreadingModel", 0, REG_SZ, reinterpret_cast<const BYTE*>("Both"), 10),
					status = result == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(result);
					if(SUCCEEDED(status)){
						// Create MFT enumeration entry in registry
						MFT_REGISTER_TYPE_INFO media_types[] = {
							{MFMediaType_Video, MFVideoFormat_RGB24},
							{MFMediaType_Video, MFVideoFormat_RGB32},
							{MFMediaType_Video, MFVideoFormat_ARGB32}
						};
						status = MFTRegister(
								*FilterBase::get_filter_guid(),
								MFT_CATEGORY_VIDEO_EFFECT,
								const_cast<wchar_t*>(FilterBase::get_namew()),
								0x0,
								sizeof(media_types)/sizeof(media_types[0]),
								media_types,
								sizeof(media_types)/sizeof(media_types[0]),
								media_types,
								NULL
							);
					}
				}
				RegCloseKey(subkey);
			}
		}
		RegCloseKey(key);
	}
	return status;
}

STDAPI __declspec(dllexport) DllUnregisterServer(){
	// Remove MFT enumeration entry in registry
	MFTUnregister(*FilterBase::get_filter_guid());
	// Remove CLSID for CoCreateInstance in registry
	RegDeleteTreeW(HKEY_CLASSES_ROOT, gen_clsid_keyname(*FilterBase::get_filter_guid()).c_str());
	return S_OK;
}

STDAPI __declspec(dllexport) DllCanUnloadNow(){
	return MyFilter::locked() ? S_FALSE : S_OK;
}

STDAPI __declspec(dllexport) DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv){
	IUnknown* inst;
	if(clsid == *FilterBase::get_filter_guid() && (inst = new MyFilter)){
		inst->QueryInterface(riid, ppv),
		inst->Release();
		return S_OK;
	}
	return E_NOINTERFACE;
}
