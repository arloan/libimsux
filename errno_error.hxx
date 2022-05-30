#ifndef __IMSLIB_ERRNO_ERROR_HPP
#define __IMSLIB_ERRNO_ERROR_HPP

#if (_MSC_VER >= 800)
#pragma once
#endif

#ifndef __cplusplus
#error You must use c++ compiler.
#endif//__cplusplus

#include <string>
#include <stdexcept>

#include <string.h>
#include <errno.h>

namespace imsux {

class errno_error : public std::runtime_error
{
public:
	errno_error() :
		std::runtime_error(format_message("Exception occured,", errno)),
		_code(errno)
	{
	}

	errno_error(const std::string & message) :
		std::runtime_error(format_message(message, errno)),
		_code(errno)
	{
	}

	errno_error(const std::string & message, int e) :
		std::runtime_error(format_message(message, e)),
		_code(e)
	{
	}

	int code()
	{
		return _code;
	}

private:

	static std::string format_message(const std::string & message, int e)
	{
		return xs("%s errno: %d (%s)", message.c_str(), e, strerror(e));
	}

	int _code;
};

}

#endif /* __IMSLIB_ERRNO_ERROR_HPP */
