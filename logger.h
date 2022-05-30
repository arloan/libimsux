// logger.h: implement a inflexible file & console logging util.
// Version 2.1
// PENG Qiu, 2008-2009 (pengqiu0815@gmail.com)

#ifndef OPADMIN_LOG_H_INCLUDED__
#define OPADMIN_LOG_H_INCLUDED__

#if _MSC_VER > 800
#pragma once
#endif // _MSC_VER > 800
#if _MSC_VER == 1200
#pragma warning (disable: 4786)
#endif

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "xs.hxx"
#include "lock.hxx"
#include "auto.hxx"
#include "win32_error.hxx"

using namespace imsux;

#define LSV_DEBUG	0
#define LSV_TRACE	1
#define LSV_INFO	2
#define LSV_WARNING	3
#define LSV_ERROR	4
#define LSV_FATAL	5
static const char * logSeverity__[] = {
	"[DEBUG]",
	"",	// NORMAL TRACE LOG
	"", // IMPORTANT INFORMATION
	"[WARNING]",
	"[ERROR]",
	"[FATAL]",
};

#ifdef _WIN32
typedef WORD CONSOLE_ATTRIBUTE;
static CONSOLE_ATTRIBUTE severityAttr__[] = {
	0,
	0,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,	// hi-lighted white
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,					// hi-lighted yellow
	FOREGROUND_RED | FOREGROUND_INTENSITY,										// hi-lighted red
	FOREGROUND_BLUE| FOREGROUND_INTENSITY,										// hi-lighted blue
};
#else
typedef enum {
	defcolor = 0,
	black = 30,
	red,
	green,
	yellow,
	blue,
	purple,
	cyan,
	white,
} ansi_color;

typedef FILE * HANDLE;
typedef ansi_color CONSOLE_ATTRIBUTE;
static CONSOLE_ATTRIBUTE severityAttr__[] = {
	green,
	cyan,
	white,
	yellow,
	red,
	blue,
};

void SetConsoleTextAttribute(HANDLE console, CONSOLE_ATTRIBUTE attr);
int CreateDirectory(const char * dir, void * reserved)
{
	return !mkdir(dir, 0777);
}
inline int GetLastError() { return errno; }
int GetFullPathName(const char * path, int bufLen, char * buf, char ** fileName)
{
	if (path[0] == '/')
	{
		memcpy(buf, path, bufLen);
		buf[bufLen - 1] = '\0';
	}
	else
	{
		getcwd(buf, bufLen);
		strcat(buf, "/");
		strcat(buf, path);
	}
	char * fp = (char *)strrchr(buf, '/');
	* fileName = ++fp;
	return strlen(buf);
}
#define ERROR_ALREADY_EXISTS EEXIST
#define MAX_PATH 260
#endif//_WIN32

class logger
{
public:
	logger(const std::string & logFile = "", int logLevel = LSV_INFO)
		: mFile(NULL)
		, mLevel(logLevel)
		, mVerbose(false)
	{
		if (logFile.length() != 0) SetLogName(logFile);

		#ifdef _WIN32
		CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
		mStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(mStdHandle, &csbi);
		mNormalTextAttribute = csbi.wAttributes;
		#else
		mNormalTextAttribute = (CONSOLE_ATTRIBUTE)0;
		mStdHandle = stdout;
		#endif//_WIN32

		InitializeCriticalSection(&mLock);
		if (getenv("OPLOG_VERBOSE") != NULL) mVerbose = true;
	}

	~logger()
	{
		RestoreConsoleAttr();
		DeleteCriticalSection(&mLock);
	}

	#ifndef _WIN32
	static inline bool TermColorful()
	{
		static bool colorful = 
			strcmp(getenv("TERM"), "xterm") == 0
			||
			strcmp(getenv("TERM"), "rxvt" ) == 0
			||
			strcmp(getenv("TERM"), "linux") == 0;
		
		return colorful;
	}
	#endif //!_WIN32

	void AdjustConsoleAttr(int severity)
	{
		if (severity > LSV_TRACE)
		{
			SetConsoleTextAttribute(mStdHandle, severityAttr__[severity]);
		}
	}

	void RestoreConsoleAttr()
	{
		SetConsoleTextAttribute(mStdHandle, mNormalTextAttribute);
	}

	void Write(int severity, const char * format, ...)
	{
		va_list vl;
		va_start(vl, format);
		WriteV(0, 0, severity, format, vl);
		va_end(vl);
	}

	void WriteLine(int severity, const char * format, ...)
	{
		va_list vl;
		va_start(vl, format);
		WriteV(0, 1, severity, format, vl);
		va_end(vl);
	}

	void WriteRaw(int lineEnd, const char * format, ...)
	{
		va_list vl;
		va_start(vl, format);
		WriteV(1, lineEnd, 0, format, vl);
		va_end(vl);
	}

	void WriteV(int raw, int lineEnd, int severity, const char * format, va_list vl)
	{
		const char * lineFeed = "";
		std::string format_s = format;
		if (lineEnd && format_s[format_s.length() -1] != '\n') lineFeed = "\n";

		struct tm * t = CheckLogName();
		if (t == NULL) return;

		char message[1024];
		vsprintf(message, format, vl);

		struct tm & tm= *t;
		xs tsp ("[%d-%02d-%02d %02d:%02d:%02d]"
			, tm.tm_year+1900
			, tm.tm_mon+1
			, tm.tm_mday
			, tm.tm_hour
			, tm.tm_min
			, tm.tm_sec
		);

		CriticalSectionLocker lock(mLock);
		_ims_lock(CriticalSectionLocker, lock)
		{
			if (!raw) fprintf(mFile
				, "%s %s%s"
				, tsp.s
				, logSeverity__[severity]
				, logSeverity__[severity][0] ? " " : ""
			);

			fprintf(mFile, "%s%s", message, lineFeed);
			fflush(mFile);

			// handle stdout
			if (mVerbose || severity >= mLevel)
			{
				AdjustConsoleAttr(severity);
				printf("%s %s%s", tsp.s, message, lineFeed);
				if (lineEnd) RestoreConsoleAttr();
			}
		}
	}

	struct tm * CheckLogName()
	{
		// hard-coded logging policy: one file per day
		struct tm * t = NULL;
		time_t now = 0;

		time(&now);
		t = localtime(&now);

		xs targetFile ("%s.%d%02d%02d.log"
			, mBaseName.c_str()
			, t->tm_year+1900
			, t->tm_mon+1
			, t->tm_mday
		);

		if (mLogFile != targetFile.s)
		{
			xs logdir("%s/%d-%02d", mLogDir.c_str(), t->tm_year+1900, t->tm_mon+1);
			if (!CreateDirectory(logdir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
			{
				fprintf(stderr, "CreateDirectory('%s') failed (%u).\n", logdir.s, (unsigned)GetLastError());
				return NULL;
			}

			mFile = fopen(xs("%s/%s", logdir.s, targetFile.s), "a+t");
			if (mFile.get() == NULL)
			{
				fprintf(stderr, "cannot open log file '%s': %s"
					, targetFile.s
					, strerror(errno));
				return NULL;
			}

			mLogFile = targetFile.s;
		}

		if (mFile.get() == NULL)
		{
			// log file not available, return NULL to avoid recursive invoke
			return NULL;
		}

		return t;
	}

	bool SetLogName(const std::string & logName)
	{
		if (logName.length() == 0)
		{
			fprintf(stderr, "not a valid log file name.");
			return false;
		}

		char * fname = NULL;
		char full[MAX_PATH];
		GetFullPathName(logName.c_str(), sizeof(full)/sizeof(full[0]), full, &fname);

		mBaseName = fname;
		fname[0] = '\x0';
		mLogDir = full;

		printf("mBaseName: %s\n", mBaseName.c_str());
		printf("mLogDir: %s\n", mLogDir.c_str());

		return true;
	}

private:

	struct FileDtor {
		void operator () (FILE * f) { if (f) fclose(f); }
	};

	scoped_ptr <FILE, FileDtor> mFile;
	
	std::string mBaseName;
	std::string mLogDir;
	std::string mLogFile;

	int mLevel;
	bool mVerbose;

	CRITICAL_SECTION mLock;
	HANDLE mStdHandle;
	CONSOLE_ATTRIBUTE mNormalTextAttribute;
};

#ifndef _WIN32
void SetConsoleTextAttribute(HANDLE console, CONSOLE_ATTRIBUTE attr)
{
	if (logger::TermColorful()) fprintf(console, "\033[1;%dm", (int)attr);
}
#endif//!_WIN32

extern logger & logger__;
struct logutil
{
	logutil(int severity = LSV_TRACE, bool lineEnd = true, bool raw = false)
		: mSeverity(severity)
		, mLineEnd (lineEnd)
		, mRawOutput(raw)
	{
	}

	void operator () (const char * format, ...)
	{
		va_list vl;
		va_start(vl, format);
		logger__.WriteV(mRawOutput, mLineEnd, mSeverity, format, vl);
		va_end(vl);
	}

	int mSeverity;
	bool mLineEnd;
	bool mRawOutput;
};

#define DOLOG		logutil
#define TRACEENDL	do {logger__.WriteRaw(1, "");} while(0)
#define TRACE		logutil(LSV_TRACE, false)
#define TRACELINE	logutil()
#define TRACE_I		logutil(LSV_INFO, false)
#define TRACELINE_I	logutil(LSV_INFO)
#define TRACE_W		logutil(LSV_WARNING, false)
#define TRACELINE_W	logutil(LSV_WARNING)
#define TRACE_E		logutil(LSV_ERROR, false)
#define TRACELINE_E	logutil(LSV_ERROR)
#define TRACE_F		logutil(LSV_FATAL, false)
#define TRACELINE_F	logutil(LSV_FATAL)
#define TRACE_RAW	logutil(0, false, true)
#define TRACELINE_RAW logutil(0, true, true)

#endif//OPADMIN_LOG_H_INCLUDED__
