// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/version.hpp>

#ifdef __clang__

	#define EPP_QUOTE_(x) #x
	#define EPP_QUOTE(x) EPP_QUOTE_(x)

	// There's the implicit assumption here that Boost is being compiled with MSVC.
	#define EPP_BOOST_TOOLSET "-vc" EPP_QUOTE(PLATFORM_TOOLSET_VERSION)

	#ifdef _MT
		#define EPP_BOOST_THREAD_OPT "-mt"
	#else
		#define EPP_BOOST_THREAD_OPT
	#endif

	#ifdef _DEBUG
		#define EPP_BOOST_RUNTIME_OPT "-gd"
	#else
		#define EPP_BOOST_RUNTIME_OPT
	#endif

	#if defined(BUILD_WIN32)
		#define EPP_BOOST_ARCH_AND_MODEL_OPT "x32"
	#elif defined(BUILD_WIN64)
		#define EPP_BOOST_ARCH_AND_MODEL_OPT "x64"
	#elif defined(BUILD_ARM64)
		#define EPP_BOOST_ARCH_AND_MODEL_OPT "a64"
	#else
		#error "Unknown target platform"
	#endif

	#define EPP_BOOST_LIB_NAME(lib)                                                                \
		"boost_" EPP_QUOTE(lib) EPP_BOOST_TOOLSET EPP_BOOST_THREAD_OPT EPP_BOOST_RUNTIME_OPT       \
			"-" EPP_BOOST_ARCH_AND_MODEL_OPT "-" BOOST_LIB_VERSION ".lib"

	#ifdef _DEBUG
		#define EPP_FMT_CONFIG "d"
	#else
		#define EPP_FMT_CONFIG
	#endif

	#define EPP_FMT_LIB_NAME "fmt" EPP_FMT_CONFIG ".lib"

	#ifdef _DEBUG
		#define EPP_GFLAGS_CONFIG "_debug"
	#else
		#define EPP_GFLAGS_CONFIG
	#endif

	#define EPP_GFLAGS_LIB_NAME "gflags_static" EPP_GFLAGS_CONFIG ".lib"

	// vcpkg's autolink feature (which links the necessary .lib files using a wildcard) doesn't work
	// with lld-link.exe. Therefore, the .lib files will be manually linked here.
	#pragma comment(lib, EPP_BOOST_LIB_NAME(atomic))
	#pragma comment(lib, EPP_BOOST_LIB_NAME(charconv))
	#pragma comment(lib, EPP_BOOST_LIB_NAME(chrono))
	#pragma comment(lib, EPP_BOOST_LIB_NAME(container))
	#pragma comment(lib, EPP_BOOST_LIB_NAME(date_time))
	#pragma comment(lib, EPP_BOOST_LIB_NAME(locale))
	#pragma comment(lib, EPP_BOOST_LIB_NAME(thread))
	#pragma comment(lib, "CLI11.lib")
	#pragma comment(lib, "concurrencpp.lib")
	#pragma comment(lib, "cppwinrt_fast_forwarder.lib")
	#pragma comment(lib, "detours.lib")
	#pragma comment(lib, EPP_FMT_LIB_NAME)
	#pragma comment(lib, EPP_GFLAGS_LIB_NAME)
	#pragma comment(lib, "glog.lib")
	#pragma comment(lib, "lua.lib")

	#undef EPP_QUOTE_
	#undef EPP_QUOTE
	#undef EPP_BOOST_TOOLSET
	#undef EPP_BOOST_THREAD_OPT
	#undef EPP_BOOST_RUNTIME_OPT
	#undef EPP_BOOST_ARCH_AND_MODEL_OPT
	#undef EPP_BOOST_LIB_NAME
	#undef EPP_FMT_CONFIG
	#undef EPP_FMT_LIB_NAME
	#undef EPP_GFLAGS_CONFIG
	#undef EPP_GFLAGS_LIB_NAME

#endif // __clang__
