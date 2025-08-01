// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellContextMenu.h"
#include "MenuHelpTextHost.h"
#include "MenuHelper.h"
#include "ShellContextMenuBuilder.h"
#include "ShellContextMenuDelegate.h"
#include "ShellContextMenuIdRemapper.h"
#include "ShellHelper.h"
#include "StringHelper.h"
#include "WindowSubclass.h"

ShellContextMenu::ShellContextMenu(PCIDLIST_ABSOLUTE directory,
	const std::vector<PCITEMID_CHILD> &items, MenuHelpTextHost *menuHelpTextHost) :
	m_directory(directory),
	m_items(items.begin(), items.end()),
	m_menuHelpTextHost(menuHelpTextHost),
	m_idGenerator(MAX_SHELL_MENU_ID + 1)
{
}

ShellContextMenu::~ShellContextMenu() = default;

void ShellContextMenu::AddDelegate(ShellContextMenuDelegate *delegate)
{
	m_delegates.push_back(delegate);

	auto [itr, didInsert] = m_delegateToIdRemapperMap.insert(
		{ delegate, std::make_unique<ShellContextMenuIdRemapper>(delegate, &m_idGenerator) });
	DCHECK(didInsert);
}

void ShellContextMenu::ShowMenu(HWND hwnd, const POINT *pt, IUnknown *site, UINT flags)
{
	wil::unique_hmenu menu(CreatePopupMenu());

	m_contextMenu = MaybeGetShellContextMenu(hwnd);
	wil::unique_set_site_null_call resetSite;

	if (m_contextMenu)
	{
		resetSite = wil::com_set_site(m_contextMenu.get(), site);
		m_contextMenu->QueryContextMenu(menu.get(), 0, MIN_SHELL_MENU_ID, MAX_SHELL_MENU_ID, flags);
	}

	UpdateMenuEntries(menu.get());

	MenuHelper::RemoveTrailingSeparators(menu.get());
	MenuHelper::RemoveDuplicateSeperators(menu.get());

	if (GetMenuItemCount(menu.get()) == 0)
	{
		// If the folder doesn't provide any IContextMenu instance, the application can still add
		// items. If, however, the application doesn't add any items either, there's no menu to show
		// and it doesn't make sense to continue.
		return;
	}

	// Subclass the owner window, so that menu messages can be handled.
	auto subclass = std::make_unique<WindowSubclass>(hwnd,
		std::bind_front(&ShellContextMenu::ParentWindowSubclass, this));

	auto helpTextConnection = m_menuHelpTextHost->AddMenuHelpTextRequestObserver(
		std::bind_front(&ShellContextMenu::MaybeGetMenuHelpText, this, menu.get()));

	UINT cmd =
		TrackPopupMenu(menu.get(), TPM_LEFTALIGN | TPM_RETURNCMD, pt->x, pt->y, 0, hwnd, nullptr);

	helpTextConnection.disconnect();

	// When the selected command is invoked below, it could potentially do anything, including show
	// another menu. It doesn't make sense for the subclass to be called in that situation and
	// there's no need for the subclass to remain in place once the menu has closed anyway.
	subclass.reset();

	if (cmd == 0)
	{
		return;
	}

	if (cmd >= MIN_SHELL_MENU_ID && cmd <= MAX_SHELL_MENU_ID)
	{
		// Items in this range are assigned to shell menu items. If this CHECK is triggered, it
		// indicates that the application has incorrectly added items within this range.
		CHECK(m_contextMenu);

		wchar_t verb[64] = _T("");
		HRESULT hr = m_contextMenu->GetCommandString(cmd - MIN_SHELL_MENU_ID, GCS_VERB, nullptr,
			reinterpret_cast<LPSTR>(verb), std::size(verb));

		bool handled = false;

		// Pass the item to the delegates to give one of them the chance to handle it.
		if (SUCCEEDED(hr))
		{
			handled = MaybeHandleShellMenuItem(verb);
		}

		if (!handled)
		{
			std::optional<std::string> parsingPathOpt = MaybeGetFilesystemDirectory();

			// Note that some menu items require the directory field to be set. For example, the
			// "Git Bash Here" item (which is added by Git for Windows) requires that.
			CMINVOKECOMMANDINFO commandInfo = {};
			commandInfo.cbSize = sizeof(commandInfo);
			commandInfo.fMask = 0;
			commandInfo.hwnd = hwnd;
			commandInfo.lpVerb = reinterpret_cast<LPCSTR>(MAKEINTRESOURCE(cmd - MIN_SHELL_MENU_ID));
			commandInfo.lpDirectory = parsingPathOpt ? parsingPathOpt->c_str() : nullptr;
			commandInfo.nShow = SW_SHOWNORMAL;
			m_contextMenu->InvokeCommand(&commandInfo);
		}
	}
	else if (auto *delegate = m_idGenerator.MaybeGetDelegateForId(cmd))
	{
		const auto *idRemapper = GetIdRemapperForDelegate(delegate);
		delegate->HandleCustomMenuItem(m_directory.Raw(), m_items, idRemapper->GetOriginalId(cmd));
	}
}

void ShellContextMenu::UpdateMenuEntries(HMENU menu)
{
	for (auto *delegate : m_delegates)
	{
		ShellContextMenuBuilder builder(menu, m_contextMenu.get(),
			GetIdRemapperForDelegate(delegate));
		delegate->UpdateMenuEntries(m_directory.Raw(), m_items, &builder);
	}
}

bool ShellContextMenu::MaybeHandleShellMenuItem(const std::wstring &verb)
{
	for (auto *delegate : m_delegates)
	{
		if (delegate->MaybeHandleShellMenuItem(m_directory.Raw(), m_items, verb))
		{
			return true;
		}
	}

	return false;
}

// Returns the parsing path for the current directory, but only if it's a filesystem path.
std::optional<std::string> ShellContextMenu::MaybeGetFilesystemDirectory() const
{
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
		return WstrToStr(parsingPath.get());
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

	return WstrToStr(parsingPath.get());
}

LRESULT ShellContextMenu::ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_MENUSELECT:
		m_menuHelpTextHost->MenuItemSelected(reinterpret_cast<HMENU>(lParam), LOWORD(wParam),
			HIWORD(wParam));
		break;

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
			contextMenu2->HandleMenuMsg(msg, wParam, lParam);
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

std::optional<std::wstring> ShellContextMenu::MaybeGetMenuHelpText(HMENU shellContextMenu,
	HMENU menu, int id)
{
	if (!MenuHelper::IsPartOfMenu(shellContextMenu, menu))
	{
		return std::nullopt;
	}

	if (id >= MIN_SHELL_MENU_ID && id <= MAX_SHELL_MENU_ID)
	{
		CHECK(m_contextMenu);

		wchar_t helpText[512];
		HRESULT hr = m_contextMenu->GetCommandString(id - MIN_SHELL_MENU_ID, GCS_HELPTEXT, nullptr,
			reinterpret_cast<LPSTR>(helpText), std::size(helpText));

		if (FAILED(hr))
		{
			StringCchCopy(helpText, std::size(helpText), L"");
		}

		return helpText;
	}

	if (auto *delegate = m_idGenerator.MaybeGetDelegateForId(id))
	{
		const auto *idRemapper = GetIdRemapperForDelegate(delegate);
		return delegate->GetHelpTextForCustomItem(idRemapper->GetOriginalId(id));
	}

	return std::nullopt;
}

ShellContextMenuIdRemapper *ShellContextMenu::GetIdRemapperForDelegate(
	ShellContextMenuDelegate *delegate)
{
	auto itr = m_delegateToIdRemapperMap.find(delegate);
	CHECK(itr != m_delegateToIdRemapperMap.end());
	return itr->second.get();
}
