// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellContextMenu.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/StringHelper.h"
#include <boost/algorithm/string/predicate.hpp>

ShellContextMenu::ShellContextMenu(MenuView *menuView, const AcceleratorManager *acceleratorManager,
	const PidlAbsolute &directory, HWND hwnd) :
	MenuBase(menuView, acceleratorManager),
	m_directory(directory),
	m_hwnd(hwnd)
{
	m_rootMenuView->SetExternalItemDelegate(this);
}

void ShellContextMenu::AppendShellItems(IUnknown *site, UINT flags)
{
	m_contextMenu = MaybeGetShellContextMenu(m_hwnd);

	if (!m_contextMenu)
	{
		return;
	}

	m_resetSite = wil::com_set_site(m_contextMenu.get(), site);
	m_contextMenu->QueryContextMenu(m_rootMenuView->GetNativeMenuForExternalItems(),
		m_rootMenuView->GetNumItems(), MenuView::EXTERNAL_ID_MIN, MenuView::EXTERNAL_ID_MAX, flags);
}

std::optional<UINT> ShellContextMenu::MaybeGetShellItemIndex(const std::wstring &verb) const
{
	if (!m_contextMenu)
	{
		return std::nullopt;
	}

	HMENU menu = m_rootMenuView->GetNativeMenuForExternalItems();

	int numItems = GetMenuItemCount(menu);
	CHECK_NE(numItems, -1);

	for (int i = 0; i < numItems; i++)
	{
		MENUITEMINFO menuItemInfo = {};
		menuItemInfo.cbSize = sizeof(menuItemInfo);
		menuItemInfo.fMask = MIIM_ID | MIIM_FTYPE;
		auto res = GetMenuItemInfo(menu, i, true, &menuItemInfo);

		if (!res || WI_IsFlagSet(menuItemInfo.fType, MFT_SEPARATOR)
			|| menuItemInfo.wID < MenuView::EXTERNAL_ID_MIN
			|| menuItemInfo.wID > MenuView::EXTERNAL_ID_MAX)
		{
			continue;
		}

		wchar_t currentVerb[64] = L"";
		HRESULT hr = m_contextMenu->GetCommandString(menuItemInfo.wID - MenuView::EXTERNAL_ID_MIN,
			GCS_VERBW, nullptr, reinterpret_cast<LPSTR>(currentVerb), std::size(currentVerb));

		if (FAILED(hr))
		{
			continue;
		}

		if (boost::iequals(currentVerb, verb))
		{
			return i;
		}
	}

	return std::nullopt;
}

void ShellContextMenu::RemoveShellItems(const std::vector<std::wstring> &verbsToRemove)
{
	for (const auto &verb : verbsToRemove)
	{
		RemoveShellItem(verb);
	}
}

void ShellContextMenu::RemoveShellItem(const std::wstring &verb)
{
	auto index = MaybeGetShellItemIndex(verb);

	if (!index)
	{
		return;
	}

	HMENU menu = m_rootMenuView->GetNativeMenuForExternalItems();
	auto res = DeleteMenu(menu, *index, MF_BYPOSITION);
	CHECK(res);
}

void ShellContextMenu::RemoveRedundantSeparators()
{
	HMENU menu = m_rootMenuView->GetNativeMenuForExternalItems();
	MenuHelper::RemoveTrailingSeparators(menu);
	MenuHelper::RemoveDuplicateSeparators(menu);
}

std::optional<LRESULT> ShellContextMenu::MaybeHandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MEASUREITEM:
	case WM_DRAWITEM:
	case WM_INITMENUPOPUP:
	case WM_MENUCHAR:
		// wParam is 0 if this item was sent by a menu.
		if ((msg == WM_MEASUREITEM || msg == WM_DRAWITEM) && wParam != 0)
		{
			break;
		}

		if (!m_contextMenu)
		{
			break;
		}

		if (auto contextMenu3 = m_contextMenu.try_query<IContextMenu3>())
		{
			LRESULT result;
			HRESULT hr = contextMenu3->HandleMenuMsg2(msg, wParam, lParam, &result);

			if (SUCCEEDED(hr))
			{
				return result;
			}
		}
		else if (auto contextMenu2 = m_contextMenu.try_query<IContextMenu2>())
		{
			HRESULT hr = contextMenu2->HandleMenuMsg(msg, wParam, lParam);

			if (SUCCEEDED(hr))
			{
				switch (msg)
				{
				case WM_DRAWITEM:
				case WM_MEASUREITEM:
					return TRUE;

				case WM_INITMENUPOPUP:
					return 0;
				}
			}
		}
		break;
	}

	return std::nullopt;
}

std::optional<std::wstring> ShellContextMenu::MaybeGetExternalItemHelpText(UINT id) const
{
	CHECK(m_contextMenu);

	wchar_t helpText[512] = L"";
	HRESULT hr = m_contextMenu->GetCommandString(id - MenuView::EXTERNAL_ID_MIN, GCS_HELPTEXTW,
		nullptr, reinterpret_cast<LPSTR>(helpText), std::size(helpText));

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return helpText;
}

void ShellContextMenu::OnExternalItemSelected(UINT id)
{
	// If an external item was selected, m_contextMenu should be set (since only it can add items in
	// the external item range).
	CHECK(m_contextMenu);

	wchar_t verb[64] = L"";
	HRESULT hr = m_contextMenu->GetCommandString(id - MenuView::EXTERNAL_ID_MIN, GCS_VERBW, nullptr,
		reinterpret_cast<LPSTR>(verb), std::size(verb));

	// Allow a derived class to potentially handle the item first.
	if (SUCCEEDED(hr) && MaybeHandleShellMenuItem(verb))
	{
		return;
	}

	auto parsingPath = MaybeGetFilesystemDirectory();

	std::optional<std::string> narrowParsingPath;

	if (parsingPath)
	{
		// The documentation for CMIC_MASK_UNICODE (see
		// https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/ns-shobjidl_core-cminvokecommandinfoex#cmic_mask_unicode)
		// explicitly notes that ANSI strings should still be passed even when the CMIC_MASK_UNICODE
		// flag is supplied, which is the reason for the conversion here. Depending on the current
		// code page, the path may not be representable as a narrow string.
		narrowParsingPath = WstrToStr(*parsingPath);
	}

	UINT commandOffset = id - MenuView::EXTERNAL_ID_MIN;

	// Note that some menu items require the directory field to be set. For example, the "Git Bash
	// Here" item (which is added by Git for Windows) requires that.
	CMINVOKECOMMANDINFOEX commandInfo = {};
	commandInfo.cbSize = sizeof(commandInfo);
	commandInfo.fMask = CMIC_MASK_UNICODE;
	commandInfo.hwnd = m_hwnd;
	commandInfo.lpVerb = MAKEINTRESOURCEA(commandOffset);
	commandInfo.lpVerbW = MAKEINTRESOURCEW(commandOffset);
	commandInfo.lpDirectoryW = parsingPath ? parsingPath->c_str() : nullptr;
	commandInfo.lpDirectory = narrowParsingPath ? narrowParsingPath->c_str() : nullptr;
	commandInfo.nShow = SW_SHOWNORMAL;
	hr = m_contextMenu->InvokeCommand(reinterpret_cast<CMINVOKECOMMANDINFO *>(&commandInfo));
	DCHECK(SUCCEEDED(hr));
}

bool ShellContextMenu::MaybeHandleShellMenuItem(const std::wstring &verb)
{
	UNREFERENCED_PARAMETER(verb);

	return false;
}

// Returns the parsing path for the current directory, but only if it's a filesystem path.
std::optional<std::wstring> ShellContextMenu::MaybeGetFilesystemDirectory() const
{
	if (!m_directory.HasValue())
	{
		// The directory can be empty in tests.
		return std::nullopt;
	}

	wil::com_ptr_nothrow<IShellItem> shellItem;
	HRESULT hr = SHCreateItemFromIDList(m_directory.Raw(), IID_PPV_ARGS(&shellItem));

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	wil::unique_cotaskmem_string parsingPath;
	hr = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &parsingPath);

	if (SUCCEEDED(hr))
	{
		return parsingPath.get();
	}

	// In Windows Explorer, it appears that when a menu item is invoked from the background context
	// menu in a library folder, the directory that's used is the default directory for that
	// library. The functionality here tries to mimic that behavior.
	//
	// Without this, menu items like "Git Bash Here" wouldn't work when invoked from a library
	// directory (since a library doesn't have a filesystem path, even though it may be filesystem
	// backed).
	wil::com_ptr_nothrow<IShellLibrary> shellLibrary;
	hr = SHLoadLibraryFromItem(shellItem.get(), STGM_READ, IID_PPV_ARGS(&shellLibrary));

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	wil::com_ptr_nothrow<IShellItem> defaultLocation;
	hr = shellLibrary->GetDefaultSaveFolder(DSFT_DETECT, IID_PPV_ARGS(&defaultLocation));

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	hr = defaultLocation->GetDisplayName(SIGDN_FILESYSPATH, &parsingPath);

	if (FAILED(hr))
	{
		return std::nullopt;
	}

	return parsingPath.get();
}
