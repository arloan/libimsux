// special edition: timestamp with millisecond

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
#include <sys/types.h>
#include <sys/timeb.h>

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
static const char * severityName__[] = {
	"DEBUG",
	"TRACE",
	"INFORMATION",
	"WARNING",
	"ERROR",
	"FATAL",
};
static WORD severityAttr__[] = {
	0,
	0,
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,	// hi-lighted white
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,					// hi-lighted yellow
	FOREGROUND_RED | FOREGROUND_INTENSITY,										// hi-lighted red
	FOREGROUND_BLUE| FOREGROUND_INTENSITY,										// hi-lighted blue
};

class logger
{
public:
	logger(const std::string & logFile = "", int logLevel = LSV_INFO)
		: mFile(NULL)
		, mLevel(logLevel)
		, mVerbose(false)
	{
		if (logFile.length() != 0) SetLogName(logFile);

		CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
		mStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(mStdHandle, &csbi);
		mNormalTextAttribute = csbi.wAttributes;

		InitializeCriticalSection(&mLock);

		if (getenv("OPLOG_VERBOSE") != NULL) mVerbose = true;
	}

	~logger()
	{
		RestoreConsoleAttr();
		DeleteCriticalSection(&mLock);
	}

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

		int millis = 0;
		struct tm * t = CheckLogName(millis);
		if (t == NULL) return;

		char message[1024];
		vsprintf(message, format, vl);

		struct tm & tm= *t;
		xs tsp ("[%d-%02d-%02d %02d:%02d:%02d.%03d] "
			, tm.tm_year+1900
			, tm.tm_mon+1
			, tm.tm_mday
			, tm.tm_hour
			, tm.tm_min
			, tm.tm_sec
			, millis
		);

		CriticalSectionLocker lock(mLock);
		_ims_lock(CriticalSectionLocker, lock)
		{
			if (!raw) fprintf(mFile
				, "%s%s"
				, tsp.s
				, logSeverity__[severity]
			);

			fprintf(mFile, "%s%s", message, lineFeed);
			fflush(mFile);

			// handle stdout
			if (mVerbose || severity >= mLevel)
			{
				AdjustConsoleAttr(severity);
				printf("%s%s%s", tsp.s, message, lineFeed);
				if (lineEnd) RestoreConsoleAttr();
			}
		}
	}

	struct tm * CheckLogName(int & millis)
	{
		// hard-coded logging policy: one file per day
		struct _timeb tb;
		_ftime(&tb);
		millis = (int)tb.millitm;

		struct tm * t = localtime(&tb.time);
		xs targetFile ("%s.%d%02d%02d.log"
			, mBaseName.c_str()
			, t->tm_year+1900
			, t->tm_mon+1
			, t->tm_mday
		);

		if (mLogFile != targetFile.s)
		{
			xs logdir("%s\\%d-%02d", mLogDir.c_str(), t->tm_year+1900, t->tm_mon+1);
			if (!CreateDirectory(logdir, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
			{
				fprintf(stderr, "CreateDirectory('%s') failed.\n", logdir.s);
				return NULL;
			}

			mFile = fopen(xs("%s\\%s", logdir.s, targetFile.s), "a+t");
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

		return true;
	}

private:

	struct FileDtor {
		void operator () (FILE * f) { if (f) fclose(f); }
	};

	std::string mBaseName;
	std::string mLogDir;
	std::string mLogFile;

	bool mVerbose;
	int mLevel;
	scoped_ptr <FILE, FileDtor> mFile;
	CRITICAL_SECTION mLock;

	HANDLE mStdHandle;
	WORD mNormalTextAttribute;
};

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
