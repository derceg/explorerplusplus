// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#define EMPTY_STRING _T("")

template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
#define SIZEOF_ARRAY(array) (sizeof(ArraySizeHelper(array)))
