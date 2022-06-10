/*********************************************************************************
File name:	comma_sep.hxx
Description:
			convert integer to per-thousands comma-separated string

Author:		Peng Qiu
Copyright:	PENG Qiu, 2022
History:
			created  on 06/03/2006 by Peng Qiu
**********************************************************************************/

#ifndef __IMSLIB_COMMASEP_HPP_UX
#define __IMSLIB_COMMASEP_HPP_UX

#ifndef __cplusplus
#error This file need a C++ compiler.
#endif//__cplusplus

#if (_MSC_VER >= 800)
#pragma once
#endif

#include <cmath>

namespace imsux {

template <class I = int, int maxdigits = 32>
class comma_sep {
	char buff[maxdigits + maxdigits / 3 + 2];
	char * p;
	I i;
	char sc;
public:
	comma_sep(I i, char c = ',') : p(buff), i(i), sc(c) {
		if (i < 0) {
			buff[0] = '-';
			*++p = '\0';
		}
	}

	const char * sep() {
		return _sep(i > 0 ? i : i * -1);
	}
private:
	const char * _sep(I i) {
		I r = i % 1000;
		I n = i / 1000;
		if (n > 0) {
			_sep(n);
			p += sprintf(p, "%c%03d", sc, (int)r);
			*p = '\0';
		} else {
			p += sprintf(p, "%d", (int)r);
			*p = '\0';
		}
		return buff;
	}
};

template<int maxd>
class comma_sep<double, maxd> {
	comma_sep<int64_t, maxd> _cs;
	char fs[64];
    double f;
public:
	const int max_frac = 12;
	comma_sep(double d, char c = ',') : _cs((int64_t)d, c) {
        double np;
		f = fabs(modf(d, &np));
	}
	const char * sep(int frac = 3) {
		if (frac < 1 || frac > max_frac) {
			throw std::invalid_argument("factional part too too long or invalid");
		}
		auto p = _cs.sep();
        strcpy(fs, p);
		char fmt[8], tmp[max_frac+3];
		sprintf(fmt, "%%.%dlf", frac);
		sprintf(tmp, fmt, f);
		return strcat(fs, tmp + 1);
	}
};

////////////////////////////////////////////////////////////
// usage:
// cout << comma_sep(-2439009706097096, '_').sep() << endl;
////////////////////////////////////////////////////////////

}

#endif//__IMSLIB_COMMASEP_HPP_UX
