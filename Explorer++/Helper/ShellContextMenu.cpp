// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ShellContextMenu.h"
#include "MenuHelper.h"
#include "ShellHelper.h"
#include "StatusBar.h"
#include "StringHelper.h"
#include "WindowSubclass.h"

ShellContextMenu::ShellContextMenu(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PCITEMID_CHILD> &pidlItems, ShellContextMenuHandler *handler,
	StatusBar *statusBar) :
	m_pidlParent(pidlParent),
	m_pidlItems(pidlItems.begin(), pidlItems.end()),
	m_handler(handler),
	m_statusBar(statusBar)
{
}

void ShellContextMenu::ShowMenu(HWND hwnd, const POINT *pt, IUnknown *site, Flags flags)
{
	wil::unique_hmenu menu(CreatePopupMenu());

	UINT contextMenuflags = CMF_NORMAL;

	if (WI_IsFlagSet(flags, Flags::ExtendedVerbs))
	{
		contextMenuflags |= CMF_EXTENDEDVERBS;
	}

	if (WI_IsFlagSet(flags, Flags::Rename))
	{
		// The rename item shouldn't be added to the background context menu.
		assert(!m_pidlItems.empty());

		contextMenuflags |= CMF_CANRENAME;
	}

	if (m_pidlItems.empty())
	{
		// The background context menu shouldn't have any default item set.
		contextMenuflags |= CMF_NODEFAULT;
	}

	m_contextMenu = MaybeGetShellContextMenu(hwnd);

	if (m_contextMenu)
	{
		if (site; auto objectWithSite = m_contextMenu.try_query<IObjectWithSite>())
		{
			objectWithSite->SetSite(site);
		}

		m_contextMenu->QueryContextMenu(menu.get(), 0, MIN_SHELL_MENU_ID, MAX_SHELL_MENU_ID,
			contextMenuflags);
	}

	m_handler->UpdateMenuEntries(menu.get(), m_pidlParent.Raw(), m_pidlItems, m_contextMenu.get());

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

	UINT cmd =
		TrackPopupMenu(menu.get(), TPM_LEFTALIGN | TPM_RETURNCMD, pt->x, pt->y, 0, hwnd, nullptr);

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

		TCHAR verb[64] = _T("");
		HRESULT hr = m_contextMenu->GetCommandString(cmd - MIN_SHELL_MENU_ID, GCS_VERB, nullptr,
			reinterpret_cast<LPSTR>(verb), static_cast<UINT>(std::size(verb)));

		bool handled = false;

		// Pass the menu back to the caller to give it the chance to handle it.
		if (SUCCEEDED(hr))
		{
			handled = m_handler->HandleShellMenuItem(m_pidlParent.Raw(), m_pidlItems, verb);
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
	else
	{
		m_handler->HandleCustomMenuItem(m_pidlParent.Raw(), m_pidlItems, cmd);
	}
}

// It's possible for a folder to not provide any IContextMenu instance (for example, the Home folder
// in Windows 11 doesn't provide any IContextMenu instance for the background menu). So, this method
// may return null.
wil::com_ptr_nothrow<IContextMenu> ShellContextMenu::MaybeGetShellContextMenu(HWND hwnd) const
{
	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = BindToIdl(m_pidlParent.Raw(), IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return nullptr;
	}

	wil::com_ptr_nothrow<IContextMenu> contextMenu;

	if (m_pidlItems.empty())
	{
		hr = shellFolder->CreateViewObject(hwnd, IID_PPV_ARGS(&contextMenu));
	}
	else
	{
		std::vector<PCITEMID_CHILD> pidlItemsRaw;
		std::transform(m_pidlItems.begin(), m_pidlItems.end(), std::back_inserter(pidlItemsRaw),
			[](const PidlChild &pidl) { return pidl.Raw(); });

		hr = GetUIObjectOf(shellFolder.get(), hwnd, static_cast<UINT>(pidlItemsRaw.size()),
			pidlItemsRaw.data(), IID_PPV_ARGS(&contextMenu));
	}

	if (FAILED(hr))
	{
		return nullptr;
	}

	return contextMenu;
}

// Returns the parsing path for the current directory, but only if it's a filesystem path.
std::optional<std::string> ShellContextMenu::MaybeGetFilesystemDirectory() const
{
	wil::com_ptr_nothrow<IShellItem> shellItem;
	HRESULT hr = SHCreateItemFromIDList(m_pidlParent.Raw(), IID_PPV_ARGS(&shellItem));

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

	case WM_MENUSELECT:
	{
		if (!m_statusBar)
		{
			break;
		}

		if (HIWORD(wParam) == 0xFFFF && lParam == 0)
		{
			m_statusBar->HandleStatusBarMenuClose();
		}
		else
		{
			m_statusBar->HandleStatusBarMenuOpen();

			UINT menuItemId = LOWORD(wParam);

			if (WI_IsAnyFlagSet(HIWORD(wParam), MF_POPUP | MF_SEPARATOR))
			{
				m_statusBar->SetPartText(0, L"");
			}
			else if (menuItemId >= MIN_SHELL_MENU_ID && menuItemId <= MAX_SHELL_MENU_ID)
			{
				CHECK(m_contextMenu);

				TCHAR helpString[512];
				HRESULT hr = m_contextMenu->GetCommandString(menuItemId - MIN_SHELL_MENU_ID,
					GCS_HELPTEXT, nullptr, reinterpret_cast<LPSTR>(helpString),
					static_cast<UINT>(std::size(helpString)));

				if (FAILED(hr))
				{
					StringCchCopy(helpString, std::size(helpString), L"");
				}

				m_statusBar->SetPartText(0, helpString);
			}
			else
			{
				std::wstring helpString = m_handler->GetHelpTextForItem(menuItemId);
				m_statusBar->SetPartText(0, helpString.c_str());
			}
		}

		// Prevent the message from been passed onto the original window.
		return 0;
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
