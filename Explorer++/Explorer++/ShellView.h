// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/ShellHelper.h"
#include <wil/com.h>

class ShellBrowser;

// This isn't a complete implementation. There's only enough functionality to support the "New
// Folder" item shown on the background context menu in a phone's virtual folder (when that phone is
// connected via USB).
class ShellView : public IShellView
{
public:
	static wil::com_ptr_nothrow<ShellView> Create(
		PCIDLIST_ABSOLUTE directory, ShellBrowser *shellBrowser);

	// IShellView
	IFACEMETHODIMP TranslateAccelerator(MSG *msg);
	IFACEMETHODIMP EnableModeless(BOOL enable);
	IFACEMETHODIMP UIActivate(UINT state);
	IFACEMETHODIMP Refresh();
	IFACEMETHODIMP CreateViewWindow(IShellView *previous, LPCFOLDERSETTINGS folderSettings,
		IShellBrowser *shellBrowser, RECT *view, HWND *hwnd);
	IFACEMETHODIMP DestroyViewWindow();
	IFACEMETHODIMP GetCurrentInfo(LPFOLDERSETTINGS folderSettings);
	IFACEMETHODIMP AddPropertySheetPages(
		DWORD reserved, LPFNSVADDPROPSHEETPAGE callback, LPARAM lParam);
	IFACEMETHODIMP SaveViewState();
	IFACEMETHODIMP SelectItem(PCUITEMID_CHILD pidlItem, SVSIF flags);
	IFACEMETHODIMP GetItemObject(UINT item, REFIID riid, void **ppv);

	// IOleWindow
	IFACEMETHODIMP GetWindow(HWND *hwnd);
	IFACEMETHODIMP ContextSensitiveHelp(BOOL enterMode);

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

private:
	ShellView(PCIDLIST_ABSOLUTE directory, ShellBrowser *shellBrowser);

	ULONG m_refCount;
	unique_pidl_absolute m_directory;
	ShellBrowser *m_shellBrowser;
};