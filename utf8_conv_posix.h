/*********************************************************************************
File name:	utf8_conv_posix.h
Description:
			utf8 conversion class for POSIX system
			Version 1.3.3

Author:		Peng Qiu
Copyright:	PENG Qiu, 2006-2010
History:
			created  on 07/07/2006 by Peng Qiu
			modified on 09/12/2006 by Peng Qiu
			modified on 10/10/2006 by Peng Qiu
			modified on 02/06/2007 by Peng Qiu
			modified on 03/30/2010 by Peng Qiu
**********************************************************************************/

#include <iconv.h>
#include <errno.h>

typedef std::string charset_t;
typedef std::map <std::string, charset_t> charset_map;

class grow_buffer
{
public:
	grow_buffer(int32_t init_size, int32_t grow_size)
		: m_buffer_size(init_size)
		, m_grow_size  (grow_size)
	{
		if (init_size <= 0 || grow_size <= 0) throw std::invalid_argument("invalid init_size or grow_size");

		m_data_size = 0;
		m_buffer = new char [init_size];
	};

	~grow_buffer()
	{
		delete [] m_buffer;
	}

public:
	void grow()
	{
		m_buffer_size += m_grow_size;

		char * old_buffer = m_buffer;
		m_buffer = new char [m_buffer_size];
		memcpy(m_buffer, old_buffer, m_data_size);

		delete [] old_buffer;
	}

	int32_t length() const { return m_buffer_size; }
	int32_t filled() const { return m_data_size; }
	int32_t & filled() { return m_data_size; }

	char * front() { return m_buffer; }
	const char * front() const { return m_buffer; }
	char * back() { return m_buffer + m_data_size; }
	const char * back() const { return m_buffer + m_data_size; }

private:
	char * m_buffer;
	int32_t m_buffer_size;
	int32_t m_data_size;
	int32_t m_grow_size;
};

class os_sal	// os-depend helper class: Software Abstract Layer :P
{
public:
	static std::string charset_convert(charset_t from_charset, charset_t to_charset, const char * str, int32_t len)
	{
		iconv_t cd = iconv_open(to_charset.c_str(), from_charset.c_str());
		if (cd == (iconv_t)-1)
		{
			return "";
		}

		grow_buffer gb(1024, 1024);
		size_t nn = len;
		for (const char * s = str; nn; )
		{
			char * out = gb.back();
			size_t n = gb.length();
			// warning: explicit conver &s to char** MAY cause problem
			size_t r = iconv(cd, (char **)&s, &nn, &out, &n);

			gb.filled() = gb.length() - n;

			if (r == (size_t)-1)
			{
				if (errno == E2BIG)
				{
					gb.grow();
					continue;
				}
				break;
			}
		}

		return std::string(gb.front(), gb.filled());
	}

	static charset_map & init_charset_map()
	{
		static charset_map ch_map;
		ch_map["UTF8"]		= "UTF-8";
		ch_map["UTF-8"]		= "UTF-8";
		ch_map["ANSI"]		= "ASCII";
		ch_map["C"]			= "ASCII";
		ch_map["GB2312"]	= "GB2312";
		ch_map["GBK"]		= "GBK";
		ch_map["GB18030"]	= "GB18030";
		ch_map["OEM"]		= "";
		ch_map[""]			= "";
		return ch_map;
	}
};
