// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#define EMPTY_STRING _T("")

template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define SIZEOF_ARRAY(array) (sizeof(ArraySizeHelper(array)))

/* A macro to disallow the copy constructor and operator= functions
This should be used in the private: declarations for a class.
See http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml. */
#define DISALLOW_COPY_AND_ASSIGN(TypeName)                                                         \
	TypeName(const TypeName &);                                                                    \
	void operator=(const TypeName &)
