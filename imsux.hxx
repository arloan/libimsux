#ifndef __IMSLIB_HPP_UX
#define __IMSLIB_HPP_UX

#ifndef __cplusplus
#error please use a c++ compiler
#endif//__cplusplus

#ifndef IMSUX_VER
#define IMSUX_VER 0x0100
#endif//IMSUX_VER

#include "xs.hxx"
#include "except.hxx"
#include "lock.hxx"
#include "auto.hxx"
#include "errno_error.hxx"
#include "comma_sep.hxx"
#include "stop_watch.hxx"
#ifdef _WINDOWS
#include "win32_error.hxx"
#endif//_WINDOWS

#define IMSUX_USE_NS	using namespace imsux;

#endif//__IMSLIB_HPP_UX
