// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ClipboardHelper.h"
#include <wil/com.h>

bool CanShellPasteDataObject(PCIDLIST_ABSOLUTE destination, IDataObject *dataObject, DWORD effects)
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = SHBindToObject(nullptr, destination, nullptr, IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return false;
	}

	wil::com_ptr_nothrow<IDropTarget> dropTarget;
	hr = shellFolder->CreateViewObject(nullptr, IID_PPV_ARGS(&dropTarget));

	if (FAILED(hr))
	{
		return false;
	}

	// Internally, a paste is a simulated drop, so this should give a reliable indication of whether
	// such a drop would succeed.
	DWORD finalEffect = effects;
	hr = dropTarget->DragEnter(dataObject, 0, { 0, 0 }, &finalEffect);

	if (FAILED(hr) || finalEffect == DROPEFFECT_NONE)
	{
		return false;
	}

	return true;
}