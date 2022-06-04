/*********************************************************************************
File name:	stop_watch.hxx
Description:
			a simple tool to count seconds & milliseconds

Author:		Peng Qiu
Copyright:	PENG Qiu, 2022
History:
			created  on 06/03/2006 by Peng Qiu
**********************************************************************************/

#ifndef __IMSLIB_STOPWATCH_HPP_UX
#define __IMSLIB_STOPWATCH_HPP_UX

#ifndef __cplusplus
#error This file need a C++ compiler.
#endif//__cplusplus

#if (_MSC_VER >= 800)
#pragma once
#endif

#include <sys/time.h>
#include "errno_error.hxx"

namespace imsux {

class stop_watch {
public:
	struct tickv {
		double ellapsed;
		double fracs;
		time_t secs;
		int usecs;
	};

	static void rst() { instance__().reset(); }
	static tickv tik() { return instance__().tick(); }

	stop_watch() {
		reset();
	}

	void reset() {
		if (gettimeofday(&tv, NULL)) throw errno_error("gettimeofday() failed");
	}

	tickv tick() {
		timeval old = tv;
		reset();
		auto eus = tv.tv_sec * (uint64_t)1000000 + tv.tv_usec - (old.tv_sec * (uint64_t)1000000 + old.tv_usec);
		time_t secs = (time_t)(eus / 1000000);
		int usecs = (int)(eus - secs * 1000000);
		double fracs = usecs / 1000000.0;
		double ellapsed = eus / 1000000.0;
		return tickv {ellapsed, fracs, secs, usecs};
	}
private:
	timeval tv;
	static stop_watch & instance__() {
		static stop_watch sw__;
		return sw__;
	}
};

////////////////////////////////////////////////////////////
// usage:
// cout << comma_sep(-2439009706097096, '_').sep() << endl;
////////////////////////////////////////////////////////////

}

#endif//__IMSLIB_STOPWATCH_HPP_UX
