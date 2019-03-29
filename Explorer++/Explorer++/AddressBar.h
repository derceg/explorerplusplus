// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Explorer++_internal.h"
#include "MainToolbar.h"
#include "TabContainerInterface.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/Macros.h"

class AddressBar : public CBaseWindow
{
public:

	static AddressBar *Create(HWND parent, IExplorerplusplus *expp, TabContainerInterface *tabContainer, MainToolbar *mainToolbar);

	void SetText(LPITEMIDLIST pidl, const TCHAR *szDisplayText);

private:

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	AddressBar(HWND parent, IExplorerplusplus *expp, TabContainerInterface *tabContainer, MainToolbar *mainToolbar);
	~AddressBar();

	static HWND CreateAddressBar(HWND parent);

	static LRESULT CALLBACK EditSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	void OnGo();
	void OnBeginDrag();

	IExplorerplusplus *m_expp;
	TabContainerInterface *m_tabContainer;
	MainToolbar *m_mainToolbar;
};