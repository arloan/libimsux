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
#include <strings.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdexcept>

#include "xs.hxx"
#include "lock.hxx"
#include "auto.hxx"
#include "errno_error.hxx"
#include "win32_error.hxx"

using namespace imsux;

#define LSV_DEBUG	0
#define LSV_TRACE	1
#define LSV_INFO	2
#define LSV_WARNING	3
#define LSV_ERROR	4
#define LSV_FATAL	5
#define LSV_UNKNOWN 6
#define LSV_MAX     LSV_UNKNOWN
static const char * logSeverity__[] = {
    "[DEBUG]",
    "",
    "[INFO]",
    "[WARNING]",
    "[ERROR]",
    "[FATAL]",
    "[UNKNOWN]",
};

#ifdef _WIN32
typedef WORD CONSOLE_ATTRIBUTE;
static CONSOLE_ATTRIBUTE severityAttr__[] = {
    0,
    0,
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,	// hi-lighted white
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,                   // hi-lighted yellow
    FOREGROUND_RED | FOREGROUND_INTENSITY,                                      // hi-lighted red
    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_RED,       // white on red BG
    FOREGROUND_BLUE| FOREGROUND_INTENSITY,                                      // hi-lighted blue
};
#else
enum ansi_color {
    reset = 0,
    black = 30,
    red,
    green,
    yellow,
    blue,
    purple,
    cyan,
    white,
};

typedef FILE * HANDLE;
typedef const char * CONSOLE_ATTRIBUTE;
static CONSOLE_ATTRIBUTE severityAttr__[] = {
    "37;2",
    "37",
    "37;1",
    "33;1",
    "31;1",
    "37;41;1",
    "34;1",
};
static CONSOLE_ATTRIBUTE resetAttribute__ = "0";

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
    enum rotation {
        as_is,
        timestamped,
        fixed_size,
        daily,
        weekly,
        monthly,
        anually,
    };

public:
    logger() : mFile(NULL)
    {
        mTermColorful = TermColorful();

        #ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
        mStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleScreenBufferInfo(mStdHandle, &csbi);
        mNormalTextAttribute = csbi.wAttributes;
        #else
        mNormalTextAttribute = resetAttribute__;
        mStdHandle = stdout;
        #endif//_WIN32

        InitializeCriticalSection(&mLock);
    }

    ~logger()
    {
        RestoreConsoleAttr();
        DeleteCriticalSection(&mLock);
    }

    void setup(int logLevel = LSV_TRACE, const std::string & logFile = "", rotation policy = as_is)
    {
        if (logFile.length() != 0) SetLogName(logFile);
        if (policy == fixed_size) throw std::invalid_argument("rotation policy `fixed_size` not implemented");

        mLevel = logLevel;
        mPolicy = policy;
        char * overrideSev = getenv("IMSLOG_SEVERITY");
        if (overrideSev != NULL)
        {
            int sev = atoi(overrideSev);
            if (sev >= 0 && sev <= LSV_MAX)
            {
                printf("Obeyed severity value %d from environment.\n", sev);
                mLevel = sev;
            }
        }
    }

    #ifndef _WIN32
    static inline bool TermColorful()
    {
        static bool colorful = getenv("TERM")
            &&(strncmp(getenv("TERM"), "xterm", 5) == 0
            || strcmp (getenv("TERM"), "rxvt" ) == 0
            || strcmp (getenv("TERM"), "linux") == 0);
        
        return colorful;
    }
    #endif //!_WIN32

    void AdjustConsoleAttr(int severity)
    {
        SetConsoleTextAttribute(mStdHandle, severityAttr__[severity]);
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

    template<int BUFFSIZE=1024>
    void WriteV(int raw, int lineEnd, int severity, const char * format, va_list vl)
    {
        if (severity < mLevel && severity != LSV_UNKNOWN) return;

        struct tm * t = CheckLogName();
        if (t == NULL) return;

        char message[BUFFSIZE];
        vsnprintf(message, BUFFSIZE, format, vl);

        const char * lineFeed = "";
        int fmtlen = strlen(format);
        if (lineEnd && format[fmtlen - 1] != '\n') lineFeed = "\n";

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
            if (mFile.get())
            {
                if (!raw) fprintf(mFile
                    , "%s %s%s"
                    , tsp.s
                    , logSeverity__[severity]
                    , logSeverity__[severity][0] ? " " : ""
                );

                fprintf(mFile, "%s%s", message, lineFeed);
                fflush(mFile);
            }

            // handle stdout
            if (mTermColorful) AdjustConsoleAttr(severity);
            if (raw)
            {
                printf("%s", message);
            }
            else
            {
                printf("%s %s", tsp.s, message);
            }
            if (mTermColorful && lineEnd) RestoreConsoleAttr();
            if (lineEnd) printf("%s", lineFeed);
        }
    }

    struct tm * CheckLogName()
    {
        time_t now = time(NULL);
        struct tm * t = gmtime(&now);
        if (mBaseName.length() == 0 || mPolicy <= timestamped && mFile.get()) return t;

        xs expectedLogFileName = [&]() {
            char woy[4] = { 0 };
            switch (mPolicy)
            {
                case daily:
                    return xs("%s.%d%02d%02d%s", mBaseName.c_str(), t->tm_year+1900, t->tm_mon+1, t->tm_mday, mBaseExt.c_str());
                case weekly:
                    strftime(woy, sizeof(woy), "%w", t);
                    return xs("%s.%dW%s%s", mBaseName.c_str(), t->tm_year+1900, woy, mBaseExt.c_str());
                case monthly:
                    return xs("%s.%d%02d%s", mBaseName.c_str(), t->tm_year+1900, t->tm_mon+1, mBaseExt.c_str());
                case anually:
                    return xs("%s.%d%s", mBaseName.c_str(), t->tm_year+1900, mBaseExt.c_str());
                default:
                    return xs("%s%s", mBaseName.c_str(), mBaseExt.c_str());
            }
        }();

        if (mLogFile != expectedLogFileName.s)
        {
            mFile = fopen(xs("%s/%s", mLogDir.c_str(), expectedLogFileName.s), "a+t");
            if (mFile.get() == NULL)
            {
                fprintf(stderr, "cannot open log file '%s': %s"
                    , expectedLogFileName.s
                    , strerror(errno));
            }
            else
            {
                mLogFile = expectedLogFileName.s;
            }
        }

        return t;
    }

    void SetLogName(const std::string & logName)
    {
        if (logName.length() == 0)
        {
            throw std::invalid_argument("not a valid log file name.");
        }

        char * fname = NULL;
        char full[MAX_PATH];
        GetFullPathName(logName.c_str(), sizeof(full)/sizeof(full[0]), full, &fname);

        mBaseName = fname;
        char * dot = strrchr(fname, '.');
        if (dot)
        {
            dot[0] = '\0';
            mBaseName = fname;
            dot[0] = '.';
            if (dot[1]) mBaseExt = dot;
        }
        if (mPolicy == timestamped)
        {
            mBaseName = xs("%s.%ld", mBaseName.c_str(), (long)time(NULL)).s;
        }

        if (mBaseExt.length() == 0) mBaseExt = ".log";
        fname[0] = '\x0';
        mLogDir = full;

        if (!CreateDirectory(mLogDir.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        {
            throw errno_error(xs("CreateDirectory('%s') failed: ", mLogDir.c_str()).s);
        }

        // printf("LogBaseName: %s%s\n", mBaseName.c_str(), mBaseExt.c_str());
        // printf("LogDir: %s\n", mLogDir.c_str());
    }

private:
    scoped_ptr <FILE, file_dtor> mFile;
    
    std::string mBaseName;
    std::string mBaseExt;
    std::string mLogDir;
    std::string mLogFile;

    rotation mPolicy;
    int mSizeLimit;
    int mLevel;
    bool mTermColorful;

    CRITICAL_SECTION mLock;
    HANDLE mStdHandle;
    CONSOLE_ATTRIBUTE mNormalTextAttribute;
};

#ifndef _WIN32
inline void SetConsoleTextAttribute(HANDLE console, CONSOLE_ATTRIBUTE attr)
{
    fprintf(console, "\033[%sm", attr);
}
#endif//!_WIN32

extern logger & logger__;
template<int N=1024>
struct logutil
{
    logutil(int severity = LSV_TRACE, bool raw = false, bool lineEnd = true)
        : mSeverity(severity)
        , mLineEnd (lineEnd)
        , mRawOutput(raw)
    {
    }

    void operator () (const char * format, ...)
    {
        va_list vl;
        va_start(vl, format);
        logger__.WriteV<N>(mRawOutput, mLineEnd, mSeverity, format, vl);
        va_end(vl);
    }

    int mSeverity;
    bool mLineEnd;
    bool mRawOutput;
};

#define LOGENDL	    do {logger__.WriteRaw(1, "");} while(0)
#define LOGX		logutil
#define TRACE		logutil()
#define LOGD        logutil(LSV_DEBUG)
#define LOGT        logutil(LSV_TRACE)
#define LOGI		logutil(LSV_INFO)
#define LOGW		logutil(LSV_WARNING)
#define LOGE		logutil(LSV_ERROR)
#define LOGF		logutil(LSV_FATAL)
#define LOGU		logutil(LSV_UNKNOWN)

#endif//OPADMIN_LOG_H_INCLUDED__
