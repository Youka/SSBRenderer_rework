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
#include <mferror.h>
#include <atomic>
#include <mutex>
#include <memory>

extern const IID IID_IUNKNOWN;	// Found in uuid library but not in MinGW CGuid.h

// IUnknown deleter
auto iunknown_deleter = [](IUnknown* p){p->Release();};
// Extracts image meta informations from MediaType
struct ImageHeader{
	DWORD width, height;
	GUID subtype;
};
static ImageHeader get_image_header(IMFMediaType* pmt){
	MFVIDEOFORMAT* mvf;
	if(SUCCEEDED(pmt->GetRepresentation(FORMAT_MFVideoFormat, reinterpret_cast<void**>(&mvf)))){
		ImageHeader result = {mvf->videoInfo.dwWidth, mvf->videoInfo.dwHeight, mvf->guidFormat};
		pmt->FreeRepresentation(FORMAT_MFVideoFormat, mvf);
		return result;
	}
	return {0, 0, GUID_NULL};
}

// Filter configuration interface
struct IMyFilterConfig : public FilterBase::MediaF::IFilterConfig, public IUnknown{};

// Filter class
class MyFilter : public IMFTransform, public IMyFilterConfig{
	private:
		// Number of active MyFilter instances
		static std::atomic_uint instances_n; // Member-initialization not allowed...
		// Instance references counter
		std::atomic_ulong refcount;
		// Instance userdata & video format
		void *userdata = nullptr;
		std::unique_ptr<IMFMediaType, std::function<void(IUnknown*)>> input, output;
                std::unique_ptr<IMFSample, std::function<void(IUnknown*)>> sample;
		std::mutex mutex;
		// Destruction of COM object by IUnknown instance->Release
		virtual ~MyFilter(){
			FilterBase::MediaF::deinit(static_cast<FilterBase::MediaF::IFilterConfig*>(this)),
			--instances_n;
		}
	public:
		// Any MyFilter instance is still locked?
		static bool active_instances(){return instances_n != 0;}
		// Ctors&assignment
		MyFilter() : refcount(1), input(nullptr, iunknown_deleter), output(nullptr, iunknown_deleter), sample(nullptr, iunknown_deleter){
			++instances_n,
			FilterBase::MediaF::init(static_cast<FilterBase::MediaF::IFilterConfig*>(this));
		}
		MyFilter(const MyFilter&) = delete;
		MyFilter(MyFilter&&) = delete;
		MyFilter& operator=(const MyFilter&) = delete;
		MyFilter& operator=(MyFilter&&) = delete;
		// IUnknown implementation
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override{
			if(!ppvObject)
				return E_POINTER;
			if(riid == IID_IUNKNOWN)
				*ppvObject = static_cast<IUnknown*>(static_cast<IMFTransform*>(this)); // Because of multiple inheritance of IUnknown, one has to be chosen
			else if(riid == __uuidof(IMFTransform))
				*ppvObject = static_cast<IMFTransform*>(this);
			else if(riid == *FilterBase::get_filter_guid())
				*ppvObject = this;
			else if(riid == *FilterBase::get_filter_config_guid())
				*ppvObject = static_cast<IMyFilterConfig*>(this);
			else{
				*ppvObject = NULL;
				return E_NOINTERFACE;
			}
			this->AddRef();
			return S_OK;
		}
		ULONG STDMETHODCALLTYPE AddRef(void) override{
			return ++this->refcount;
		}
		ULONG STDMETHODCALLTYPE Release(void) override{
			ULONG count = --this->refcount;
			if(count == 0)
				delete this;
			return count;
		}
		// IMyFilterConfig implementation
		void** LockData() override{
			this->mutex.lock();
			return &this->userdata;
		}
		void UnlockData() override{
			this->mutex.unlock();
		}
		void* GetData() override{
			return this->userdata;
		}
		// IMFTransform implementation
		HRESULT STDMETHODCALLTYPE GetStreamLimits(DWORD *pdwInputMinimum, DWORD *pdwInputMaximum, DWORD *pdwOutputMinimum, DWORD *pdwOutputMaximum) override{
			if(!pdwInputMinimum || !pdwInputMaximum || !pdwOutputMinimum || !pdwOutputMaximum)
				return E_POINTER;
			*pdwInputMinimum = *pdwInputMaximum = *pdwOutputMinimum = *pdwOutputMaximum = 1;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetStreamCount(DWORD *pcInputStreams, DWORD *pcOutputStreams) override{
			if(!pcInputStreams || !pcOutputStreams)
				return E_POINTER;
			*pcInputStreams = *pcOutputStreams = 1;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetStreamIDs(DWORD, DWORD *, DWORD, DWORD *) override{
			return E_NOTIMPL; // Fixed number of streams, so ID==INDEX
		}
		HRESULT STDMETHODCALLTYPE GetInputStreamInfo(DWORD dwInputStreamID, MFT_INPUT_STREAM_INFO *pStreamInfo) override{
			if(!pStreamInfo)
				return E_POINTER;
			if(dwInputStreamID != 0) // Just one input stream
				return MF_E_INVALIDSTREAMNUMBER;
			std::unique_lock<std::mutex>(this->mutex);
			pStreamInfo->hnsMaxLatency = 0, // No time difference between input&output sample
			pStreamInfo->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE,
			pStreamInfo->cbMaxLookahead = 0, // No holding data to look
			pStreamInfo->cbAlignment = 0; // No special alignment requirements
			if(this->input){
				auto image_header = get_image_header(this->input.get());
				pStreamInfo->cbSize = image_header.width * image_header.height * (image_header.subtype == MFVideoFormat_RGB24 ? 3 : 4);
			}else
				pStreamInfo->cbSize = 0;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetOutputStreamInfo(DWORD dwOutputStreamID, MFT_OUTPUT_STREAM_INFO *pStreamInfo) override{
			if(!pStreamInfo)
				return E_POINTER;
			if(dwOutputStreamID != 0) // Just one output stream
				return MF_E_INVALIDSTREAMNUMBER;
			std::unique_lock<std::mutex>(this->mutex);
			pStreamInfo->dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE,
			pStreamInfo->cbAlignment = 0;
			if(this->output){
				auto image_header = get_image_header(this->output.get());
				pStreamInfo->cbSize = image_header.width * image_header.height * (image_header.subtype == MFVideoFormat_RGB24 ? 3 : 4);
			}else
				pStreamInfo->cbSize = 0;
			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetAttributes(IMFAttributes **) override{
			return E_NOTIMPL; // No attributes
		}
		HRESULT STDMETHODCALLTYPE GetInputStreamAttributes(DWORD, IMFAttributes **) override{
			return E_NOTIMPL; // No attributes
		}
		HRESULT STDMETHODCALLTYPE GetOutputStreamAttributes(DWORD, IMFAttributes **) override{
			return E_NOTIMPL; // No attributes
		}
		HRESULT STDMETHODCALLTYPE DeleteInputStream(DWORD) override{
			return E_NOTIMPL; // One default input stream, no change
		}
		HRESULT STDMETHODCALLTYPE AddInputStreams(DWORD, DWORD *) override{
			return E_NOTIMPL; // One default input stream, no change
		}
		HRESULT STDMETHODCALLTYPE GetInputAvailableType(DWORD dwInputStreamID, DWORD dwTypeIndex, IMFMediaType **ppType) override{
			if(!ppType)
				return E_POINTER;
			if(dwInputStreamID != 0)
				return MF_E_INVALIDSTREAMNUMBER;

			// TODO

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetOutputAvailableType(DWORD dwOutputStreamID, DWORD dwTypeIndex, IMFMediaType **ppType) override{
			if(!ppType)
				return E_POINTER;
			if(dwOutputStreamID != 0)
				return MF_E_INVALIDSTREAMNUMBER;

			// TODO

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE SetInputType(DWORD dwInputStreamID, IMFMediaType *pType, DWORD dwFlags) override{
			if(dwInputStreamID != 0)
				return MF_E_INVALIDSTREAMNUMBER;

			// TODO

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE SetOutputType(DWORD dwOutputStreamID, IMFMediaType *pType, DWORD dwFlags) override{
			if(dwOutputStreamID != 0)
				return MF_E_INVALIDSTREAMNUMBER;

			// TODO

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetInputCurrentType(DWORD dwInputStreamID, IMFMediaType **ppType) override{
			if(!ppType)
				return E_POINTER;
			if(dwInputStreamID != 0)
				return MF_E_INVALIDSTREAMNUMBER;

			// TODO

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetOutputCurrentType(DWORD dwOutputStreamID, IMFMediaType **ppType) override{
			if(!ppType)
				return E_POINTER;
			if(dwOutputStreamID != 0)
				return MF_E_INVALIDSTREAMNUMBER;

			// TODO

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags) override{
			if(!pdwFlags)
				return E_POINTER;
			if(dwInputStreamID != 0)
				return MF_E_INVALIDSTREAMNUMBER;

			// TODO

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE GetOutputStatus(DWORD *pdwFlags) override{
			if(!pdwFlags)
				return E_POINTER;

			// TODO

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE SetOutputBounds(LONGLONG, LONGLONG) override{
			return E_NOTIMPL; // Optional informations
		}
		HRESULT STDMETHODCALLTYPE ProcessEvent(DWORD, IMFMediaEvent *) override{
			return E_NOTIMPL; // Not event processing
		}
		HRESULT STDMETHODCALLTYPE ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam) override{

			// TODO

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE ProcessInput(DWORD dwInputStreamID, IMFSample *pSample, DWORD dwFlags) override{
			if(!pSample)
				return E_POINTER;
			if(dwInputStreamID != 0)
				return MF_E_INVALIDSTREAMNUMBER;

			// TODO

			return S_OK;
		}
		HRESULT STDMETHODCALLTYPE ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER *pOutputSamples, DWORD *pdwStatus) override{
			if(!pOutputSamples || !pdwStatus)
				return E_POINTER;

			// TODO

			return S_OK;
		}
};
std::atomic_uint MyFilter::instances_n(0);	// ...but direct-initialization

// Filter registration to server
static inline std::wstring gen_clsid_keyname(const GUID& guid){
	wchar_t guid_str[39];
	return StringFromGUID2(guid, guid_str, 39) ? std::wstring(L"CLSID\\") + guid_str : L"";
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
	status = __HRESULT_FROM_WIN32(
		RegCreateKeyExW(
			HKEY_CLASSES_ROOT,
			gen_clsid_keyname(*FilterBase::get_filter_config_guid()).c_str(),
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
		std::wstring key_value = std::wstring(FilterBase::get_namew()) + L" configuration";
		LONG result = RegSetValueExW(key, NULL, 0, REG_SZ, reinterpret_cast<const BYTE*>(key_value.c_str()), (key_value.length()+1)*sizeof(wchar_t));
		status = result == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(result),
		RegCloseKey(key);
	}
	return status;
}

STDAPI __declspec(dllexport) DllUnregisterServer(){
	// Remove MFT enumeration entry in registry
	MFTUnregister(*FilterBase::get_filter_guid());
	// Remove CLSID for CoCreateInstance in registry
	RegDeleteTreeW(HKEY_CLASSES_ROOT, gen_clsid_keyname(*FilterBase::get_filter_guid()).c_str());
	RegDeleteTreeW(HKEY_CLASSES_ROOT, gen_clsid_keyname(*FilterBase::get_filter_config_guid()).c_str());
	return S_OK;
}

STDAPI __declspec(dllexport) DllCanUnloadNow(){
	return MyFilter::active_instances() ? S_FALSE : S_OK;
}

STDAPI __declspec(dllexport) DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv){
	IUnknown* inst;
	if(clsid == *FilterBase::get_filter_guid() && (inst = static_cast<IUnknown*>(static_cast<IMFTransform*>(new MyFilter)))){
		HRESULT status = inst->QueryInterface(riid, ppv);
		inst->Release();
		return status;
	}
	return E_NOINTERFACE;
}
