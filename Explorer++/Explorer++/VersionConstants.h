// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#define QUOTE_(x) #x
#define QUOTE(x) QUOTE_(x)

#define VERSION_STRING QUOTE(MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION.BUILD_VERSION)

#define BUILD_DATE_STRING _T(__DATE__) _T(" ") _T(__TIME__)
