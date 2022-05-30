#ifndef __IMSLIB_XS_HPP_UX
#define __IMSLIB_XS_HPP_UX

#ifndef __cplusplus
#error This file need a C++ compiler.
#endif//__cplusplus

#if (_MSC_VER >= 800)
#pragma once
#endif

#include <stdarg.h>
#include <string>

#ifndef IMSUX_XSBUFF_LEN
#define IMSUX_XSBUFF_LEN  2048
#endif//IMSUX_XSBUFF_LEN

namespace imsux {

struct xs
{
	xs(const xs & x) : s(buffer) {
		n = x.n;
		memcpy(buffer, x.buffer, sizeof(buffer));
	}
	xs(const std::string & s) : s(buffer) {
		n = (int)s.length();
		memcpy(buffer, s.c_str(), n+1);
	}
	xs(const char * format, ...) : s(buffer) {
		va_list vl;
		va_start(vl, format);
#if __STDC_VERSION__ >= 199901L
        n = vsnprintf(buffer, IMSUX_XSBUFF_LEN, format, vl);
#endif
		n = vsprintf(buffer, format, vl);
		va_end(vl);
	}
	/*
	// danger! va_list is of type void *, this function will got overloaded incorrectly
	xs(const char * format, va_list vl) : s(buffer) {
		n = vsprintf(buffer, format, vl);
	}
	*/
	operator const char * () const {
		return (const char *)buffer;
	}
	operator std::string () const {
		return str();
	}
	std::string str() const {
		return std::string(buffer, n);
	}

	const char * to_upper() {
		for (int i=0; i<n; ++i) {
			char c = buffer[i];
			if (c >= 'a' && c <= 'z') buffer[i] = c - 32;
		}
		return s;
	}
	const char * to_lower() {
		for (int i=0; i<n; ++i) {
			char c = buffer[i];
			if (c >= 'A' && c <= 'Z') buffer[i] = c + 32;
		}
		return s;
	}

	int n;
	const char * s;
	char buffer[IMSUX_XSBUFF_LEN];
};

} // namespace imsux

#endif/*__IMSLIB_XS_HPP_UX*/
