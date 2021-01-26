// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellView.h"
#include "ShellBrowser/ShellBrowser.h"

wil::com_ptr_nothrow<ShellView> ShellView::Create(
	PCIDLIST_ABSOLUTE directory, ShellBrowser *shellBrowser)
{
	wil::com_ptr_nothrow<ShellView> shellView;
	shellView.attach(new ShellView(directory, shellBrowser));
	return shellView;
}

ShellView::ShellView(PCIDLIST_ABSOLUTE directory, ShellBrowser *shellBrowser) :
	m_refCount(1),
	m_directory(ILCloneFull(directory)),
	m_shellBrowser(shellBrowser)
{
}

// IShellView
IFACEMETHODIMP ShellView::TranslateAccelerator(MSG *msg)
{
	UNREFERENCED_PARAMETER(msg);

	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::EnableModeless(BOOL enable)
{
	UNREFERENCED_PARAMETER(enable);

	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::UIActivate(UINT state)
{
	UNREFERENCED_PARAMETER(state);

	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::Refresh()
{
	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::CreateViewWindow(IShellView *previous, LPCFOLDERSETTINGS folderSettings,
	IShellBrowser *shellBrowser, RECT *view, HWND *hwnd)
{
	UNREFERENCED_PARAMETER(previous);
	UNREFERENCED_PARAMETER(folderSettings);
	UNREFERENCED_PARAMETER(shellBrowser);
	UNREFERENCED_PARAMETER(view);
	UNREFERENCED_PARAMETER(hwnd);

	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::DestroyViewWindow()
{
	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::GetCurrentInfo(LPFOLDERSETTINGS folderSettings)
{
	UNREFERENCED_PARAMETER(folderSettings);

	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::AddPropertySheetPages(
	DWORD reserved, LPFNSVADDPROPSHEETPAGE callback, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(reserved);
	UNREFERENCED_PARAMETER(callback);
	UNREFERENCED_PARAMETER(lParam);

	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::SaveViewState()
{
	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::SelectItem(PCUITEMID_CHILD pidlItem, SVSIF flags)
{
	if (flags == SVSI_EDIT)
	{
		auto pidlComplete = unique_pidl_absolute(ILCombine(m_directory.get(), pidlItem));
		m_shellBrowser->QueueRename(pidlComplete.get());
		return S_OK;
	}

	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::GetItemObject(UINT item, REFIID riid, void **ppv)
{
	UNREFERENCED_PARAMETER(item);
	UNREFERENCED_PARAMETER(riid);
	UNREFERENCED_PARAMETER(ppv);

	return E_NOTIMPL;
}

// IOleWindow
IFACEMETHODIMP ShellView::GetWindow(HWND *hwnd)
{
	UNREFERENCED_PARAMETER(hwnd);

	return E_NOTIMPL;
}

IFACEMETHODIMP ShellView::ContextSensitiveHelp(BOOL enterMode)
{
	UNREFERENCED_PARAMETER(enterMode);

	return E_NOTIMPL;
}

// IUnknown
IFACEMETHODIMP ShellView::QueryInterface(REFIID riid, void **ppvObject)
{
	// clang-format off
	static const QITAB qit[] = {
		QITABENT(ShellView, IShellView),
		QITABENT(ShellView, IOleWindow),
		{ nullptr }
	};
	// clang-format on

	return QISearch(this, qit, riid, ppvObject);
}

IFACEMETHODIMP_(ULONG) ShellView::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

IFACEMETHODIMP_(ULONG) ShellView::Release()
{
	ULONG refCount = InterlockedDecrement(&m_refCount);

	if (refCount == 0)
	{
		delete this;
		return 0;
	}

	return refCount;
}