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
interface IFilterConfig : public FilterBase::DShow::IFilterConfig, public IUnknown{};

// Video filter
class MyFilter : public CVideoTransformFilter, public IFilterConfig{
	private:
		// Critical section for save configuration access from other interfaces
		CCritSec crit_section;
		// Configuration data
		void* userdata = nullptr;
		// Filter instance constructor
		MyFilter(IUnknown* unknown) : CVideoTransformFilter(FilterBase::get_namew, unknown, *FilterBase::get_filter_guid()) throw(std::string){
			FilterBase::DShow::init(dynamic_cast<FilterBase::DShow::IFilterConfig*>(this));
		}
	public:
		// Create class instance
		static CUnknown* CALLBACK CreateInstance(LPUNKNOWN unknown, HRESULT* result){
			MyFilter* filter = nullptr;
			try{
				if(!(filter = new MyFilter(unknown)))
					*result = E_OUTOFMEMORY;
			}catch(std::string err){
                                delete filter;
                                *result = E_FAIL;
                                MessageBoxA(NULL, err.c_str(), FilterBase::get_name(), MB_OK | MB_ICONERROR);
			}
			return filter;
		}
		// Filter instance destruction
		~MyFilter(){
			FilterBase::DShow::deinit(dynamic_cast<FilterBase::DShow::IFilterConfig*>(this));
		}
		// Check validation of input media stream
		HRESULT CheckInputType(const CMediaType* In){
			// Valid pointer?
			CheckPointer(In, E_POINTER);
			// Valid stream type?
			if(In->majortype != MEDIATYPE_Video || (In->subtype != MEDIASUBTYPE_RGB24 && In->subtype != MEDIASUBTYPE_RGB32) ||
				In->formattype != FORMAT_VideoInfo || In->cbFormat < sizeof(VIDEOINFOHEADER))
				return VFW_E_TYPE_NOT_ACCEPTED;
			// Valid bitmap?
			BITMAPINFOHEADER* bmp = &reinterpret_cast<VIDEOINFOHEADER*>(In->pbFormat)->bmiHeader;
			if((bmp->biBitCount != 24 && bmp->biBitCount != 32) || bmp->biCompression != BI_RGB)
				return VFW_E_TYPE_NOT_ACCEPTED;
			// Media type accepted
			return S_OK;
		}
		// Prefered output media stream type
		HRESULT GetMediaType(int position, CMediaType* Out){
			// Valid pointer?
			CheckPointer(Out, E_POINTER);
			// Input pin isn't connected
			if(!this->m_pInput->IsConnected())
				return VFW_E_NOT_CONNECTED;
			// Item pick error
			if(position < 0)
				return E_ACCESSDENIED;
			// No further items
			if(position > 0)
				return VFW_S_NO_MORE_ITEMS;
			// Output type = input type
			HRESULT hr = this->m_pInput->ConnectionMediaType(Out);
			if(FAILED(hr))
				return hr;
			// Output accepted
			return S_OK;
		}
		// Checks compatibility of input & output pin
		HRESULT CheckTransform(const CMediaType* In, const CMediaType* Out){
			// Valid pointers?
			CheckPointer(In, E_POINTER);
			CheckPointer(Out, E_POINTER);
			// In- & output the same?
			return this->CheckInputType(In) == S_OK && *In == *Out ? S_OK : VFW_E_INVALIDMEDIATYPE;
		}
		// Allocate buffers for in- & output
		HRESULT DecideBufferSize(IMemAllocator* alloc, ALLOCATOR_PROPERTIES* props){
			// Valid pointers?
			CheckPointer(alloc, E_POINTER);
			CheckPointer(props, E_POINTER);
			// Input pin isn't connected
			if(!this->m_pInput->IsConnected())
				return VFW_E_NOT_CONNECTED;
			// Set buffer size
			props->cBuffers = 1;
			props->cbBuffer = this->m_pInput->CurrentMediaType().GetSampleSize();
			// Allocate buffer memory
			ALLOCATOR_PROPERTIES actual;
			HRESULT hr = alloc->SetProperties(props,&actual);
			if (FAILED(hr))
				return hr;
			// Enough memory allocated?
			if (actual.cBuffers < props->cBuffers ||
				actual.cbBuffer < props->cbBuffer)
				return E_OUTOFMEMORY;
			// Got memory
			return S_OK;
		}
		// Frame processing
		HRESULT Transform(IMediaSample* In, IMediaSample* Out){
			// Valid pointers?
			CheckPointer(In, E_POINTER);
			CheckPointer(Out, E_POINTER);
			// Get bitmap info
			BITMAPINFOHEADER *bmp_in = &reinterpret_cast<VIDEOINFOHEADER*>(this->m_pInput->CurrentMediaType().pbFormat)->bmiHeader;
			BITMAPINFOHEADER *bmp_out = &reinterpret_cast<VIDEOINFOHEADER*>(this->m_pOutput->CurrentMediaType().pbFormat)->bmiHeader;
			// Calculate pitches (from BITMAPINFOHEADER remarks)
			int pitch_src = (((bmp_in->biWidth * bmp_in->biBitCount) + 31) & ~31) >> 3;
			int pitch_dst = (((bmp_out->biWidth * bmp_in->biBitCount) + 31) & ~31) >> 3;
			// Get absolute frame height
			int abs_height = ::abs(bmp_in->biHeight);
			// Set output size
			Out->SetActualDataLength(abs_height * pitch_dst);
			// Get frame pointers
			BYTE *src, *dst;
			HRESULT hr;
			hr = In->GetPointer(&src);
			if(FAILED(hr))
				return hr;
			hr = Out->GetPointer(&dst);
			if(FAILED(hr))
				return hr;
			// Copy image to output
			if(pitch_src == pitch_dst)
				std::copy(src, src+In->GetActualDataLength(), dst);
			else if(pitch_dst > pitch_src){
				unsigned char* psrc = src, *pdst = dst;
				for(int y = 0; y < abs_height; ++y){
					std::copy(psrc, psrc+pitch_src, pdst);
					psrc += pitch_src;
					pdst += pitch_dst;
				}
			}else	// pitch_dst < pitch_src
				// How can the target memory be smaller than the source???
				return E_UNEXPECTED;
			// Get time
			LONGLONG start, end;
			hr = In->GetTime(&start, &end);
			if(FAILED(hr))
				return hr;
			// Filter frame and invert vertically if required
			FilterBase::DShow::filter_frame(dst, pitch_dst, start / 10000, end / 10000, dynamic_cast<FilterBase::DShow::IFilterConfig*>(this));
			// Frame successfully filtered
			return S_OK;
		}
		// Start frame streaming
		HRESULT StartStreaming(){
			// Get video infos
			BITMAPINFOHEADER *bmp = &reinterpret_cast<VIDEOINFOHEADER*>(this->m_pInput->CurrentMediaType().pbFormat)->bmiHeader;
			try{
				FilterBase::DShow::start(bmp->biWidth, bmp->biHeight, bmp->biBitCount == 32 ? FilterBase::ColorType::BGRA : FilterBase::ColorType::BGR, dynamic_cast<FilterBase::DShow::IFilterConfig*>(this));
			}catch(std::string err){
				MessageBoxA(NULL, err.c_str(), FilterBase::get_name(), MB_OK | MB_ICONSTOP);
				return VFW_E_WRONG_STATE;
			}
			// Continue with default behaviour
			return CVideoTransformFilter::StartStreaming();
		}
		// Stop frame streaming
		HRESULT StopStreaming(){
			FilterBase::DShow::end(dynamic_cast<FilterBase::DShow::IFilterConfig*>(this));
			// Continue with default behaviour
			return CVideoTransformFilter::StopStreaming();
		}
		// Number of filter pins
		int GetPinCount(){
			return 2;
		}
		// Get filter pins
		CBasePin* GetPin(int n){
			// Pick pin by index
			switch(n){
				case 0:
					// Input pin
					if (!this->m_pInput){
						// Create new one
						HRESULT hr = S_OK;
						this->m_pInput = new CTransformInputPin(L"Video input", this, &hr, L"Video input");
						if (FAILED(hr))
							return nullptr;
					}
					return this->m_pInput;
				case 1:
					// Output pin
					if (!this->m_pOutput){
						// Create new one
						HRESULT hr = S_OK;
						this->m_pOutput = new CTransformOutputPin(L"Video output", this, &hr, L"Video output");
						if (FAILED(hr))
							return nullptr;
					}
					return this->m_pOutput;
				default:
					// Not expected pin
					return nullptr;
			}
		}
		// Answer to interface requests from outside
		HRESULT CALLBACK NonDelegatingQueryInterface(REFIID riid, __deref_out void**ppv){
			// Valid pointer?
			CheckPointer(ppv, E_POINTER);
			// Return filter configuration interface
			if(riid == *FilterBase::get_filter_config_guid())
				return GetInterface(reinterpret_cast<IFilterConfig*>(this), ppv);
			// Return default interfaces
			return CVideoTransformFilter::NonDelegatingQueryInterface(riid, ppv);
		}
		// Define COM object base methods
		DECLARE_IUNKNOWN;
		// Filter configuration
		void** LockData(){
			// Lock critical section for thread-safety
			this->crit_section.Lock();
			// Return now thread safe memory
			return &this->userdata;
		}
		void UnlockData(){
			this->crit_section.Unlock();
		}
		void* GetData(){
			return this->userdata;
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
	FilterBase::get_filter_guid(),         // Filter CLSID
	FilterBase::get_namew(),       // Filter name
	MERIT_DO_NOT_USE,       // Filter merit
	2,                      // Number of pins
	sudpPins                // Pin information
};

// Filter definition as COM objects
CFactoryTemplate g_Templates[] = {
	{
		FilterBase::get_namew(),	// Filter name
		FilterBase::get_filter_guid(),		// Filter CLSID
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
