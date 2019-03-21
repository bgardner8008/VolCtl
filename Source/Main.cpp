//
// VolCtl
//
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "WaGetopt.h"
#include "WaLog.h"
#include "VolCtl.h"

void usage()
{
	fprintf(stderr,"Usage: VolCtl [options]\n");
	fprintf(stderr,"Options:\n");
	fprintf(stderr,"-l                list all active devices, each line has format:\n");
	fprintf(stderr, "                     'name' 'id' isInput\n");
	fprintf(stderr,"-I                list default input device for role\n");
	fprintf(stderr,"-O                list default output device for role\n");
	fprintf(stderr,"-i                select default input device (default is default output device)\n");
	fprintf(stderr,"-n deviceName     specify device name\n");
	fprintf(stderr,"-d deviceId       specify device ID\n");
	fprintf(stderr,"-v vol            set volume, float between 0 and 1\n");
	fprintf(stderr,"-V                get volume\n");
	fprintf(stderr,"-m 0/1            set mute state: 1 == mute, 0 == no mute\n");
	fprintf(stderr,"-M                get mute state\n");
	fprintf(stderr, "-r role          device role: 0 = console, 1 = multimedia (default), 2 = communication\n");
	fprintf(stderr, "-s sec           sleep, to test caller timeouts\n");
}

typedef enum {
	UNKNOWN = 0,
	LIST_DEVS,
	LIST_DEFAULT_IN,
	LIST_DEFAULT_OUT,
	SET_VOL,
	GET_VOL,
	SET_MUTE,
	GET_MUTE
} COMMAND;

typedef enum {
	CONSOLE = 0,
	MULTIMEDIA,
	COMMUNICATIONS,
	NUM_ROLES
} ROLE_ENUM;

int gCommand = COMMAND::UNKNOWN;
char *gDevName = NULL;
char *gDevId = NULL;
float gVol;		// vol argument
bool gMute;		// mute argument
bool gInput;	// select default input device
char* gLogFilename;	// log file name or null if none
int gRole;		// device role, affects defaults
int gSleep;		// sleep time in sec, for testing subprocess.run timeouts

ERole GetRole(int role)
{
	switch (role) {
	case ROLE_ENUM::CONSOLE:
		return ERole::eConsole;
	default:
	case ROLE_ENUM::MULTIMEDIA:
		return ERole::eMultimedia;
	case ROLE_ENUM::COMMUNICATIONS:
		return ERole::eCommunications;
	}
}

void main_error(char *fmt, ...)
{
	va_list args;

	va_start(args,fmt);
	vfprintf(stderr,fmt,args);
	va_end(args);
	if (fmt[strlen(fmt)-1] != '\n')
		fprintf(stderr,"\n");
	exit(1);
}

/*
 * Parse command line arguments.
 */
void parse_args(int argc, char **argv)
{
	int c;
	int nargs;
	
	while ((c = WaGetopt(argc, argv, "lIOin:d:v:Vm:MhL:r:s:")) > 0) {
		switch (c) {
		case 'l':
			gCommand = COMMAND::LIST_DEVS;
			break;
		case 'I':
			gCommand = COMMAND::LIST_DEFAULT_IN;
			break;
		case 'O':
			gCommand = COMMAND::LIST_DEFAULT_OUT;
			break;
		case 'L':
			gLogFilename = optarg;
			break;
		case 'r':
			gRole = atoi(optarg);
			if (gRole < 0 || gRole >= ROLE_ENUM::NUM_ROLES)
				main_error("illegal role %d", gRole);
		case 'i':
			gInput = true;
			break;
		case 'n':
			gDevName = optarg;
			break;
		case 'd':
			gDevId = optarg;
			break;
		case 'v':
			gCommand = COMMAND::SET_VOL;
			gVol = (float) atof(optarg);
			if (gVol < 0 || gVol > 1)
				main_error("illegal volume, should be float between 0 and 1\n");
			break;
		case 'V':
			gCommand = COMMAND::GET_VOL;
			break;
		case 'm':
			gCommand = COMMAND::SET_MUTE;
			gMute = (atoi(optarg) != 0);
			break;
		case 'M':
			gCommand = COMMAND::GET_MUTE;
			break;
		case 's':
			gSleep = atoi(optarg);
			break;
		case 'h':
			usage();
			exit(0);
			break;
		case '?':
			main_error("unknown option '%c'\n",optopt);
			break;
		case ':':
			main_error("missing argument for option '%c'\n",optopt);
			break;
		}
	}
	/*
	 * Remaining args at argv[optind]
	 */
	nargs = argc - optind;
	if (nargs > 0)
		main_error("extra arguments");
	if (gCommand == COMMAND::UNKNOWN)
		main_error("no command specified");
}

void PrintDev(WadDevInfo& info)
{
	printf("'%s' '%s' %d\n", info.name, info.devId, info.isInput);
}

int doCtl()
{
	WadDevInfo info;
	VolCtl volCtl(GetRole(gRole));
	int status = volCtl.Init();
	if (status != 0)
		main_error("error initializing: %s", volCtl.GetErrorText());

	// set default device
	int devIndex = (gInput ? volCtl.GetDefaultInDevIndex() : volCtl.GetDefaultOutDevIndex());

	// set device if devId or devName specified
	if (gDevId != NULL) {
		devIndex = volCtl.FindDevById(gDevId);
		if (devIndex == -1)
			main_error("can't find device ID '%s'", gDevId);
	}
	else if (gDevName != NULL) {
		devIndex = volCtl.FindDevByName(gDevName);
		if (devIndex == -1)
			main_error("can't find device name '%s'", gDevName);
	}
	int numDevs = volCtl.GetNumDevices();
	float vol = 0;
	bool mute = false;

	switch (gCommand) {
	case COMMAND::LIST_DEVS:
		for (int i = 0; i < numDevs; i++) {
			volCtl.GetDevInfo(i, &info);
			PrintDev(info);
		}
		break;
	case COMMAND::LIST_DEFAULT_IN:
		volCtl.GetDevInfo(volCtl.GetDefaultInDevIndex(), &info);
		PrintDev(info);
		break;
	case COMMAND::LIST_DEFAULT_OUT:
		volCtl.GetDevInfo(volCtl.GetDefaultOutDevIndex(), &info);
		PrintDev(info);
		break;
	case COMMAND::GET_VOL:
		volCtl.GetVol(devIndex, &vol);
		printf("%f\n", vol);
		break;
	case COMMAND::SET_VOL:
		volCtl.SetVol(devIndex, gVol);
		break;
	case COMMAND::GET_MUTE:
		volCtl.GetMute(devIndex, &mute);
		printf("%d\n", mute);
		break;
	case COMMAND::SET_MUTE:
		volCtl.SetMute(devIndex, gMute);
		break;
	}
	return 0;
}

int main(int argc, char* argv[])
{
	parse_args(argc, argv);
	if (gLogFilename) {
		WaLogOpen(gLogFilename, TRUE);
	}
	if (gSleep > 0)
		Sleep(gSleep * 1000);
	int status = doCtl();
	WaLogClose();
	return status;
}
