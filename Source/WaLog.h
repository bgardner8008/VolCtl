/** Logging functions

Simple log functions accessed via macro, e.g.:

WA_LOG(2, (__FILE__, "got value %d and value %d", a, b));

First argument is level:
1 = critical error
2 = error or important message
3 = information
4 = more detail
5 = fine detail, e.g., inside loops, for debugging only

Second argument is (source, fmt, args...) in printf style, and
MUST be enclosed in parentheses to be treated as a single macro argument.

To disable log compilation in an individual module:
#define WA_LOG(level, arg)

At runtime, logging can be disabled by no opening log, or
setting log level to 0. Note that the log level comparison
happens in the WA_LOG macro, so it's pretty lightweight.

Note that there is no thread synchronization, and it might not work
as expected for DLLs or plugins where globals are shared between instances,
but in practice it works fine, likely due to multithreaded CRT libs.

It's a good idea to enclose WA_LOG in curly braces if used as clause of an if
statement, otherwise any else clause will generate compile error.

@file WaLog.h
*/
#ifndef _WA_LOG_H
#define _WA_LOG_H

#include <stdio.h>
#include <stdarg.h>

//! level must be 1...5 or expect errors.
//! The semicolon after WA_LOG is not necessary, leads to null statement.
#define WA_LOG(level, arg) if (level <= gWaLogLevel) { WaLogMacro_##level(arg); }

// These macros needed to propagate level, to route into another logging system
#define WaLogMacro_1(arg) WaLog1 arg
#define WaLogMacro_2(arg) WaLog2 arg
#define WaLogMacro_3(arg) WaLog3 arg
#define WaLogMacro_4(arg) WaLog4 arg
#define WaLogMacro_5(arg) WaLog5 arg

//! Log decoration flags
enum {
	WA_LOG_DATE = 1,		//!< output date
	WA_LOG_TIME = 2,		//!< output time
	WA_LOG_SOURCE = 4,		//!< output source
	WA_LOG_NEWLINE = 8,		//!< output trailing newline automatically
	WA_LOG_INITNL = 16,		//!< output leading newline before time/date
	WA_LOG_INITSPACE = 32,	//!< output leading space
	WA_LOG_THREADID = 64,	//!< output quasi-unique thread ID
	WA_LOG_THREADSWITCH = 128,	//!< output ! when thread switches
	WA_LOG_STRIPCR = 256	//!< remove \r chars
};

//! Prototype logging function, to redirect logging
typedef void WaLogFn(void *arg, int level, const char *buf);

#ifdef __cplusplus
extern "C" {
#endif

//! Generate log message and pass to WaLogMessage
void WaLog(const char *source, int level, const char *fmt, va_list args);
// WaLogN functions are called by WA_LOG macro
//! Log level 1
void WaLog1(const char *source, const char *fmt, ...);
//! Log level 2
void WaLog2(const char *source, const char *fmt, ...);
//! Log level 3
void WaLog3(const char *source, const char *fmt, ...);
//! Log level 4
void WaLog4(const char *source, const char *fmt, ...);
//! Log level 5
void WaLog5(const char *source, const char *fmt, ...);
//! Set the decoration flags
void WaLogSetDecor(int flags);
//! Get the decoration flags
int WaLogGetDecor();
//! Set function to receive log messages
void WaLogSetLogFn(WaLogFn *fn, void *arg);
//! Directly output to log stream without decoration
void WaLogMessage(int level, const char *buf);
//! Get logging level
int WaLogGetLevel();
//! Set logging level, 0 (disable) thru 5
void WaLogSetLevel(int level);
// return flush level
int WaLogGetFlushLevel();
// set level at which log messages will cause log file flush
void WaLogSetFlushLevel(int level);
//! Log to specified file
int WaLogOpen(const char *file, int append);
//! Truncate log
int WaLogTruncate(const char *file, int maxSize);
//! Print time and date and log level
void WaLogPrintHeader();
//! Close log file
void WaLogClose();
//! Log to stdout
void WaLogOpenStdout();
//! Set whether to also log to debugger
void WaLogToDebugger(int flag);
//! Get thread ID used by logger, not unique on Posix
unsigned int WaLogGetThreadID();

extern int gWaLogLevel;
extern FILE *gWaLogFp;

#ifdef __cplusplus
}
#endif

#endif
