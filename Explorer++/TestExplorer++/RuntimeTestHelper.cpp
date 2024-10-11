// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "RuntimeTestHelper.h"
#include "ComStaThreadPoolExecutor.h"
#include "Runtime.h"
#include "UIThreadExecutor.h"

Runtime BuildRuntimeForTest()
{
	return Runtime(std::make_unique<UIThreadExecutor>(),
		std::make_unique<ComStaThreadPoolExecutor>(1));
}
