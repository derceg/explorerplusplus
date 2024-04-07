// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellView.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "TabNavigationInterface.h"

ShellView::ShellView(std::weak_ptr<ShellBrowserImpl> shellBrowserWeak,
	TabNavigationInterface *tabNavigation, bool switchToTabOnSelect) :
	m_shellBrowserWeak(shellBrowserWeak),
	m_tabNavigation(tabNavigation),
	m_switchToTabOnSelect(switchToTabOnSelect)
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

IFACEMETHODIMP ShellView::AddPropertySheetPages(DWORD reserved, LPFNSVADDPROPSHEETPAGE callback,
	LPARAM lParam)
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
	auto shellBrowser = m_shellBrowserWeak.lock();

	if (!shellBrowser)
	{
		return E_FAIL;
	}

	if (flags == SVSI_EDIT)
	{
		auto pidlComplete =
			unique_pidl_absolute(ILCombine(shellBrowser->GetDirectoryIdl().get(), pidlItem));
		shellBrowser->QueueRename(pidlComplete.get());
		return S_OK;
	}
	else if (WI_IsFlagSet(flags, SVSI_SELECT))
	{
		if (m_switchToTabOnSelect)
		{
			m_tabNavigation->SelectTabById(shellBrowser->GetId());
		}

		auto pidlComplete =
			unique_pidl_absolute(ILCombine(shellBrowser->GetDirectoryIdl().get(), pidlItem));
		shellBrowser->SelectItems({ pidlComplete.get() });

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

namespace winrt
{
template <>
bool is_guid_of<IShellView>(guid const &id) noexcept
{
	return is_guid_of<IShellView, IOleWindow>(id);
}
}
