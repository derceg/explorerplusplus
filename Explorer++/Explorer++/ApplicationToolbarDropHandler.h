// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationToolbar.h"

class ApplicationToolbar;

class ApplicationToolbarDropHandler : public IDropTarget
{
public:

	ApplicationToolbarDropHandler(HWND hToolbar, ApplicationToolbar *toolbar);

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

	/* Drag and drop. */
	HRESULT _stdcall	DragEnter(IDataObject *pDataObject,DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragOver(DWORD grfKeyState,POINTL pt,DWORD *pdwEffect);
	HRESULT _stdcall	DragLeave(void);
	HRESULT _stdcall	Drop(IDataObject *pDataObject,DWORD grfKeyState,POINTL ptl,DWORD *pdwEffect);

private:

	static FORMATETC	GetSupportedDropFormat();
	void				AddNewButton(DROPFILES *df);
	void				OpenExistingButton(DROPFILES *df, int buttonIndex);

	ULONG				m_RefCount;

	ApplicationToolbar	*m_toolbar;
	HWND				m_hToolbar;

	/* Drag and drop. */
	IDragSourceHelper	*m_pDragSourceHelper;
	IDropTargetHelper	*m_pDropTargetHelper;
};