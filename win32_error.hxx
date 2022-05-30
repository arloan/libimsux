#ifndef __IMSLIB_WIN32_ERROR_HPP
#define __IMSLIB_WIN32_ERROR_HPP

#if (_MSC_VER >= 800)
#pragma once
#endif

#ifndef __cplusplus
#error You must use c++ compiler.
#endif//__cplusplus

#ifdef _WIN32

#include <stdexcept>
#include <string>

namespace imsux {

class win32_error : public std::runtime_error
{
public:
	win32_error(DWORD e = GetLastError()) :
		std::runtime_error(format_message("WIN32 system error:", e)),
		_code(e)
	{
	}

	win32_error(const std::string & message, DWORD e = GetLastError()) :
		std::runtime_error(format_message(message, e)),
		_code(e)
	{
	}

	DWORD code()
	{
		return _code;
	}

private:

	static std::string format_message(const std::string & message, DWORD e)
	{
		LPVOID buf;
		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			e,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(char *)&buf,
			0,
			NULL);

		scoped_ptr<char, MemLocalDtor> msg((char *)buf);

		return xs("%s (%u: %s).", message.c_str(), e, (const char *)buf);
	}

	DWORD _code;
};

}

#endif /* _WIN32 */

#endif /* __IMSLIB_WIN32_ERROR_HPP */
