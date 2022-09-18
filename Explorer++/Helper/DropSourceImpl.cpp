// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DropSourceImpl.h"
#include <wil/common.h>

IFACEMETHODIMP DropSourceImpl::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	if (fEscapePressed == TRUE
		|| (WI_IsFlagSet(grfKeyState, MK_LBUTTON) && WI_IsFlagSet(grfKeyState, MK_RBUTTON)))
	{
		return DRAGDROP_S_CANCEL;
	}

	if (WI_IsFlagClear(grfKeyState, MK_LBUTTON) && WI_IsFlagClear(grfKeyState, MK_RBUTTON))
	{
		return DRAGDROP_S_DROP;
	}

	return S_OK;
}

IFACEMETHODIMP DropSourceImpl::GiveFeedback(DWORD dwEffect)
{
	UNREFERENCED_PARAMETER(dwEffect);

	return DRAGDROP_S_USEDEFAULTCURSORS;
}
