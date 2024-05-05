// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#define EMPTY_STRING _T("")

/* TODO: Switch to this once the compiler
provides support for constexpr.
See:
http://www.reddit.com/r/programming/comments/i0gz4/chromiums_code_is_perhaps_the_most_quality_code/
http://stackoverflow.com/questions/4748083/when-should-you-use-constexpr-capability-in-c0x
*/
// template <typename T,size_t N>
// constexpr size_t SIZEOF_ARRAY(T (&)[N])
//{
//	return N;
//}

template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define SIZEOF_ARRAY(array) (sizeof(ArraySizeHelper(array)))

/* A macro to disallow the copy constructor and operator= functions
This should be used in the private: declarations for a class.
See http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml. */
#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                                         \
	TypeName(const TypeName &);                                                                    \
	void operator=(const TypeName &)
