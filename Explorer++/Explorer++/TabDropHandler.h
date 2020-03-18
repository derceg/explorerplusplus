// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/DropHandler.h"

class TabContainer;

class TabDropHandler : public IDropTarget
{
public:
	TabDropHandler(HWND hTabCtrl, TabContainer *tabContainer);
	~TabDropHandler();

	/* IUnknown methods. */
	HRESULT __stdcall QueryInterface(REFIID iid, void **ppvObject) override;
	ULONG __stdcall AddRef() override;
	ULONG __stdcall Release() override;

private:
	static const UINT_PTR SUBCLASS_ID = 1;

	static const int TIMER_ID = 0;
	static const int TIMEOUT_VALUE = 500;

	/* IDropTarget methods. */
	HRESULT __stdcall DragEnter(
		IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
	HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
	HRESULT __stdcall DragLeave() override;
	HRESULT __stdcall Drop(
		IDataObject *pDataObject, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

	static LRESULT CALLBACK TabCtrlProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK TabCtrlProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void GetRepresentativeSourceDrive(IDataObject *pDataObject, CLIPFORMAT Format);
	void GetRepresentativeSourceDriveHDrop(IDataObject *pDataObject);
	DWORD DetermineCurrentDragEffect(int iTab, DWORD grfKeyState, DWORD CurrentDropEffect);

	HWND m_hTabCtrl;
	ULONG m_RefCount;

	TabContainer *m_tabContainer;

	IDragSourceHelper *m_pDragSourceHelper;
	IDropTargetHelper *m_pDropTargetHelper;
	bool m_AcceptData;
	int m_TabHoverIndex;
	DragType m_DragType;
	std::wstring m_RepresentativeDrive;
};