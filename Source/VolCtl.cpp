//
// Volume control using Windows Audio Services API (WASAPI).
//
// Internally we use char for characters and use multi-byte character
// conversion from UNICODE if required. This code should compile with
// either multi-byte or UNICODE set.
//
#include "VolCtl.h"
#include "MiscDef.h"
#include "WaLog.h"
#include "EndpointVolume.h"
#include "Functiondiscoverykeys_devpkey.h"
#include "Strsafe.h"

#define THIS_FILE	"VolCtl.cpp"

#ifdef _MSC_VER
#pragma warning(disable : 4244 4995)
#endif

#define CHECK(hr, retval, funcName) \
	if (FAILED(hr)) { \
		char tmpStr[128]; \
		GetAudioClientResultStr(hr, tmpStr, sizeof(tmpStr)); \
		_snprintf(errorText, sizeof(errorText), "%s returned %x (%s)", funcName, hr, tmpStr); \
		WA_LOG(1, (THIS_FILE, errorText)); \
		return retval; \
	}

#define CHECK_GOTO(hr, retval, funcName, label) \
	if (FAILED(hr)) { \
		char tmpStr[128]; \
		GetAudioClientResultStr(hr, tmpStr, sizeof(tmpStr)); \
		_snprintf(errorText, sizeof(errorText), "%s returned %x (%s)", funcName, hr, tmpStr); \
		WA_LOG(1, (THIS_FILE, errorText)); \
		status = retval; \
		goto label; \
	}

#define CHECK_HANDLE(h, retval, funcName) \
	if (h == NULL) { \
		DWORD err = GetLastError(); \
		char tmpStr[128]; \
		GetWindowsErrorStr(err, tmpStr, sizeof(tmpStr)); \
		_snprintf(errorText, sizeof(errorText), "%s error: %x (%s)", funcName, err, tmpStr); \
		WA_LOG(1, (THIS_FILE, errorText)); \
		return retval; \
	}

#define CHECK_HANDLE_GOTO(h, retval, funcName, label) \
	if (h == NULL) { \
		DWORD err = GetLastError(); \
		char tmpStr[128]; \
		GetWindowsErrorStr(err, tmpStr, sizeof(tmpStr)); \
		_snprintf(errorText, sizeof(errorText), "%s error: %x (%s)", funcName, err, tmpStr); \
		WA_LOG(1, (THIS_FILE, errorText)); \
		status = retval; \
		goto label; \
	}

#define CHECK_INIT() \
	if (!isInitialized) { \
		SetErrorText("device not initialized"); \
		return WAD_ERR_NOT_INITIALIZED; \
	}

#define CHECK_OPEN() \
	if (!isOpen) { \
		SetErrorText("device not open"); \
		return WAD_ERR_NOT_OPEN; \
	}

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID IID_IAudioEndpointVolume = __uuidof(IAudioEndpointVolume);
const IID IID_ISimpleAudioVolume = __uuidof(ISimpleAudioVolume);
const IID IID_IAudioClock = __uuidof(IAudioClock);

static bool GetWindowsErrorStr(HRESULT hr, char *errStr, size_t len)
{
	LPVOID lpMsgBuf = NULL;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		hr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
	if (!lpMsgBuf) {
		errStr[0] = 0;
		return false;
	}
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, 0, (WCHAR *) lpMsgBuf, wcslen((WCHAR *) lpMsgBuf), errStr, len, NULL, NULL);
#else
	strncpy(errStr, (char *) lpMsgBuf, len);
#endif
	// clobber trailing newline
	if (errStr[strlen(errStr)-1] == '\n')
		errStr[strlen(errStr)-1] = 0;
	// clobber trailing CR
	if (errStr[strlen(errStr)-1] == '\r')
		errStr[strlen(errStr)-1] = 0;
	LocalFree(lpMsgBuf);
	return true;
}

// do we really have to do this ourselves?
typedef struct {
	HRESULT hr;
	char *desc;
} ErrTabEntry;
#define MAKE_ENTRY(result) { result, #result } 
ErrTabEntry gErrTab[] = {
	MAKE_ENTRY(AUDCLNT_E_NOT_INITIALIZED),
	MAKE_ENTRY(AUDCLNT_E_ALREADY_INITIALIZED),
	MAKE_ENTRY(AUDCLNT_E_WRONG_ENDPOINT_TYPE),
	MAKE_ENTRY(AUDCLNT_E_DEVICE_INVALIDATED),
	MAKE_ENTRY(AUDCLNT_E_NOT_STOPPED),
	MAKE_ENTRY(AUDCLNT_E_BUFFER_TOO_LARGE),
	MAKE_ENTRY(AUDCLNT_E_OUT_OF_ORDER),
	MAKE_ENTRY(AUDCLNT_E_UNSUPPORTED_FORMAT),
	MAKE_ENTRY(AUDCLNT_E_INVALID_SIZE),
	MAKE_ENTRY(AUDCLNT_E_DEVICE_IN_USE),
	MAKE_ENTRY(AUDCLNT_E_BUFFER_OPERATION_PENDING),
	MAKE_ENTRY(AUDCLNT_E_THREAD_NOT_REGISTERED),
	MAKE_ENTRY(AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED),
	MAKE_ENTRY(AUDCLNT_E_ENDPOINT_CREATE_FAILED),
	MAKE_ENTRY(AUDCLNT_E_SERVICE_NOT_RUNNING),
	MAKE_ENTRY(AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED),
	MAKE_ENTRY(AUDCLNT_E_EXCLUSIVE_MODE_ONLY),
	MAKE_ENTRY(AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL),
	MAKE_ENTRY(AUDCLNT_E_EVENTHANDLE_NOT_SET),
	MAKE_ENTRY(AUDCLNT_E_INCORRECT_BUFFER_SIZE),
	MAKE_ENTRY(AUDCLNT_E_BUFFER_SIZE_ERROR),
	MAKE_ENTRY(AUDCLNT_E_CPUUSAGE_EXCEEDED),
#if MSC_VER >= 1600
	// these not defined in Win SDK v6.0a
	MAKE_ENTRY(AUDCLNT_E_BUFFER_ERROR),
	MAKE_ENTRY(AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED),
	MAKE_ENTRY(AUDCLNT_E_INVALID_DEVICE_PERIOD),
#endif
	MAKE_ENTRY(AUDCLNT_S_BUFFER_EMPTY),
	MAKE_ENTRY(AUDCLNT_S_THREAD_ALREADY_REGISTERED),
	MAKE_ENTRY(AUDCLNT_S_POSITION_STALLED)
};

static bool GetAudioClientResultStr(HRESULT hr, char *errStr, size_t len)
{
	size_t i;
	size_t n = sizeof(gErrTab) / sizeof(ErrTabEntry);
	for (i = 0; i < n; i++) {
		if (gErrTab[i].hr == hr) {
			strncpy(errStr, gErrTab[i].desc, len);
			return true;
		}
	}
	return GetWindowsErrorStr(hr, errStr, len);
}

//
//  Retrieves the device friendly name for a device, comverted to multi-byte characters.
//
bool VolCtl::GetDeviceName(IMMDevice *device, char *devName, size_t len)
{
    HRESULT hr;
    IPropertyStore *propertyStore;
    PROPVARIANT friendlyName;

	hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
    if (FAILED(hr))
    {
        _snprintf(errorText, sizeof(errorText), "OpenPropertyStore returned %x", hr);
		WA_LOG(1, (THIS_FILE, errorText));
        return false;
    }

    PropVariantInit(&friendlyName);
    hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
    SafeRelease(&propertyStore);
    if (FAILED(hr))
    {
        _snprintf(errorText, sizeof(errorText), "GetValue returned %x", hr);
		WA_LOG(1, (THIS_FILE, errorText));
        return false;
    }
	if (friendlyName.vt == VT_LPWSTR) {
		// copy wide to multi-byte
		WideCharToMultiByte(CP_ACP, 0, friendlyName.pwszVal, wcslen(friendlyName.pwszVal), devName, len, NULL, NULL);
	}
	else {
		// should never happen
		devName[0] = 0;
	}
    PropVariantClear(&friendlyName);
    return true;
}

//=============================================================================
//
// VolCtl
//

VolCtl::VolCtl(ERole _role) :
	role(_role)
{
	// discovery
	pEnumerator = NULL;
	numDev = 0;
	devTab = NULL;
	defaultInDev = -1;
	defaultOutDev = -1;
	// streaming
	pCaptureDevice = NULL;
	pRenderDevice = NULL;
	isInitialized = false;
	memset(errorText, 0, sizeof(errorText));
}

int VolCtl::Init()
{
	HRESULT hr;
	IMMDeviceCollection *pRenderCollection = NULL;
	IMMDeviceCollection *pCaptureCollection = NULL;
	UINT numRender = 0;
	UINT numCapture = 0;
	IMMDevice *pDevice;
	UINT i, index;
	LPWSTR defaultCaptureId = NULL;
	LPWSTR defaultRenderId = NULL;
	int status = 0;

	// initialize COM
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	// returns S_FALSE if already initialized
	CHECK_GOTO(hr, WAD_ERR_INTERNAL, "CoInitializeEx", Init_exit);
	// create the enumerator
	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	CHECK_GOTO(hr, WAD_ERR_INTERNAL, "CoCreateInstance", Init_exit);
	// get the default capture device id, if any
	hr = pEnumerator->GetDefaultAudioEndpoint(eCapture, role, &pDevice);
	if (hr != E_NOTFOUND) {
		CHECK_GOTO(hr, WAD_ERR_INTERNAL, "GetDefaultAudioEndpoint", Init_exit);
		hr = pDevice->GetId(&defaultCaptureId);
		CHECK_GOTO(hr, WAD_ERR_INTERNAL, "GetId", Init_exit);
	}
	// get the default render device id, if any
	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, role, &pDevice);
	if (hr != E_NOTFOUND) {
		CHECK_GOTO(hr, WAD_ERR_INTERNAL, "GetDefaultAudioEndpoint", Init_exit);
		hr = pDevice->GetId(&defaultRenderId);
		CHECK_GOTO(hr, WAD_ERR_INTERNAL, "GetId", Init_exit);
	}
	// enumerate capture devices
	hr = pEnumerator->EnumAudioEndpoints(
		eCapture, DEVICE_STATE_ACTIVE,
		&pCaptureCollection);
	CHECK_GOTO(hr, WAD_ERR_INTERNAL, "EnumAudioEndpoints", Init_exit);
	// get count
	hr = pCaptureCollection->GetCount(&numCapture);
	CHECK_GOTO(hr, WAD_ERR_INTERNAL, "GetCount", Init_exit);
	// enumerate render devices
	hr = pEnumerator->EnumAudioEndpoints(
		eRender, DEVICE_STATE_ACTIVE,
		&pRenderCollection);
	CHECK_GOTO(hr, WAD_ERR_INTERNAL, "EnumAudioEndpoints", Init_exit);
	// get count
	hr = pRenderCollection->GetCount(&numRender);
	CHECK_GOTO(hr, WAD_ERR_INTERNAL, "GetCount", Init_exit);
	numDev = numCapture + numRender;
	WA_LOG(2, (THIS_FILE, "%d devices:", numDev));
	// allocate device table
	devTab = (WadDevInfo *) calloc(numDev, sizeof(WadDevInfo));
	// build table starting with capture devices
	for (i = 0; i < numCapture; i++) {
		index = i;
		hr = pCaptureCollection->Item(i, &pDevice);
		CHECK_GOTO(hr, WAD_ERR_INTERNAL, "Item", Init_exit);
		// get the device ID
		hr = pDevice->GetId(&devTab[index].id);
		CHECK_GOTO(hr, WAD_ERR_INTERNAL, "GetId", Init_exit);
		// check if default
		if (defaultCaptureId && !wcscmp(defaultCaptureId, devTab[index].id))
			defaultInDev = index;
		// Just fill in a default number of channels.
		// It would be nice to know how many channels are supported!
		// It looks like you have to actually try opening the device
		// with different WAV parameters, is this really so?
		devTab[index].isInput = true;
		// get the name
		if (!GetDeviceName(pDevice, devTab[index].name, sizeof(devTab[index].name))) {
			status = WAD_ERR_INTERNAL;
			goto Init_exit;
		}
		// convert ID to multibyte
		WideCharToMultiByte(CP_ACP, 0, devTab[index].id, wcslen(devTab[index].id), devTab[index].devId, WAD_NAME_LEN, NULL, NULL);
		WA_LOG(2, (THIS_FILE, "%d: '%s' '%s' isInput %d", index, devTab[index].name, devTab[index].devId,
			devTab[index].isInput));
	}
	// render devices
	for (i = 0; i < numRender; i++) {
		index = i + numCapture;
		hr = pRenderCollection->Item(i, &pDevice);
		CHECK_GOTO(hr, WAD_ERR_INTERNAL, "Item", Init_exit);
		// get the device ID
		hr = pDevice->GetId(&devTab[index].id);
		CHECK_GOTO(hr, WAD_ERR_INTERNAL, "GetId", Init_exit);
		// check if default
		if (defaultRenderId && !wcscmp(defaultRenderId, devTab[index].id))
			defaultOutDev = index;
		// Just fill in a default number channels
		devTab[index].isInput = false;
		// get the name
		if (!GetDeviceName(pDevice, devTab[index].name, sizeof(devTab[index].name))) {
			status = WAD_ERR_INTERNAL;
			goto Init_exit;
		}
		// convert ID to multibyte
		WideCharToMultiByte(CP_ACP, 0, devTab[index].id, wcslen(devTab[index].id), devTab[index].devId, WAD_NAME_LEN, NULL, NULL);
		WA_LOG(2, (THIS_FILE, "%d: '%s' '%s' isInput %d", index, devTab[index].name, devTab[index].devId,
			devTab[index].isInput));
	}
	// adjust default devices
	if (defaultInDev == -1 && numCapture > 0)
		defaultInDev = 0;
	if (defaultOutDev == -1 && numRender > 0)
		defaultOutDev = numCapture;
Init_exit:
	// free default ids
	if (defaultCaptureId)
		CoTaskMemFree(defaultCaptureId);
	if (defaultRenderId)
		CoTaskMemFree(defaultRenderId);
	if (status != WAD_OK) {
		SafeRelease(&pEnumerator);
		isInitialized = false;
	}
	else
		isInitialized = true;
	return status;
}


VolCtl::~VolCtl()
{
	int i;
	if (isInitialized) {
		SafeRelease(&pEnumerator);
		if (devTab) {
			for (i = 0; i < numDev; i++)
				CoTaskMemFree(devTab[i].id);
			free(devTab);
			devTab = NULL;
		}
		numDev = 0;
		isInitialized = false;
	}
}

void VolCtl::SetErrorText(char *text)
{
	int i;
	for (i = 0; text[i] && i < sizeof(errorText) - 1; i++)
		errorText[i] = text[i];
	errorText[i] = 0;
}

const char* VolCtl::GetErrorText()
{
	return errorText;
}

int VolCtl::GetNumDevices()
{
	return numDev;
}

int VolCtl::GetDevInfo(int devId, WadDevInfo *pInfo)
{
	CHECK_INIT();
	if (devId >= 0 && devId < numDev) {
		// copy
		*pInfo = devTab[devId];
		return WAD_OK;
	}
	return WAD_ERR_INVALID_DEVICE;
}

int VolCtl::GetDefaultInDevIndex()
{
	return defaultInDev;
}

int VolCtl::GetDefaultOutDevIndex()
{
	return defaultOutDev;
}

// Lookup by id, return -1 if not found
int VolCtl::FindDevById(const char* devId)
{
	int devIndex = -1;
	for (int i = 0; i < numDev; i++) {
		if (!strcmp(devId, devTab[i].devId)) {
			devIndex = i;
			break;
		}
	}
	return devIndex;
}

// Lookup by name, return -1 if not found
int VolCtl::FindDevByName(const char* devName)
{
	int devIndex = -1;
	for (int i = 0; i < numDev; i++) {
		if (!strcmp(devName, devTab[i].name)) {
			devIndex = i;
			break;
		}
	}
	return devIndex;
}

int VolCtl::AccessVol(int devIndex, bool setVol, float *pVol)
{
	HRESULT hr;
	IMMDevice *pDevice;
	IAudioEndpointVolume *pAudioEndpointVolume;

	// check device
	if (devIndex < 0 || devIndex >= numDev) {
		_snprintf(errorText, sizeof(errorText), "AccessVol: device %d is not valid", devIndex);
		WA_LOG(1, (THIS_FILE, errorText));
		return WAD_ERR_INVALID_DEVICE;
	}
	// get device
	hr = pEnumerator->GetDevice(devTab[devIndex].id, &pDevice);
	CHECK(hr, WAD_ERR_INTERNAL, "GetDevice");
	hr = pDevice->Activate(IID_IAudioEndpointVolume, CLSCTX_ALL, NULL,
		(void **) &pAudioEndpointVolume);
	CHECK(hr, WAD_ERR_INTERNAL, "Activate");

	// set or get volume
	if (setVol) {
		hr = pAudioEndpointVolume->SetMasterVolumeLevelScalar(*pVol, NULL);
		CHECK(hr, WAD_ERR_INTERNAL, "SetMasterVolumeLevelScalar");
	}
	else {
		hr = pAudioEndpointVolume->GetMasterVolumeLevelScalar(pVol);
		CHECK(hr, WAD_ERR_INTERNAL, "GetMasterVolumeLevelScalar");
	}

	// clean up
	SafeRelease(&pAudioEndpointVolume);
	SafeRelease(&pDevice);
	return WAD_OK;
}


int VolCtl::AccessMute(int devIndex, bool setMute, bool *pMute)
{
	HRESULT hr;
	IMMDevice *pDevice;
	IAudioEndpointVolume *pAudioEndpointVolume;
	BOOL bMute;

	// check device
	if (devIndex < 0 || devIndex >= numDev) {
		_snprintf(errorText, sizeof(errorText), "AccessMute: device %d is not valid", devIndex);
		WA_LOG(1, (THIS_FILE, errorText));
		return WAD_ERR_INVALID_DEVICE;
	}
	// get device
	hr = pEnumerator->GetDevice(devTab[devIndex].id, &pDevice);
	CHECK(hr, WAD_ERR_INTERNAL, "GetDevice");
	hr = pDevice->Activate(IID_IAudioEndpointVolume, CLSCTX_ALL, NULL,
		(void **) &pAudioEndpointVolume);
	CHECK(hr, WAD_ERR_INTERNAL, "Activate");

	// set or get volume
	if (setMute) {
		bMute = *pMute;
		hr = pAudioEndpointVolume->SetMute(bMute, NULL);
		CHECK(hr, WAD_ERR_INTERNAL, "SetMute");
	}
	else {
		hr = pAudioEndpointVolume->GetMute(&bMute);
		CHECK(hr, WAD_ERR_INTERNAL, "GetMute");
		*pMute = bMute != 0;
	}

	// clean up
	SafeRelease(&pAudioEndpointVolume);
	SafeRelease(&pDevice);
	return WAD_OK;
}

int VolCtl::SetVol(int devIndex, float vol)
{
	WA_LOG(2, (THIS_FILE, "SetVol devIndex=%d vol=%f", devIndex, vol));
	return AccessVol(devIndex, true, &vol);
}

int VolCtl::GetVol(int devIndex, float *pVol)
{
	return AccessVol(devIndex, false, pVol);
}

int VolCtl::SetMute(int devIndex, bool mute)
{
	WA_LOG(2, (THIS_FILE, "SetMute devIndex=%d mute=%d", devIndex, mute));
	return AccessMute(devIndex, true, &mute);
}

int VolCtl::GetMute(int devIndex, bool *pMute)
{
	return AccessMute(devIndex, false, pMute);
}

