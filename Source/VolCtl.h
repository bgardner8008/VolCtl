
#ifndef _VOL_CTL_H
#define _VOL_CTL_H

#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

enum WadStatus {
	WAD_OK = 0,					//!< success
	WAD_ERR_INTERNAL,			//!< internal error, see errorText
	WAD_ERR_UNSUPPORTED,		//!< unsupported feature
	WAD_ERR_INVALID_ARG,		//!< invalid argument
	WAD_ERR_NOT_INITIALIZED,	//!< device not initialized
	WAD_ERR_NOT_OPEN,			//!< device not open
	WAD_ERR_INVALID_DEVICE,		//!< invalid device
	// Use GetClosestFormat() to return suggestions for unsupported formats
	WAD_ERR_IN_FORMAT,			//!< unsupported input format
	WAD_ERR_OUT_FORMAT,			//!< unsupported output format
	// more detail...
	WAD_ERR_IN_SAMPRATE,		//!< unsupported input sampling rate
	WAD_ERR_OUT_SAMPRATE,		//!< unsupported output sampling rate
	WAD_ERR_IN_NUMCHAN,			//!< unsupported number input channels
	WAD_ERR_OUT_NUMCHAN,		//!< unsupported number output channels
	WAD_ERR_IN_SAMPFORMAT,		//!< unsupported input sample format
	WAD_ERR_OUT_SAMPFORMAT,		//!< unsupported output sample format
	WAD_ERR_IN_FRAMESPERBUF,	//!< unsupported buffer length
	WAD_ERR_OUT_FRAMESPERBUF,	//!< unsupported buffer length
};

#define WAD_NAME_LEN	256

/** Device information structure
*/
typedef struct {
	bool isInput;	//! T/F if input device
	char name[WAD_NAME_LEN];	//!< device name
	LPWSTR id;			//!< id from GetId, allocated
	char devId[WAD_NAME_LEN];	//! id converted to multichar
} WadDevInfo;

class VolCtl {
protected:
	// device discovery
	IMMDeviceEnumerator *pEnumerator;	//!< device enumerator
	int numDev;					//!< number devices in device table
	WadDevInfo *devTab;		//!< device table, allocated
	//! Get the device name as C string
	bool GetDeviceName(IMMDevice *device, char *devName, size_t len);
	int defaultInDev;			//!< index of default input device, -1 if none
	int defaultOutDev;			//!< index of default output device, -1 if none
	// streaming
	IMMDevice *pCaptureDevice;	//!< capture device, or NULL
	IMMDevice *pRenderDevice;	//!< render device, or NULL
	//! volume control
	int AccessVol(int devIndex, bool setVol, float *pVol);
	//! mute control
	int AccessMute(int devIndex, bool setMute, bool *pMute);
	char errorText[256];
	ERole role;
	bool isInitialized;

public:
	//! Creator
	VolCtl(ERole role = eCommunications);
	//! Destructor
	~VolCtl();

	int Init();
	void SetErrorText(char *text);
	const char* GetErrorText();

	int GetNumDevices();
	int GetDefaultInDevIndex();
	int GetDefaultOutDevIndex();
	int FindDevById(const char* devId);
	int FindDevByName(const char* devName);
	// these return errors
	int GetDevInfo(int devIndex, WadDevInfo *pInfo);
	int SetVol(int devIndex, float vol);
	int GetVol(int devIndex, float *pVol);
	int SetMute(int devIndex, bool mute);
	int GetMute(int devIndex, bool *pMute);
};

#endif
