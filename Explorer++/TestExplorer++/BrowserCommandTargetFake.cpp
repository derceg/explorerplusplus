// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "BrowserCommandTargetFake.h"

bool BrowserCommandTargetFake::IsCommandEnabled(int command) const
{
	UNREFERENCED_PARAMETER(command);

	return false;
}

void BrowserCommandTargetFake::ExecuteCommand(int command)
{
	UNREFERENCED_PARAMETER(command);
}
