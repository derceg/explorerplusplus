// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MyTreeView.h"

HRESULT _stdcall CMyTreeView::QueryContinueDrag(BOOL fEscapePressed,DWORD grfKeyState)
{
	DWORD dwStopButton = 0;

	if(m_DragType == DRAG_TYPE_LEFTCLICK)
	{
		if((grfKeyState & MK_LBUTTON) == 0)
			return DRAGDROP_S_DROP;

		dwStopButton = MK_RBUTTON;
	}
	else if(m_DragType == DRAG_TYPE_RIGHTCLICK)
	{
		if((grfKeyState & MK_RBUTTON) == 0)
			return DRAGDROP_S_DROP;

		dwStopButton = MK_LBUTTON;
	}

	if(fEscapePressed == TRUE || grfKeyState & dwStopButton)
		return DRAGDROP_S_CANCEL;

	return S_OK;
}

HRESULT _stdcall CMyTreeView::GiveFeedback(DWORD dwEffect)
{
	UNREFERENCED_PARAMETER(dwEffect);

	return DRAGDROP_S_USEDEFAULTCURSORS;
}