// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellTreeView.h"

HRESULT _stdcall ShellTreeView::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	if (fEscapePressed == TRUE
		|| (WI_IsFlagSet(grfKeyState, MK_LBUTTON) && WI_IsFlagSet(grfKeyState, MK_RBUTTON)))
	{
		return DRAGDROP_S_CANCEL;
	}

	return S_OK;
}

HRESULT _stdcall ShellTreeView::GiveFeedback(DWORD dwEffect)
{
	UNREFERENCED_PARAMETER(dwEffect);

	return DRAGDROP_S_USEDEFAULTCURSORS;
}