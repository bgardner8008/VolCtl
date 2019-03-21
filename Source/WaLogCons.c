//
// Log functions
//
// Note: the comparison with level happens in the WA_LOG macro
// so that WA_LOG is low overhead.
//
// This is a "consolidated version" with all the dependencies inline below
// so you only need "WaLog.h" and "WaLogCons.c" for quickly adding to a
// project for debuggin.
//
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

//=============================================================================
//#include "WaPlatform.h"
//===========================================================
/*
Platform definitions for plug-ins.
*/
#ifndef WA_PLATFORM_H
#define WA_PLATFORM_H

/*
Platform OS definitions
*/
#if (defined (_WIN32) || defined (_WIN64))
#define WA_WINDOWS	1
#elif defined (LINUX) || defined (__linux__) || defined (__unix) || defined(__unix__)
#define	WA_LINUX	1
#elif defined(macintosh)
#define	WA_MACOS9	1
#define WA_MAC		1
#define WA_MACINTOSH		1
#elif defined (__APPLE__) && defined(__MACH__)
#define WA_MACOSX	1
#define WA_MAC		1
#define WA_MACINTOSH		1
#else
#error unknown OS platform
#endif

/*
64-bit versus 32-bit build.
*/
#if WA_WINDOWS
#ifdef _WIN64
#define WA_64BIT	1
#endif
#elif WA_MAC
#ifdef __LP64__
#define WA_64BIT	1
#endif
#elif WA_LINUX
// this only works for intel
#if __x86_64__
#define WA_64BIT	1
#endif
#else
#error unkown target platform
#endif
#ifndef WA_64BIT
#define WA_32BIT	1
#endif

#endif  // WA_PLATFORM_H

//=============================================================================
//#include "MiscDef.h"
/*
* Miscellaneous and often needed preprocessor definitions. Formerly
* defined in "misc.h".
*
* Bill Gardner, March, 1999.
*/
#ifndef _MISC_DEF_H
#define _MISC_DEF_H

#ifndef FALSE
#define FALSE	0
#define TRUE	(!FALSE)
#endif

/*
* C++ really complains about pointer types.
*/
#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef MIN
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a)		(((a) < 0) ? -(a) : (a))
#endif

#ifndef NELEMS
#define NELEMS(array) (sizeof(array) / sizeof(array[0]))
#define NELEM(array) (sizeof(array) / sizeof(array[0]))
#endif

// suppress unused warning, needs trailing semicolon
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#ifndef WA_UNUSED
#define WA_UNUSED(x) (void)(x)
#endif

// This is a simple way to do compile time assertions. If the test is not true,
// then the array will have a negative number of elements and the compiler will
// produce an error.
#define WA_STATIC_ASSERT(test) typedef char assertion_on_mystruct[( !!(test) )*2-1 ]

#endif

//=============================================================================
//#include "WaTime.h"

typedef struct {
	int hour;		// 0 - 23
	int min;		// 0 - 59
	int sec;		// 0 - 59
	int msec;		// 0 - 999
	int year;		// calendar year
	int weekday;	// 0 - 6, 0 = sunday
	int day;		// day of month, 1 - 31
	int month;		// 1 - 12
} WaParsedTime;

#ifdef __cplusplus
extern "C" {
#endif

	void WaGetParsedLocalTime(WaParsedTime *pt);
	char *WaAsciiDateEuro(WaParsedTime *pt);
	char *WaAsciiDate(WaParsedTime *pt);
	char *WaAsciiTime(WaParsedTime *pt);

#ifdef __cplusplus
}
#endif

// WaTime.c

#if WA_WINDOWS
#include <windows.h>
#else
#include <time.h>
#endif

void WaGetParsedLocalTime(WaParsedTime *pt)
{
#if WA_WINDOWS
	SYSTEMTIME st;
	GetLocalTime(&st);
	pt->hour = st.wHour;
	pt->min = st.wMinute;
	pt->sec = st.wSecond;
	pt->msec = st.wMilliseconds;
	pt->year = st.wYear;
	pt->month = st.wMonth;
	pt->weekday = st.wDayOfWeek;
	pt->day = st.wDay;
#else
	struct tm *t;
	time_t timeval;
	timeval = time(NULL);
	t = localtime(&timeval);
	pt->hour = t->tm_hour;
	pt->min = t->tm_min;
	pt->sec = t->tm_sec;
	pt->msec = 0;
	pt->year = t->tm_year + 1900;
    pt->month = t->tm_mon + 1;
	pt->weekday = t->tm_wday;
	pt->day = t->tm_mday;
#endif
}

// format dd-mm-yyyy
char *WaAsciiDateEuro(WaParsedTime *pt)
{
	static char dateBuf[32];
	sprintf(dateBuf, "%02d-%02d-%04d ", pt->day, pt->month, pt->year);
	return dateBuf;
}

// format mm-dd-yyyy
char *WaAsciiDate(WaParsedTime *pt)
{
	static char dateBuf[32];
	sprintf(dateBuf, "%02d-%02d-%04d ", pt->month, pt->day, pt->year);
	return dateBuf;
}

char *WaAsciiTime(WaParsedTime *pt)
{
	static char timeBuf[32];
	sprintf(timeBuf, "%02d:%02d:%02d.%03d ", pt->hour, pt->min, pt->sec, pt->msec);
	return timeBuf;
}
//=============================================================================

#include "WaLog.h"

#if WA_WINDOWS
#include <windows.h>
#define snprintf _snprintf
#pragma warning(disable: 4706)
#else
#include <pthread.h>
#endif

int gWaLogLevel = 3;	// current log level
int gWaLogFlushLevel = 2;	// flush log at this level or lower
int gWaLogFlags = WA_LOG_TIME | WA_LOG_SOURCE | WA_LOG_NEWLINE;	// decoration settings
WaLogFn *gWaLogFn;	// function to call to display or write log message
void *gWaLogFnArg;	// argument to log function
FILE *gWaLogFp;		// file pointer
unsigned int gWaLogLastThreadID;	// last thread ID
int gWaLogToDebugger;	// T/F if also send to debugger

unsigned int WaLogGetThreadID()
{
#if WA_WINDOWS
	return GetCurrentThreadId();
#else
    pthread_t ptid = pthread_self();
    unsigned int threadId = 0;
	unsigned int *p = (unsigned int *) &ptid;
	// compute a simple hash
	for (int i = 0; i < sizeof(ptid) / sizeof(unsigned int); i++)
		threadId |= p[i];
	return threadId;
#endif
}

void WaLogSetDecor(int flags)
{
	gWaLogFlags = flags;
}

int WaLogGetDecor()
{
	return gWaLogFlags;
}

/*
 This is the default log handler.
 */
void WaLogToFileFn(void *arg, int level, const char *buf)
{
    WA_UNUSED(level);
    FILE *fp = (FILE *) arg;
    fputs(buf, fp);
}

void WaLogSetLogFn(WaLogFn *fn, void *arg)
{
	gWaLogFn = fn;
	gWaLogFnArg = arg;
}

void WaLogMessageToDebugger(const char *buf)
{
#if WA_WINDOWS
	OutputDebugStringA(buf);
#elif WA_MAC
	fputs(buf, stderr);
#endif
}

static void WaLogMessageInternal(int level, const char *buf)
{
	if (gWaLogFn)
		gWaLogFn(gWaLogFnArg, level, buf);
	if (gWaLogToDebugger)
		WaLogMessageToDebugger(buf);
	// For critical errors, when using stdio, flush output before we crash.
    // This only works if using the built-in WaLogToFileFn.
	if (level <= gWaLogFlushLevel && gWaLogFn == WaLogToFileFn && gWaLogFp)
		fflush(gWaLogFp);
}

// external entry point, must check level
void WaLogMessage(int level, const char *buf)
{
	if (level <= gWaLogLevel) {
		WaLogMessageInternal(level, buf);
	}
}

static void removeChar(char *str, int c)
{
	char *s1, *s2;
	s1 = s2 = str;
	while (*str = *s1++)
		if (*str != c) str++;
}

// maximum length of a log message
#define WA_LOG_LEN	1024

void WaLog(const char *source, int level, const char *fmt, va_list args)
{
	char buf[WA_LOG_LEN + 1];
	size_t i, n, len;
	int vn;
	const char *s;
	WaParsedTime pt;
	unsigned int threadID;

	// skip if nothing set up to receive log messages
	if (!gWaLogFn && !gWaLogToDebugger)
		return;

	i = 0;
	len = WA_LOG_LEN;
	threadID = WaLogGetThreadID();

	// initial newline
	if (gWaLogFlags & WA_LOG_INITNL) {
		buf[i++] = '\n';
		len--;
	}

	// initial space, for matching with pj_log
	if (gWaLogFlags & WA_LOG_INITSPACE) {
		buf[i++] = ' ';
		len--;
	}

	// get date/time if needed
	if (gWaLogFlags & (WA_LOG_DATE | WA_LOG_TIME))
		WaGetParsedLocalTime(&pt);

	if (gWaLogFlags & WA_LOG_DATE) {
		s = WaAsciiDate(&pt);
		strncpy(buf + i, s, len);
		n = MIN(len, strlen(s));
		i += n;
		len -= n;
	}
	if (gWaLogFlags & WA_LOG_TIME) {
		s = WaAsciiTime(&pt);
		strncpy(buf + i, s, len);
		n = MIN(len, strlen(s));
		i += n;
		len -= n;
	}
	if (gWaLogFlags & WA_LOG_THREADID) {
		vn = snprintf(buf + i, len, "%08x ", threadID);
		i += vn;
		len -= vn;
	}
	if (gWaLogFlags & WA_LOG_THREADSWITCH) {
		if (len > 0) {
			buf[i++] = (threadID == gWaLogLastThreadID) ? ' ' : '!';
			len--;
		}
	}
	if (gWaLogFlags & WA_LOG_SOURCE) {
		strncpy(buf + i, source, len);
		n = MIN(len, strlen(source));
		i += n;
		len -= n;
		if (len > 0) {
			buf[i++] = ' ';
			len--;
		}
	}
	gWaLogLastThreadID = threadID;
	// sprintf message
	vn = vsnprintf(buf + i, len, fmt, args);
	if (vn >= 0 && vn < (int) len) {
		// sprintf success
		i += vn;
		len -= vn;
		// check for trailing newline and optionally add one
		if ((gWaLogFlags & WA_LOG_NEWLINE) && fmt[strlen(fmt)-1] != '\n' && len > 0) {
			buf[i++] = '\n';
			len--;
		}
	}
	else {
		// sprintf failed, probably too big
		vn = snprintf(buf + i, len, "*log message too large*\n");
		if (vn > 0 && vn < (int) len) {
			i += vn;
			len -= vn;
		}
	}
	// force null terminate, the extra +1 in decl ensures room in pathological cases
	buf[i] = 0;
	// optionally remove \r
	if (gWaLogFlags & WA_LOG_STRIPCR)
		removeChar(buf, '\r');
	// dispatch
	WaLogMessageInternal(level, buf);
}

//
// These functions are needed to access level so we can pass the level
// to the log function for routing into another logging system, such
// as pj_log.
//
void WaLog0(const char *source, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WaLog(source, 0, fmt, args);
	va_end(args);
}

void WaLog1(const char *source, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WaLog(source, 1, fmt, args);
	va_end(args);
}

void WaLog2(const char *source, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WaLog(source, 2, fmt, args);
	va_end(args);
}

void WaLog3(const char *source, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WaLog(source, 3, fmt, args);
	va_end(args);
}

void WaLog4(const char *source, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WaLog(source, 4, fmt, args);
	va_end(args);
}

void WaLog5(const char *source, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	WaLog(source, 5, fmt, args);
	va_end(args);
}

int WaLogOpen(const char *file, int append)
{
	gWaLogFp = fopen(file, append ? "a" : "w");
	if (gWaLogFp != NULL) {
		WaLogSetLogFn(WaLogToFileFn, gWaLogFp);
		return TRUE;
	}
	return FALSE;
}

int WaLogTruncate(const char *file, int maxSize)
{
	FILE *fp;
	size_t len;
	char *p;
	char *s;
	size_t nr;

	// first get size of file, can't use fseek because of newline translation
	if ((fp = fopen(file, "r")) == NULL)
		return FALSE;
	len = 0;
	while (fgetc(fp) != EOF)
		len++;
	fseek(fp, 0, SEEK_SET);
	if (len <= (size_t) maxSize) {
		// no need to truncate
		fclose(fp);
		return TRUE;
	}
	// read contents into memory - it would be better to use temp file.
	p = (char *) malloc(len);
	nr = fread(p, sizeof(char), len, fp);
	if (nr != len) {
		fclose(fp);
		free(p);
		return FALSE;
	}
	fclose(fp);
	// re-create file
	if ((fp = fopen(file, "w")) == NULL) {
		free(p);
		return FALSE;
	}
	// truncate to maxSize/2 to prevent truncation each time
	s = p + len - maxSize / 2;
	// advance to newline
	while (s < p + len) {
		if (*s == '\n') {
			s++;
			break;
		}
		s++;
	}
	fprintf(fp, "truncated...\n");
	// now write the end of buffer back to file
	fwrite(s, sizeof(char), p + len - s, fp);
	fclose(fp);
	free(p);
	return TRUE;
}

void WaLogPrintHeader()
{
	int decor = WaLogGetDecor();
	WaLogSetDecor(WA_LOG_INITNL | WA_LOG_DATE | WA_LOG_TIME | WA_LOG_SOURCE | WA_LOG_NEWLINE);
	WaLog0("WaLog.c", "============================== Log started at level %d", WaLogGetLevel());
	WaLogSetDecor(decor);
}

void WaLogOpenStdout()
{
	gWaLogFp = stdout;
	WaLogSetLogFn(WaLogToFileFn, gWaLogFp);
}

void WaLogClose()
{
	if (gWaLogFp) {
		if (gWaLogFp != stdout)
			fclose(gWaLogFp);
		gWaLogFp = NULL;
	}
    gWaLogFn = NULL;
}

int WaLogGetFlushLevel()
{
	return gWaLogFlushLevel;
}

void WaLogSetFlushLevel(int level)
{
	if (level < 0)
		level = 0;
	else if (level > 5)
		level = 5;
	gWaLogFlushLevel = level;
}

int WaLogGetLevel()
{
	return gWaLogLevel;
}

void WaLogSetLevel(int level)
{
	if (level < 0)
		level = 0;
	else if (level > 5)
		level = 5;
	gWaLogLevel = level;
}

void WaLogToDebugger(int flag)
{
	gWaLogToDebugger = flag;
}
