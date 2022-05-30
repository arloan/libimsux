/*********************************************************************************
File name:	utf8_conv_win32.h
Description:
			utf8 conversion class for win32
			Version 1.3.2

Author:		Peng Qiu
Copyright:	PENG Qiu, 2006-2010
History:
			created  on 07/06/2006 by Peng Qiu
			modified on 09/12/2006 by Peng Qiu
			modified on 10/10/2006 by Peng Qiu
			modified on 03/30/2010 by Peng Qiu
**********************************************************************************/

#include <windows.h>
typedef UINT charset_t;
typedef std::map <std::string, charset_t> charset_map;

class os_sal	// os-depend helper class: Software Abstract Layer :P
{
public:
	static std::string charset_convert(charset_t from_charset, charset_t to_charset, const char * str, int32_t len)
	{
		// step 1: convert mbcs from 'from_charset' to utf-16
		std::wstring unicode = mbcs_to_unicode(from_charset, str, len);

		// step 2: convert from utf-16 to mbcs of 'to_charset'
		return unicode_to_mbcs(to_charset, unicode.c_str(), (int)unicode.length());
	}

	static charset_map & init_charset_map()
	{
		static charset_map ch_map;
		ch_map["UTF8"]		= CP_UTF8;
		ch_map["UTF-8"]		= CP_UTF8;
		ch_map["ANSI"]		= CP_ACP;
		ch_map["C"]			= CP_ACP;
		ch_map["GB2312"]	= 936;
		ch_map["GBK"]		= 936;
		ch_map["GB18030"]	= 936;
		ch_map["OEM"]		= CP_OEMCP;
		ch_map[""]			= CP_OEMCP;
		return ch_map;
	}

private:
	static std::wstring mbcs_to_unicode(charset_t charset, const char * mbcs, int32_t len)
	{
		int need = MultiByteToWideChar
			( charset
			, (charset == CP_UTF8) ? 0 : MB_PRECOMPOSED
			, mbcs
			, len
			, NULL
			, 0
			);

		if (need == 0) throw std::runtime_error("calculate unicode buffer length failed.");

		wchar_t * buffer = new wchar_t [need + 1];
		memset(buffer, 0, (int)sizeof(wchar_t) * (need + 1));

		need = MultiByteToWideChar
			( charset
			, (charset == CP_UTF8) ? 0 : MB_PRECOMPOSED
			, mbcs
			, len
			, buffer
			, need
			);

		if (need == 0)
		{
			delete [] buffer;
			throw std::runtime_error("convert to unicode failed.");
		}

		std::wstring r = buffer;
		delete [] buffer;

		return r;
	}

	static std::string unicode_to_mbcs(charset_t charset, const wchar_t * unicode, int32_t len)
	{
		int need = WideCharToMultiByte
			( charset
			, 0
			, unicode
			, len
			, NULL
			, 0
			, NULL
			, NULL
			);

		if (need == 0) throw std::runtime_error("calculate mbcs buffer length failed.");

		char * buffer = new char [need + 1];
		memset(buffer, 0, sizeof(char) * (need + 1));

		need = WideCharToMultiByte
			( charset
			, 0
			, unicode
			, len
			, buffer
			, need
			, NULL
			, NULL
			);

		if (need == 0)
		{
			delete [] buffer;
			throw std::runtime_error("convert to mbcs failed.");
		}

		std::string r = buffer;
		delete [] buffer;

		return r;
	}
};
