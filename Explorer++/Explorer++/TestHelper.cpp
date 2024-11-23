// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TestHelper.h"

namespace
{

bool g_inTest = false;

}

bool IsInTest()
{
	return g_inTest;
}

void SetIsInTest()
{
	CHECK(!g_inTest);
	g_inTest = true;
}
