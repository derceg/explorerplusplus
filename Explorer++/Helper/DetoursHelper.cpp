// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DetoursHelper.h"
#include <detours/detours.h>
#include <wil/resource.h>

LONG DetourTransaction(std::function<LONG()> callback)
{
	LONG res = DetourTransactionBegin();

	if (res != NO_ERROR)
	{
		return res;
	}

	auto cleanup = wil::scope_exit(
		[&res]
		{
			if (res != ERROR_SUCCESS)
			{
				DetourTransactionAbort();
			}
		});

	res = DetourUpdateThread(GetCurrentThread());

	if (res != NO_ERROR)
	{
		return res;
	}

	res = callback();

	if (res != NO_ERROR)
	{
		return res;
	}

	res = DetourTransactionCommit();

	if (res != NO_ERROR)
	{
		return res;
	}

	return res;
}
