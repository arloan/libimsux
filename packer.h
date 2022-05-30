/*********************************************************************************
File name:	packer.h
Description:
			packer and related classes
			Version 1.4.5

Author:		PENG Qiu
Copyright:	PENG Qiu, 2006-2009
History:
			created  on 07/03/2006 by Peng Qiu
			modified on 09/12/2006 by Peng Qiu
			modified on 09/13/2006 by Peng Qiu
			modified on 09/14/2006 by Peng Qiu
			modified on 02/06/2007 by Peng Qiu
			modified on 08/22/2007 by PENG Qiu
			modified on 11/03/2009 by PENG Qiu
				(allow null string to be pushed)
				(namespace changed)
**********************************************************************************/

#ifndef IECAS_PACKER_H_INCLUDED__
#define IECAS_PACKER_H_INCLUDED__

#include "utf8_conv.h"

namespace imsux {

class blob
{
public:
	blob(int32_t len)
	{
		if (len < 0) throw std::invalid_argument("invalid buffer length.");

		m_buffer = new byte [len];
		m_buffer_len = len;

		memset(m_buffer, 0, len);
	}

	blob(byte * buffer, int32_t len)
	{
		if (!buffer) throw std::invalid_argument("invalid buffer.");
		if (len < 0) throw std::invalid_argument("invalid buffer length.");

		m_buffer = new byte [len];
		m_buffer_len = len;

		memcpy(m_buffer, buffer, len);
	}

	blob(const blob & b)
	{
		copy_from(b);
	}

	~blob()
	{
		delete [] m_buffer;
	}

	blob & operator = (const blob & b)
	{
		delete [] m_buffer;

		copy_from(b);

		return * this;
	}

	byte * buffer() { return m_buffer; };
	const byte * buffer() const { return m_buffer; };
	int32_t length() const { return m_buffer_len; };

private:
	void copy_from(const blob & b)
	{
		m_buffer = new byte [b.m_buffer_len];
		m_buffer_len = b.m_buffer_len;
		memcpy(m_buffer, b.m_buffer, m_buffer_len);
	}

	byte * m_buffer;
	int32_t m_buffer_len;
};

class value_packer
{
public:
	virtual ~value_packer() {};

public:
	// access functions

	// bind() does not do buffer copy. after bind(), the buffer is accessed but not owned by packer.
	virtual void bind(byte * buffer, int32_t buffer_len) = 0;
	virtual void * buffer() = 0;
	virtual const void * buffer() const { return ((value_packer *)this)->buffer(); }
	virtual int32_t length() const = 0;

	// push functions
	virtual void push_int8 (int8_t  v) = 0;
	virtual void push_int16(int16_t v) = 0;
	virtual void push_int32(int32_t v) = 0;
	virtual void push_int64(int64_t v) = 0;

	virtual void push_uint8 (uint8_t  v) = 0;
	virtual void push_uint16(uint16_t v) = 0;
	virtual void push_uint32(uint32_t v) = 0;
	virtual void push_uint64(uint64_t v) = 0;

	virtual void push_float (float  v) = 0;
	virtual void push_double(double v) = 0;

	virtual void push_string(const char * v, int32_t n = -1) = 0;
	virtual void push_string(const std::string & s)
	{
		push_string(s.c_str(), (int)s.length());
	}

	virtual void push_binary(const void * v, int32_t n)      = 0;
	virtual void push_binary(const blob & b)
	{
		push_binary(b.buffer(), b.length());
	}

	virtual void push_raw(const void * v, int32_t n)         = 0;
	virtual void push_raw(const blob & b)
	{
		push_raw(b.buffer(), b.length());
	}

	// pop functions
	virtual int8_t  pop_int8 () = 0;
	virtual int16_t pop_int16() = 0;
	virtual int32_t pop_int32() = 0;
	virtual int64_t pop_int64() = 0;

	virtual uint8_t  pop_uint8 () = 0;
	virtual uint16_t pop_uint16() = 0;
	virtual uint32_t pop_uint32() = 0;
	virtual uint64_t pop_uint64() = 0;

	virtual float  pop_float  () = 0;
	virtual double pop_double () = 0;

	// pop_string: when v is NULL, return length needed, and pop cursor should not be advanced.
	// when v is not NULL but n is not enough, string will be truncated, and the pop cursor will be advanced.
	virtual int32_t     pop_string(char * v, int32_t n) = 0;
	virtual std::string pop_string()
	{
		// default implementation:
		int32_t len = pop_string(NULL, 0);
		char * s = new char [len + 1];
		pop_string(s, len + 1);

		std::string rs(s, len);
		delete [] s;

		return rs;
	}

	// pop_binary: when v is NULL, return length needed, and pop cursor should not be advanced.
	// when v is not NULL but n is not enough, content will be truncated, and the pop cursor will be advanced.
	virtual int32_t     pop_binary(void * v, int32_t n) = 0;
	virtual blob        pop_binary()
	{
		// default implementation:
		int32_t len = pop_binary(NULL, 0);
		byte * b = new byte [len];
		pop_binary(b, len);

		return blob(b, len);
	}
	virtual int32_t		pop_raw(void * v, int32_t n) = 0;
	virtual blob		pop_raw(int32_t) = 0;

// assistant static functions
public:
	template <class T>
	static T convert_endian(T v)
	{
		static bool host_is_le = is_host_le();

		if (host_is_le)
		{
			T v2;

			byte * p1 = (byte *)&v;
			byte * p2 = (byte *)&v2;

			for (int i=0; i<(int)sizeof(T); ++i)
			{
				p2 [(int)sizeof(T) - i - 1] = p1[i];
			}

			return v2;
		}
		else
		{
			return v;
		}
	}

	static bool is_host_le()
	{
		int32_t v = 0x1;
		char * p = (char *)&v;

		return p[0] ? true : false;
	}
};

class binary_packer : public value_packer
{
public:
	static int32_t & default_buffer_len()
	{
		static int len_ = 1024;
		return len_;
	};

	binary_packer(int32_t init_buffer = 0, const char * charset = "")
		: m_buffer_doclean(true)
		, m_buffer(NULL)
		, m_buffer_len(init_buffer)
		, m_buffer_occupied(0)
		, m_popped(0)
		, m_charset(charset)
		, m_conv(charset)
	{
		if (m_buffer_len < 0) throw std::invalid_argument("invalid buffer length.");
		if (m_buffer_len == 0) m_buffer_len = default_buffer_len();

		m_buffer = new byte [m_buffer_len];
		memset(m_buffer, 0, m_buffer_len);
	}

	virtual ~binary_packer()
	{
		if (m_buffer_doclean) delete [] m_buffer;
	}

public:
	// access interfaces
	virtual void * buffer() { return m_buffer; }
	virtual int32_t length() const { return m_buffer_occupied; }
	virtual void bind(byte * buffer, int32_t buffer_len)
	{
		if (!buffer || buffer_len <= 0) throw std::invalid_argument("empty buffer or invalid buffer length");

		delete [] m_buffer;

		m_buffer_doclean = false;
		m_buffer = buffer;
		m_buffer_len = m_buffer_occupied = buffer_len;
	}
	virtual void reset()
	{
		memset(m_buffer, 0, m_buffer_len);
		m_buffer_occupied = 0;
	}

	// push interfaces
	virtual void push_int8 (int8_t  v) { pack_single_value(v); }
	virtual void push_int16(int16_t v) { pack_single_value(v); }
	virtual void push_int32(int32_t v) { pack_single_value(v); }
	virtual void push_int64(int64_t v) { pack_single_value(v); }
	virtual void push_float(float   v) { pack_single_value(v); }
	virtual void push_double(double v) { pack_single_value(v); }

	virtual void push_uint8 (uint8_t  v) { push_int8 ((int8_t )v); }
	virtual void push_uint16(uint16_t v) { push_int16((int16_t)v); }
	virtual void push_uint32(uint32_t v) { push_int32((int32_t)v); }
	virtual void push_uint64(uint64_t v) { push_int64((int64_t)v); }

	using value_packer::push_string;
	virtual void push_string(const char * v, int32_t n = -1)
	{
		if (n < -1) throw std::invalid_argument("invalid string length.");
		if (!v) throw std::invalid_argument("null string specified.");

		int32_t len = (n == -1) ? (int)strlen(v) : n;
		std::string utf8 = m_conv.to_utf8(v, len);

		len = (int)utf8.length();
		push_int32(len);
		push_raw(utf8.c_str(), len);
	}

	virtual void push_binary(const void * v, int32_t n)
	{
		if (!v) throw std::invalid_argument("invalid address.");
		if (n < 0) throw std::invalid_argument("invalid buffer length.");

		push_int32(n);
		push_raw(v, n);
	}

	virtual void push_raw(const void * v, int32_t n)
	{
		if (!v) throw std::invalid_argument("invalid address.");
		if (n < 0) throw std::invalid_argument("invalid buffer length.");

		ensure_buffer_enough(n);
		memcpy(m_buffer + m_buffer_occupied, v, n);
		m_buffer_occupied += n;
	}

	// pop interfaces
	virtual void pop_reset(int32_t pos = 0)
	{
		if (pos < 0 || pos > m_buffer_occupied) throw std::invalid_argument("invalid pop position.");

		m_popped = pos;
	}

	// MSVC 6.0 does not support this: T Foo::Get<T>()... -_-!
	virtual int8_t  pop_int8 () { return unpack_single_value(int8_t ()); }
	virtual int16_t pop_int16() { return unpack_single_value(int16_t()); }
	virtual int32_t pop_int32() { return unpack_single_value(int32_t()); }
	virtual int64_t pop_int64() { return unpack_single_value(int64_t()); }
	virtual float   pop_float() { return unpack_single_value(float  ()); }
	virtual double  pop_double(){ return unpack_single_value(double ()); }

	virtual uint8_t  pop_uint8 () { return (uint8_t )pop_int8 (); }
	virtual uint16_t pop_uint16() { return (uint16_t)pop_int16(); }
	virtual uint32_t pop_uint32() { return (uint32_t)pop_int32(); }
	virtual uint64_t pop_uint64() { return (uint64_t)pop_int64(); }

	virtual std::string pop_string()
	{
		int32_t len = pop_int32();

		if (m_popped + len > m_buffer_occupied) throw std::out_of_range("no more content");

		int32_t popped = m_popped;
		m_popped += len;

		std::string utf8((const char *)m_buffer + popped, len);
		return m_conv.from_utf8(utf8.c_str(), (int)utf8.length());
	}

	virtual int32_t pop_string(char * v, int32_t n)
	{
		if (v == NULL)
		{
			// v == NULL to query string length

			int32_t len = pop_int32();
			m_popped -= (int)sizeof(int32_t);
			return len;
		}

		if (n <= 0) throw std::invalid_argument("string buffer length not valid.");

		std::string s = pop_string();
		strncpy(v, s.c_str(), n - 1);
		v[n - 1] = '\0';

		return (int)strlen(v);
	}

	virtual blob pop_binary()
	{
		int32_t len = pop_int32();

		if (m_popped + len > m_buffer_occupied) throw std::out_of_range("not enough content.");

		m_popped += len;

		return blob(m_buffer + m_popped - len, len);
	}

	virtual int32_t pop_binary(void * v, int32_t n)
	{
		// n may greater or less than the real length of content

		if (v == NULL)
		{
			int32_t len = pop_int32();
			m_popped -= (int)sizeof(int32_t);
			return len;
		}

		if (n < 0) throw std::invalid_argument("invalid buffer length.");

		blob b = pop_binary();
		int32_t cp = b.length() > n ? n : b.length();

		memcpy(v, b.buffer(), cp);
		return cp;
	}

	virtual int32_t pop_raw(void * v, int32_t n)
	{
		if (v == NULL) throw std::invalid_argument("invalid buffer.");
		if (n < 0) throw std::invalid_argument("invalid buffer length");

		m_popped += n;
		memcpy(v, m_buffer + m_popped - n, n);

		return n;
	}

	virtual blob pop_raw(int32_t n)
	{
		if (m_popped + n > m_buffer_occupied) throw std::out_of_range("not enough content.");

		m_popped += n;
		return blob(m_buffer + m_popped - n, n);
	}

protected:
	void ensure_buffer_enough(size_t extend_size)
	{
		if ((int32_t)extend_size + m_buffer_occupied > m_buffer_len)
		{
			if (!m_buffer_doclean) throw std::logic_error("bound buffer cannot grow.");

			do
			{
				m_buffer_len += m_buffer_len;
			}
			while ((int32_t)extend_size + m_buffer_occupied > m_buffer_len);

			byte * new_buffer = new byte [m_buffer_len];

			memset(new_buffer, 0, m_buffer_len);
			memcpy(new_buffer, m_buffer, m_buffer_occupied);
			delete [] m_buffer;
			m_buffer = new_buffer;
		}
	}

	template <class T>
	void pack_single_value(T v)
	{
		ensure_buffer_enough(sizeof(T));

		T v2 = value_packer::convert_endian(v);
		memcpy(m_buffer + m_buffer_occupied, &v2, (int)sizeof(T));
		m_buffer_occupied += (int)sizeof(T);
	}

	// MSVC 6.0 does not support this: T Foo::Get<T>()... -_-!
	template <class T>
	T unpack_single_value(T)
	{
		if (m_popped + (int)sizeof(T) > m_buffer_occupied) throw std::out_of_range("no more content.");

		T v;
		memcpy(&v, m_buffer + m_popped, (int)sizeof(T));
		m_popped += (int)sizeof(T);

		return value_packer::convert_endian(v);
	}

protected:
	utf8_conv m_conv;
	std::string m_charset;
	bool m_buffer_doclean;
	byte  * m_buffer;
	int32_t m_buffer_len;
	int32_t m_buffer_occupied;
	int32_t m_popped;
};

} // namespace imsux

#endif //IECAS_PACKER_H_INCLUDED__
