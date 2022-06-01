/******************************************************************************

VERSION:
	1.1.0: current release
	1.0.0: first release

******************************************************************************/

#ifndef __IMSLIB_AUTO_HPP_UX
#define __IMSLIB_AUTO_HPP_UX

#ifndef __cplusplus
#error This file need a C++ compiler.
#endif//__cplusplus

#if (_MSC_VER >= 800)
#pragma once
#endif

namespace imsux {

#ifdef _WIN32	/* this macro should be defined by windows.h */
#include <windows.h>

struct RegDtor { inline void operator () (HKEY key) { RegCloseKey(key); } };
struct GDIDtor { inline void operator () (HGDIOBJ ob) { DeleteObject(ob); } };
struct ModuleDtor { inline void operator () (HMODULE h) { FreeLibrary(h); } };
struct HandleDtor { inline void operator () (HANDLE h) { CloseHandle(h); } };
struct FileMapViewDtor { inline void operator () (void * p) { UnmapViewOfFile(p); } };
struct MemLocalDtor { inline void operator () (HLOCAL h) { LocalFree(h); } };
struct MemGlobalDtor { inline void operator () (HGLOBAL h) { GlobalFree(h); } };
struct MemVirtualDtor { inline void operator () (void * p) { VirtualFree(p,0,MEM_RELEASE); } };

struct MemHeapDtor
{
	MemHeapDtor(HANDLE heap) : heap_(heap) {}
	inline void operator () (void * p) { HeapFree(heap_, 0, p); }

	HANDLE heap_;
};
struct HeapDestroier
{
	inline void operator () (HANDLE heap) { HeapDestroy(heap); }
};

#endif /* _WIN32 */


struct malloc_dtor { inline void operator () (void * p) { free(p); } };

template <class T>
struct new_dtor
{
	inline void operator () (T * p) { delete p; }
};
template <class T>
struct array_dtor
{
	inline void operator () (T * p) { delete [] p; }
};

template <class T, class C = array_dtor<T> >
class auto_ptr
{
public:
	typedef auto_ptr<T, C> this_type;
	typedef void (* Cleaner) (T * ptr);

public:
	auto_ptr(T * ptr = NULL) : m_ptr(ptr), m_ref(NULL)
	{
		addref();
	}
	auto_ptr(T * ptr, C fn) :
		m_ptr(ptr), m_fn(fn), m_ref(NULL)
	{
		addref();
	}
	auto_ptr(const auto_ptr & aptr)
	{
		m_ref = aptr.m_ref;
		m_ptr = aptr.m_ptr;
		m_fn = aptr.m_fn;

		addref();
	}

	virtual ~auto_ptr()
	{
		release();
	}

	this_type & operator = (const this_type & aptr)
	{
		release();

		m_ref = aptr.m_ref;
		m_ptr = aptr.m_ptr;
		m_fn = aptr.m_fn;

		addref();
		return * this;
	}

	bool operator == (const T * ptr) const { return m_ptr == ptr; }
	bool operator == (const this_type & ptr) const { return m_ptr == ptr.m_ptr; }
	bool operator != (const T * ptr) const { return !((*this) == ptr); }
	bool operator != (const this_type & ptr) const { return !((*this) == ptr); }

	operator T * () { return m_ptr; }
	operator const T * () const { return m_ptr; }
	T * operator -> () { return m_ptr; }
	const T * operator -> () const { return m_ptr; }
	T & operator [] (size_t index) { return m_ptr[index]; }
	const T & operator [] (size_t index) const { return m_ptr[index]; }
	T & operator * () { return * m_ptr; }
	const T & operator * () const { return * (const T*)m_ptr; }
	T * get() { return m_ptr; }
	const T * get() const { return m_ptr; }
	T ** operator & () const { return &m_ptr; }

	virtual int addref()
	{
		if (!m_ref)
		{
			m_ref = new int(1);
		}
		else
		{
			* m_ref ++;
		}

		return * m_ref;
	}
	virtual int release()
	{
		int ref = * m_ref --;
		if (ref == 0)
		{
			m_fn(m_ptr);
			delete m_ref;
			m_ref = NULL;
		}
		return ref;
	}

protected:
	int * m_ref;
	T * m_ptr;
	C m_fn;
};

#if defined _WIN32 && defined IMSUX_MT
template <class T, class C>
class auto_ptr_mt : public auto_ptr<T, C>
{
public:
	auto_ptr_mt(T * ptr = NULL) : auto_ptr(ptr)
	{
		m_lock = new CriticalLocker;
	}
	auto_ptr_mt(T * ptr, C fn) : auto_ptr(ptr, fn)
	{
		m_lock = new CriticalLocker;
	}
	auto_ptr_mt(const auto_ptr_mt & aptr)
	{
		m_ref = aptr.m_ref;
		m_ptr = aptr.m_ptr;
		m_fn = aptr.m_fn;
		m_lock = aptr.m_lock;

		addref();
	}

	~auto_ptr_mt()
	{
		release();
	}

	auto_ptr_mt & operator = (const auto_ptr_mt & aptr)
	{
		release();

		m_ref = aptr.m_ref;
		m_ptr = aptr.m_ptr;
		m_fn = aptr.m_fn;
		m_lock = aptr.m_lock;

		addref();
		return * this;
	}

	virtual int addref()
	{
		_ims_lock(CriticalSectionLocker, m_lock)
		{
			int ref = auto_ptr<T>::addref();
			return ref;
		}
	}

	virtual int release()
	{
		int ref = 0;
		_ims_lock(CriticalSectionLocker, m_lock)
		{
			ref = auto_ptr<T>::release();
		}
		if (ref == 0) delete m_lock;
		return ref;
	}

protected:
	CriticalSectionLocker * m_lock;
};
#endif//_WIN32

template <class T, class C = array_dtor<T> >
class scoped_ptr
{
public:
	typedef scoped_ptr <T, C> this_type;
	typedef void (* Cleaner) (T * ptr);

public:
	scoped_ptr(T * ptr = NULL) : m_ptr(ptr)
	{
	}
	scoped_ptr(T * ptr, C fn) :
		m_ptr(ptr), m_fn(fn)
	{
	}
	scoped_ptr(const this_type & ptr)
	{
		throw std::logic_error("Operation not allowed.");
	}
	~scoped_ptr()
	{
		m_fn(m_ptr);
	}

	this_type & attach(T * ptr) { m_fn(m_ptr); m_ptr = ptr; return * this; }
	T * detach() { T * _ptr = m_ptr; m_ptr = NULL; return _ptr; }

	this_type & operator = (T * ptr) { return attach(ptr); }
	this_type & operator = (const this_type & ob)
	{
		throw std::logic_error("Operation not allowed.");
	}

	bool operator == (const T * ptr) const { return m_ptr == ptr; }
	bool operator == (const this_type & ptr) const { return m_ptr == ptr.m_ptr; }
	bool operator != (const T * ptr) const { return !((*this) == ptr); }
	bool operator != (const this_type & ptr) const { return !((*this) == ptr); }
    
    bool isNull() const { return m_ptr == NULL; }

	operator T * () { return m_ptr; }
	operator const T * () const { return m_ptr; }
	T * operator -> () { return m_ptr; }
	const T * operator -> () const { return m_ptr; }
	T & operator [] (size_t index) { return m_ptr[index]; }
	const T & operator [] (size_t index) const { return m_ptr[index]; }
	T & operator * () { return * m_ptr; }
	const T & operator * () const { return * m_ptr; }
	T * get() { return m_ptr; }
	const T * get() const { return m_ptr; }
	T ** operator & () { return &m_ptr; }

protected:
	T * m_ptr;
	C m_fn;
};

template <class T, class C>
class scoped_ob
{
	typedef scoped_ob <T, C> this_type;

public:
	typedef void (* Cleaner)(T);

public:
	scoped_ob(T ob) : m_ob(ob)
	{
	}
	scoped_ob(T ob, C fn) : m_ob(ob), m_fn(fn)
	{
	}
	scoped_ob(const this_type & ob)
	{
		throw std::logic_error("Operation not allowed.");
	}
	~scoped_ob()
	{
		m_fn(m_ob);
	}

	this_type & attach(T ob) { m_fn(m_ob); m_ob = ob; return * this; }
	T detach(T nulVal = NULL) { T _ob = m_ob; m_ob = nulVal; return _ob; }

	bool operator == (T ob) const { return m_ob == ob; }
	bool operator == (const this_type & ob) const { return m_ob == ob.m_ob; }
	bool operator != (T ob) const { return !((*this) == ob); }
	bool operator != (const this_type & ob) const { return !((*this) == ob); }

	this_type & operator = (T ob) { return attach(ob); }
	this_type & operator = (const this_type & ob)
	{
		throw std::logic_error("Operation not allowed.");
	}

	operator T () const { return m_ob; }
	T * operator & () { return &m_ob; }
	const T * operator & () const { return &m_ob; }
	T get() const { return m_ob; }

protected:
	T m_ob;
	C m_fn;
};

} // namespace imsux

#endif/*__IMSLIB_AUTO_HPP_UX*/
