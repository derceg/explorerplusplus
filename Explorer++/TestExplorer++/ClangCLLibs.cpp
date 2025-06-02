// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ClangCLLibs.h"

#ifdef __clang__
	#pragma comment(lib, "gmock.lib")
	#pragma comment(lib, "gtest.lib")
#endif // __clang__
