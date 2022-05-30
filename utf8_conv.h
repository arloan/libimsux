/*********************************************************************************
File name:	utf8_conv.h
Description:
			generic utf8 conversion class
			Version 1.2.4

Author:		Peng Qiu
Copyright:	PENG Qiu, 2006-2010
History:
			created  on 07/06/2006 by Peng Qiu
			modified on 09/12/2006 by Peng Qiu
			modified on 10/10/2006 by Peng Qiu
			modified on 02/04/2010 by Peng Qiu
			modified on 03/30/2010 by Peng Qiu
**********************************************************************************/

#ifndef UTF8_CONV_H_INCLUDED__
#define UTF8_CONV_H_INCLUDED__

#ifndef __cplusplus
#error This file need a C++ compiler.
#endif//__cplusplus

#include <stdexcept>
#include <string>
#include <map>
#include "stdint.h"

#ifdef _WIN32
#include "utf8_conv_win32.h"
#else
#include "utf8_conv_posix.h"
#endif //_WIN32

class utf8_conv
{
public:
	utf8_conv(const char * charset = "")
	{
		if (!charset) throw std::invalid_argument("Invalid charset name.");

		m_mbs_charset = charset;
	}

	std::string to_utf8(const std::string & mbcs) const
	{
		return to_utf8(mbcs.c_str(), mbcs.length());
	}
	std::string to_utf8(const char * mbcs, int32_t len = -1) const
	{
		if (len < -1) throw std::invalid_argument("invalid string length.");
		if (len == 0) return std::string("");

		charset_map & chmap = s_charset_map();

		if (chmap.find(m_mbs_charset) == chmap.end())
		{
			throw std::runtime_error("unsupported charset");
		}

		int32_t n = (len == -1) ? (int)strlen(mbcs) : len;

		charset_t cp_utf8 = chmap["UTF-8"];
		charset_t charset = chmap[m_mbs_charset];

		if (cp_utf8 == charset)
		{
			// already utf8
			return std::string(mbcs, n);
		}
		else
		{
			// charset_convert() is platform-dependent, defined in "packer_$(platform).h"
			return os_sal::charset_convert(charset, cp_utf8, mbcs, n);
		}
	}

	std::string from_utf8(const std::string & utf8) const
	{
		return from_utf8(utf8.c_str(), utf8.length());
	}
	std::string from_utf8(const char * utf8, int32_t len = -1) const
	{
		if (len < -1) throw std::invalid_argument("invalid string length.");
		if (len == 0) return std::string("");

		charset_map & chmap = s_charset_map();

		if (chmap.find(m_mbs_charset) == chmap.end())
		{
			throw std::runtime_error("unsupported charset");
		}

		int32_t n = (len == -1) ? (int)strlen(utf8) : len;

		charset_t cp_utf8 = chmap["UTF-8"];
		charset_t charset = chmap[m_mbs_charset];

		// from utf8 to utf8: convert not needed
		if (cp_utf8 == charset)
		{
			return std::string(utf8, n);
		}
		else
		{
			// charset_convert() is platform-dependent, defined in "packer_$(platform).h"
			return os_sal::charset_convert(cp_utf8, charset, utf8, n);
		}
	}

private:
	static charset_map & s_charset_map()
	{
		return os_sal::init_charset_map();
	}

private:
	std::string m_mbs_charset;
};

#endif//UTF8_CONV_H_INCLUDED__
