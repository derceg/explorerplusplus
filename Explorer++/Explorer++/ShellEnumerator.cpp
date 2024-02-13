// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellEnumerator.h"
#include "../Helper/ShellHelper.h"
#include <wil/common.h>

HRESULT ShellEnumerator::EnumerateDirectory(IShellFolder *shellFolder, HWND embedder, Flags flags,
	std::vector<PidlChild> &outputItems)
{
	SHCONTF enumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

	if (WI_IsFlagSet(flags, Flags::IncludeHidden))
	{
		WI_SetAllFlags(enumFlags, SHCONTF_INCLUDEHIDDEN | SHCONTF_INCLUDESUPERHIDDEN);
	}

	wil::com_ptr_nothrow<IEnumIDList> enumerator;
	HRESULT hr = shellFolder->EnumObjects(embedder, enumFlags, &enumerator);

	if (FAILED(hr) || !enumerator)
	{
		return hr;
	}

	ULONG numFetched = 1;
	unique_pidl_child pidlItem;

	while (enumerator->Next(1, wil::out_param(pidlItem), &numFetched) == S_OK && (numFetched == 1))
	{
		outputItems.emplace_back(pidlItem.get());
	}

	return S_OK;
}
