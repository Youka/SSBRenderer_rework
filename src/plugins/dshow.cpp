/*
Project: SSBRenderer
File: dshow.cpp

Copyright (c) 2015, Christoph "Youka" Spanknebel

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

// Windows 'Globally Unique Identifier'
#include <initguid.h>
#include <CGuid.h>
// Windows streams
#include <streams.h>

#include "FilterBase.hpp"

// Filter configuration interface
interface IFilterConfig : public IUnknown{
	virtual void* Configure(void*) = 0;
};

// TODO

// Video filter
class MyFilter : public CVideoTransformFilter, public IFilterConfig{
	public:
		// Create class instance
		static CUnknown* CALLBACK CreateInstance(LPUNKNOWN unknown, HRESULT* result){

			// TODO

			return nullptr;
		}
		// Filter configuration
		void* Configure(void* conf){

			// TODO

			return nullptr;
		}
};

// Filter pins
const AMOVIESETUP_MEDIATYPE sudPinTypes[] = {
	// Support RGB24 colorspace
	{
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_RGB24
	},
	// Support RGB32 colorspace
	{
		&MEDIATYPE_Video,
		&MEDIASUBTYPE_RGB32
	}
};

const AMOVIESETUP_PIN sudpPins[] = {
	{
		L"Input",             // Pin string name
		FALSE,                // Is it rendered
		FALSE,                // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		nullptr,                 // Connects to pin
		2,                    // Number of media types
		sudPinTypes          // Media types
	},
	{
		L"Output",            // Pin string name
		FALSE,                // Is it rendered
		TRUE,                 // Is it an output
		FALSE,                // Are we allowed none
		FALSE,                // And allowed many
		&CLSID_NULL,          // Connects to filter
		nullptr,                 // Connects to pin
		2,                    // Number of media types
		sudPinTypes          // Media types
	}
};

// Filter setup
const AMOVIESETUP_FILTER sudFilter = {
	reinterpret_cast<const CLSID*>(FilterBase::get_filter_guid()),         // Filter CLSID
	FilterBase::get_namew(),       // Filter name
	MERIT_DO_NOT_USE,       // Filter merit
	2,                      // Number of pins
	sudpPins                // Pin information
};

// Filter definition as COM objects
CFactoryTemplate g_Templates[] = {
	{
		FilterBase::get_namew(),	// Filter name
		reinterpret_cast<const CLSID*>(FilterBase::get_filter_guid()),		// Filter CLSID
		MyFilter::CreateInstance,	// Filter instance creation
		nullptr,		// Init routine
		&sudFilter		// Filter setup
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(CFactoryTemplate);	// Number of COM objects

// Register filter to server
EXTERN_C __declspec(dllexport) HRESULT WINAPI DllRegisterServer(){
	return AMovieDllRegisterServer2(TRUE);
}

// Unregister filter from server
EXTERN_C __declspec(dllexport) HRESULT WINAPI DllUnregisterServer(){
	return AMovieDllRegisterServer2(FALSE);
}

// Further DLL initializations for DirectShow
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

// DLL entry point
BOOL APIENTRY DllMain(HANDLE module, DWORD reason, LPVOID reserved){
	return DllEntryPoint(reinterpret_cast<HINSTANCE>(module), reason, reserved);
}
