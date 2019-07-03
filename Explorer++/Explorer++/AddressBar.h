// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "MainToolbar.h"
#include "Navigation.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/Macros.h"

class AddressBar : public CBaseWindow
{
public:

	static AddressBar *Create(HWND parent, IExplorerplusplus *expp,
		Navigation *navigation, MainToolbar *mainToolbar);

private:

	static const UINT_PTR SUBCLASS_ID = 0;
	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	AddressBar(HWND parent, IExplorerplusplus *expp, Navigation *navigation,
		MainToolbar *mainToolbar);
	~AddressBar();

	static HWND CreateAddressBar(HWND parent);

	static LRESULT CALLBACK EditSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK EditSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK ParentWndProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Initialize(HWND parent);
	void OnGo();
	void OnBeginDrag();
	void OnTabSelected(const Tab &tab);
	void OnNavigationCompleted(const Tab &tab);
	void UpdateTextAndIcon(const Tab &tab);

	IExplorerplusplus *m_expp;
	Navigation *m_navigation;
	MainToolbar *m_mainToolbar;

	boost::signals2::connection m_tabSelectedConnection;
	boost::signals2::connection m_navigationCompletedConnection;
};