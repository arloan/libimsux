#ifndef __IMSLIB_EXCEPT_HPP_UX
#define __IMSLIB_EXCEPT_HPP_UX

#if (_MSC_VER >= 800)
#pragma once
#endif

#ifndef __cplusplus
#error You must use c++ compiler.
#endif//__cplusplus

namespace imsux {

struct exceptor	{
	exceptor(const char	* f, int ln) : f_(f), ln_(ln) {}
	template <class E>
	E operator = (const E & e) const {
		return E(imsux::xs("%s [FILE: '%s', LINE: %d]", e.what(), f_, ln_));
	}
	const char * f_;
	int	ln_;
};

}

#define	throwx	throw imsux::exceptor(__FILE__, __LINE__) =

#endif//__IMSLIB_EXCEPT_HPP_UX
