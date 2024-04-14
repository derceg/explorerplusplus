// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "PidlHelper.h"
#include "ShellHelper.h"

bool operator==(const PidlAbsolute &pidl1, const PidlAbsolute &pidl2)
{
	if (!pidl1.HasValue() && !pidl2.HasValue())
	{
		return true;
	}
	else if (!pidl1.HasValue() && pidl2.HasValue())
	{
		return false;
	}
	else if (pidl1.HasValue() && !pidl2.HasValue())
	{
		return false;
	}
	else
	{
		return ArePidlsEquivalent(pidl1.Raw(), pidl2.Raw());
	}
}
