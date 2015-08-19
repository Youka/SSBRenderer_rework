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
#include <strmif.h>
#include <atomic>
#include <mutex>
#include <memory>

extern const IID IID_IUNKNOWN;	// Found in uuid library but not in MinGW CGuid.h

namespace MediaF{
	// IUnknown deleters
	auto iunknown_deleter = [](IUnknown* p){p->Release();};
	auto imfmediabuffer_deleter = [](IMFMediaBuffer* p){p->Unlock(); p->Release();};

	// Extracts image meta informations from MediaType
	struct ImageHeader{
		DWORD width, height;
		GUID subtype;
	};
	static inline ImageHeader GetImageHeader(IMFMediaType* pmt){
		AM_MEDIA_TYPE* amt;
		if(SUCCEEDED(pmt->GetRepresentation(FORMAT_MFVideoFormat, reinterpret_cast<void**>(&amt)))){
			const MFVIDEOFORMAT* mvf = reinterpret_cast<MFVIDEOFORMAT*>(amt->pbFormat);
			ImageHeader result = {mvf->videoInfo.dwWidth, mvf->videoInfo.dwHeight, mvf->guidFormat};
			pmt->FreeRepresentation(FORMAT_MFVideoFormat, amt);
			return result;
		}
		return {0, 0, GUID_NULL};
	}

	// Generate available media type
	static inline HRESULT GetAvailableType(IMFMediaType* xput, DWORD dwTypeIndex, IMFMediaType **ppType){
		if(xput){
			if(dwTypeIndex > 0)
				return MF_E_NO_MORE_TYPES;
			*ppType = xput,
			xput->AddRef();
			return S_OK;
		}
		if(dwTypeIndex > 2)
			return MF_E_NO_MORE_TYPES;
		IMFMediaType* pmt;
		HRESULT status = MFCreateMediaType(&pmt);
		if(SUCCEEDED(status)){
			static const GUID subtypes[] = {MFVideoFormat_RGB24, MFVideoFormat_RGB32, MFVideoFormat_ARGB32};
			pmt->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video),
			pmt->SetGUID(MF_MT_SUBTYPE, subtypes[dwTypeIndex]);
		}
		return status;
	}

	// Check type for support
	static inline bool IsValidType(IMFMediaType* pType, IMFMediaType* xput){
		if(xput){
			DWORD flag;
			return SUCCEEDED(pType->IsEqual(xput, &flag)) && flag == S_OK;
		}
		GUID guid;
		MFVideoInterlaceMode interlace;
		return SUCCEEDED(pType->GetGUID(MF_MT_MAJOR_TYPE, &guid)) && guid == MFMediaType_Video &&
			SUCCEEDED(pType->GetGUID(MF_MT_SUBTYPE, &guid)) && (guid == MFVideoFormat_RGB24 || guid == MFVideoFormat_RGB32 || guid == MFVideoFormat_ARGB32) &&
			SUCCEEDED(pType->GetUINT32(MF_MT_INTERLACE_MODE, reinterpret_cast<UINT32*>(&interlace))) && interlace != MFVideoInterlace_Progressive;
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
			std::mutex userdata_mutex, transform_mutex;
			// Destruction of COM object by IUnknown instance->Release
			virtual ~MyFilter(){
				FilterBase::MediaF::deinit(static_cast<FilterBase::MediaF::IFilterConfig*>(this)),
				--instances_n;
			}
		public:
			// Any MyFilter instance is still locked?
			static bool active_instances(){return instances_n != 0;}
			// Ctors&assignment
			MyFilter() throw(std::string) : refcount(1), input(nullptr, iunknown_deleter), output(nullptr, iunknown_deleter), sample(nullptr, iunknown_deleter){
				++instances_n;
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
				this->userdata_mutex.lock();
				return &this->userdata;
			}
			void UnlockData() override{
				this->userdata_mutex.unlock();
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
				std::unique_lock<std::mutex>(this->transform_mutex);
				pStreamInfo->hnsMaxLatency = 0, // No time difference between input&output sample
				pStreamInfo->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE,
				pStreamInfo->cbMaxLookahead = 0, // No holding data to look
				pStreamInfo->cbAlignment = 0; // No special alignment requirements
				if(this->input){
					auto image_header = GetImageHeader(this->input.get());
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
				std::unique_lock<std::mutex>(this->transform_mutex);
				pStreamInfo->dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_FIXED_SAMPLE_SIZE,
				pStreamInfo->cbAlignment = 0;
				if(this->output){
					auto image_header = GetImageHeader(this->output.get());
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
				std::unique_lock<std::mutex>(this->transform_mutex);
				return GetAvailableType(this->output.get(), dwTypeIndex, ppType);
			}
			HRESULT STDMETHODCALLTYPE GetOutputAvailableType(DWORD dwOutputStreamID, DWORD dwTypeIndex, IMFMediaType **ppType) override{
				if(!ppType)
					return E_POINTER;
				if(dwOutputStreamID != 0)
					return MF_E_INVALIDSTREAMNUMBER;
				std::unique_lock<std::mutex>(this->transform_mutex);
				return GetAvailableType(this->input.get(), dwTypeIndex, ppType);
			}
			HRESULT STDMETHODCALLTYPE SetInputType(DWORD dwInputStreamID, IMFMediaType *pType, DWORD dwFlags) override{
				if(dwInputStreamID != 0)
					return MF_E_INVALIDSTREAMNUMBER;
				if(dwFlags & ~MFT_SET_TYPE_TEST_ONLY)
					return E_INVALIDARG;
				std::unique_lock<std::mutex>(this->transform_mutex);
				// Pending sample operation?
				if(this->sample)
					return MF_E_TRANSFORM_CANNOT_CHANGE_MEDIATYPE_WHILE_PROCESSING;
				// Valid type?
				if(!IsValidType(pType, this->output.get()))
					return MF_E_INVALIDMEDIATYPE;
				// Set when no test
				if(!(dwFlags & MFT_SET_TYPE_TEST_ONLY))
					this->input.reset(pType),
					pType->AddRef();
				return S_OK;
			}
			HRESULT STDMETHODCALLTYPE SetOutputType(DWORD dwOutputStreamID, IMFMediaType *pType, DWORD dwFlags) override{
				if(dwOutputStreamID != 0)
					return MF_E_INVALIDSTREAMNUMBER;
				if(dwFlags & ~MFT_SET_TYPE_TEST_ONLY)
					return E_INVALIDARG;
				std::unique_lock<std::mutex>(this->transform_mutex);
				// Pending sample operation?
				if(this->sample)
					return MF_E_TRANSFORM_CANNOT_CHANGE_MEDIATYPE_WHILE_PROCESSING;
				// Valid type?
				if(!IsValidType(pType, this->input.get()))
					return MF_E_INVALIDMEDIATYPE;
				// Set when no test
				if(!(dwFlags & MFT_SET_TYPE_TEST_ONLY))
					this->output.reset(pType),
					pType->AddRef();
				return S_OK;
			}
			HRESULT STDMETHODCALLTYPE GetInputCurrentType(DWORD dwInputStreamID, IMFMediaType **ppType) override{
				if(!ppType)
					return E_POINTER;
				if(dwInputStreamID != 0)
					return MF_E_INVALIDSTREAMNUMBER;
				std::unique_lock<std::mutex>(this->transform_mutex);
				if(!this->input)
					return MF_E_TRANSFORM_TYPE_NOT_SET;
				*ppType = this->input.get(),
				this->input->AddRef();
				return S_OK;
			}
			HRESULT STDMETHODCALLTYPE GetOutputCurrentType(DWORD dwOutputStreamID, IMFMediaType **ppType) override{
				if(!ppType)
					return E_POINTER;
				if(dwOutputStreamID != 0)
					return MF_E_INVALIDSTREAMNUMBER;
				std::unique_lock<std::mutex>(this->transform_mutex);
				if(!this->output)
					return MF_E_TRANSFORM_TYPE_NOT_SET;
				*ppType = this->output.get(),
				this->output->AddRef();
				return S_OK;
			}
			HRESULT STDMETHODCALLTYPE GetInputStatus(DWORD dwInputStreamID, DWORD *pdwFlags) override{
				if(!pdwFlags)
					return E_POINTER;
				if(dwInputStreamID != 0)
					return MF_E_INVALIDSTREAMNUMBER;
				std::unique_lock<std::mutex>(this->transform_mutex);
				*pdwFlags = !this->sample ? MFT_INPUT_STATUS_ACCEPT_DATA : 0x0;
				return S_OK;
			}
			HRESULT STDMETHODCALLTYPE GetOutputStatus(DWORD *pdwFlags) override{
				if(!pdwFlags)
					return E_POINTER;
				std::unique_lock<std::mutex>(this->transform_mutex);
				*pdwFlags = this->sample ? MFT_OUTPUT_STATUS_SAMPLE_READY : 0x0;
				return S_OK;
			}
			HRESULT STDMETHODCALLTYPE SetOutputBounds(LONGLONG, LONGLONG) override{
				return E_NOTIMPL; // Optional informations
			}
			HRESULT STDMETHODCALLTYPE ProcessEvent(DWORD, IMFMediaEvent *) override{
				return E_NOTIMPL; // Not event processing
			}
			HRESULT STDMETHODCALLTYPE ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR) override{
				std::unique_lock<std::mutex>(this->transform_mutex);
				switch(eMessage){
					case MFT_MESSAGE_COMMAND_FLUSH:
						this->sample.reset();
						break;
					case MFT_MESSAGE_SET_D3D_MANAGER:
						return E_NOTIMPL;
					case MFT_MESSAGE_COMMAND_DRAIN:
					case MFT_MESSAGE_NOTIFY_BEGIN_STREAMING:
						{
							if(!this->input)
								return MF_E_TRANSFORM_TYPE_NOT_SET;
							auto image_header = GetImageHeader(this->input.get());
							try{
								FilterBase::MediaF::start({static_cast<int>(image_header.width), static_cast<int>(image_header.height),
											image_header.subtype == MFVideoFormat_RGB24 ? FilterBase::ColorType::BGR : (image_header.subtype == MFVideoFormat_RGB32 ? FilterBase::ColorType::BGRX : FilterBase::ColorType::BGRA),
											0, 0}, static_cast<FilterBase::MediaF::IFilterConfig*>(this));
							}catch(std::string message){
								MessageBoxA(NULL, message.c_str(), FilterBase::get_name(), MB_OK);
								return E_UNEXPECTED;
							}
						}
						break;
					case MFT_MESSAGE_NOTIFY_END_STREAMING:
						FilterBase::MediaF::end(static_cast<FilterBase::MediaF::IFilterConfig*>(this));
						break;
					case MFT_MESSAGE_NOTIFY_END_OF_STREAM:
					case MFT_MESSAGE_NOTIFY_START_OF_STREAM:
					case MFT_MESSAGE_COMMAND_MARKER:
						break;
				}
				return S_OK;
			}
			HRESULT STDMETHODCALLTYPE ProcessInput(DWORD dwInputStreamID, IMFSample *pSample, DWORD dwFlags) override{
				if(!pSample)
					return E_POINTER;
				if(dwInputStreamID != 0)
					return MF_E_INVALIDSTREAMNUMBER;
				if(dwFlags != 0)
					return E_INVALIDARG;
				std::unique_lock<std::mutex>(this->transform_mutex);
				// All needed data available?
				if(!this->input || !this->output || this->sample)
					return MF_E_NOTACCEPTING;
				DWORD buffer_count;
				if(!SUCCEEDED(pSample->GetBufferCount(&buffer_count)) || buffer_count == 0)
					return E_FAIL;
				if(buffer_count > 1)
					return MF_E_SAMPLE_HAS_TOO_MANY_BUFFERS;
				// Save sample
				this->sample.reset(pSample),
				pSample->AddRef();
				return S_OK;
			}
			HRESULT STDMETHODCALLTYPE ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER *pOutputSamples, DWORD *pdwStatus) override{
				if(!pOutputSamples || !pdwStatus)
					return E_POINTER;
				if(dwFlags != 0 || cOutputBufferCount != 1 || !pOutputSamples[0].pSample)
					return E_INVALIDARG;
				std::unique_lock<std::mutex>(this->transform_mutex);
				// Sample to process?
				if(!this->sample)
					return MF_E_TRANSFORM_NEED_MORE_INPUT;
				// Get sample stride
				LONG stride;
				if(!SUCCEEDED(this->input->GetUINT32(MF_MT_DEFAULT_STRIDE, reinterpret_cast<UINT32*>(&stride)))){
					auto image_header = GetImageHeader(this->input.get());
					stride = image_header.width * (image_header.subtype == MFVideoFormat_RGB24 ? 3 : 4);
				}
				// Get sample data
				IMFMediaBuffer* buffer_in, *buffer_out;
				if(!SUCCEEDED(this->sample->ConvertToContiguousBuffer(&buffer_in)))
					return MF_E_NOT_AVAILABLE;
				std::unique_ptr<IMFMediaBuffer, std::function<void(IMFMediaBuffer*)>> buffer_in_storage(buffer_in, imfmediabuffer_deleter);
				if(!SUCCEEDED(pOutputSamples[0].pSample->ConvertToContiguousBuffer(&buffer_out)))
					return MF_E_SAMPLE_NOT_WRITABLE;
				std::unique_ptr<IMFMediaBuffer, std::function<void(IMFMediaBuffer*)>> buffer_out_storage(buffer_out, imfmediabuffer_deleter);
				BYTE* data_in, *data_out;
				DWORD current_data_length;
				buffer_in->Lock(&data_in, NULL, &current_data_length),
				buffer_out->Lock(&data_out, NULL, NULL);
				// Copy data to output
				std::copy(data_in, data_in+current_data_length, data_out),
				buffer_out->SetCurrentLength(current_data_length);
				// Copy sample time&duration to output (100ns units)
				LONGLONG time, duration;
				if(!SUCCEEDED(this->sample->GetSampleTime(&time)))
					return MF_E_NO_SAMPLE_TIMESTAMP;
				pOutputSamples[0].pSample->SetSampleTime(time);
				if(!SUCCEEDED(this->sample->GetSampleDuration(&duration)))
					return MF_E_NO_SAMPLE_DURATION;
				pOutputSamples[0].pSample->SetSampleDuration(duration);
				// Process sample data
				FilterBase::MediaF::filter_frame(data_out, stride, time / 10000, (time + duration) / 10000, static_cast<FilterBase::MediaF::IFilterConfig*>(this));
				// Set output status
				pOutputSamples[0].dwStatus = *pdwStatus = 0x0;
				// Release sample
				this->sample.reset();
				return S_OK;
			}
	};
	std::atomic_uint MyFilter::instances_n(0);	// ...but direct-initialization
}

// Filter registration to server
static inline std::wstring GenCLSIDKeyName(const GUID& guid){
	wchar_t guid_str[39];
	return StringFromGUID2(guid, guid_str, 39) ? std::wstring(L"CLSID\\") + guid_str : L"";
}
static std::wstring GetModuleName(){
	HMODULE module;
	wchar_t module_name[MAX_PATH];
	return GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCSTR>(&module), &module) && GetModuleFileNameW(module, module_name, MAX_PATH) ? module_name : L"";
}
static inline HRESULT RegCreateKeyExW_short(HKEY key, const wchar_t* name, HKEY* out){
	return __HRESULT_FROM_WIN32(
		RegCreateKeyExW(
			key,
			name,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			out,
			NULL
		)
	);
}
static inline HRESULT RegSetValueExW_short(HKEY key, const wchar_t* name, const wchar_t* value){
	LONG result = RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value), (wcslen(value)+1)*sizeof(wchar_t));
	return result == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(result);
}
STDAPI __declspec(dllexport) DllRegisterServer(){
	// Create CLSID for CoCreateInstance entry in registry
	HKEY key;
	HRESULT status = RegCreateKeyExW_short(HKEY_CLASSES_ROOT, GenCLSIDKeyName(*FilterBase::get_filter_guid()).c_str(), &key);
	if(SUCCEEDED(status)){
		status = RegSetValueExW_short(key, NULL, FilterBase::get_namew());
		if(SUCCEEDED(status)){
			HKEY subkey;
			status = RegCreateKeyExW_short(key, L"InprocServer32", &subkey);
			if(SUCCEEDED(status)){
				status = RegSetValueExW_short(subkey, NULL, GetModuleName().c_str());
				if(SUCCEEDED(status)){
					status = RegSetValueExW_short(subkey, L"ThreadingModel", L"Both");
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
	status = RegCreateKeyExW_short(HKEY_CLASSES_ROOT, GenCLSIDKeyName(*FilterBase::get_filter_config_guid()).c_str(), &key);
	if(SUCCEEDED(status))
		status = RegSetValueExW_short(key, NULL, (std::wstring(FilterBase::get_namew()) + L" configuration").c_str()),
		RegCloseKey(key);
	return status;
}

STDAPI __declspec(dllexport) DllUnregisterServer(){
	// Remove MFT enumeration entry in registry
	MFTUnregister(*FilterBase::get_filter_guid());
	// Remove CLSID for CoCreateInstance in registry
	RegDeleteTreeW(HKEY_CLASSES_ROOT, GenCLSIDKeyName(*FilterBase::get_filter_guid()).c_str());
	RegDeleteTreeW(HKEY_CLASSES_ROOT, GenCLSIDKeyName(*FilterBase::get_filter_config_guid()).c_str());
	return S_OK;
}

STDAPI __declspec(dllexport) DllCanUnloadNow(){
	return MediaF::MyFilter::active_instances() ? S_FALSE : S_OK;
}

STDAPI __declspec(dllexport) DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv){
	if(clsid == *FilterBase::get_filter_guid())
		try{
			IUnknown* inst = static_cast<IUnknown*>(static_cast<IMFTransform*>(new MediaF::MyFilter));
			HRESULT status = inst->QueryInterface(riid, ppv);
			inst->Release();
			return status;
		}catch(std::string message){
			MessageBoxA(NULL, message.c_str(), FilterBase::get_name(), MB_OK);
		}
	return E_NOINTERFACE;
}
