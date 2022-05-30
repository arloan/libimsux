#ifndef STDINT_H__
#define STDINT_H__

#ifndef _SYS_INT_TYPES_H

typedef signed char		int8_t;
typedef unsigned char	uint8_t;
typedef unsigned char	byte;
typedef signed short	int16_t;
typedef unsigned short	uint16_t;
typedef signed int		int32_t;
typedef unsigned int	uint32_t;

#ifdef _WIN32
typedef __int64			int64_t;
typedef unsigned __int64 uint64_t;
#else
typedef long long		int64_t;
typedef unsigned long long uint64_t;
#endif

#endif /* _SYS_INT_TYPES_H */

#endif /* STDINT_H__ */
