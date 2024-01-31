// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileContextMenuManager.h"
#include "Macros.h"
#include "MenuHelper.h"
#include "ShellHelper.h"
#include "StatusBar.h"
#include "StringHelper.h"
#include "WindowSubclassWrapper.h"

FileContextMenuManager::FileContextMenuManager(HWND hwnd, PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PCITEMID_CHILD> &pidlItems) :
	m_hwnd(hwnd),
	m_pidlParent(pidlParent)
{
	std::copy(pidlItems.begin(), pidlItems.end(), std::back_inserter(m_pidlItems));

	m_actualContextMenu = nullptr;

	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	HRESULT hr = BindToIdl(m_pidlParent.Raw(), IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return;
	}

	wil::com_ptr_nothrow<IContextMenu> contextMenu;

	if (pidlItems.empty())
	{
		wil::com_ptr_nothrow<IShellView> shellView;
		hr = shellFolder->CreateViewObject(m_hwnd, IID_PPV_ARGS(&shellView));

		if (SUCCEEDED(hr))
		{
			hr = shellView->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&contextMenu));
		}
	}
	else
	{
		hr = GetUIObjectOf(shellFolder.get(), hwnd, static_cast<UINT>(pidlItems.size()),
			pidlItems.data(), IID_PPV_ARGS(&contextMenu));
	}

	if (FAILED(hr))
	{
		return;
	}

	// First, try to get IContextMenu3, then IContextMenu2, and if neither of these are
	// available, then use IContextMenu.
	hr = contextMenu->QueryInterface(IID_PPV_ARGS(&m_contextMenu3));

	if (SUCCEEDED(hr))
	{
		m_actualContextMenu = m_contextMenu3.get();
		return;
	}

	hr = contextMenu->QueryInterface(IID_PPV_ARGS(&m_contextMenu2));

	if (SUCCEEDED(hr))
	{
		m_actualContextMenu = m_contextMenu2.get();
		return;
	}

	m_contextMenu = contextMenu;
	m_actualContextMenu = m_contextMenu.get();
}

HRESULT FileContextMenuManager::ShowMenu(FileContextMenuHandler *handler, const POINT *pt,
	StatusBar *statusBar, IUnknown *site, Flags flags)
{
	if (!m_actualContextMenu)
	{
		return E_FAIL;
	}

	m_statusBar = statusBar;

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

	HRESULT hr;

	if (site)
	{
		wil::com_ptr_nothrow<IObjectWithSite> objectWithSite;
		hr = m_actualContextMenu->QueryInterface(IID_PPV_ARGS(&objectWithSite));

		if (SUCCEEDED(hr))
		{
			objectWithSite->SetSite(site);
		}
	}

	hr = m_actualContextMenu->QueryContextMenu(menu.get(), 0, MIN_SHELL_MENU_ID, MAX_SHELL_MENU_ID,
		contextMenuflags);

	if (FAILED(hr))
	{
		return hr;
	}

	handler->UpdateMenuEntries(menu.get(), m_pidlParent.Raw(), m_pidlItems, m_actualContextMenu);

	MenuHelper::RemoveTrailingSeparators(menu.get());
	MenuHelper::RemoveDuplicateSeperators(menu.get());

	// Subclass the owner window, so that menu messages can be handled.
	auto subclass = std::make_unique<WindowSubclassWrapper>(m_hwnd,
		std::bind_front(&FileContextMenuManager::ParentWindowSubclass, this));

	int cmd =
		TrackPopupMenu(menu.get(), TPM_LEFTALIGN | TPM_RETURNCMD, pt->x, pt->y, 0, m_hwnd, nullptr);

	// When the selected command is invoked below, it could potentially do anything, including show
	// another menu. It doesn't make sense for the subclass to be called in that situation and
	// there's no need for the subclass to remain in place once the menu has closed anyway.
	subclass.reset();

	if (cmd == 0)
	{
		return S_OK;
	}

	if (cmd >= MIN_SHELL_MENU_ID && cmd <= MAX_SHELL_MENU_ID)
	{
		TCHAR verb[64] = _T("");
		hr = m_actualContextMenu->GetCommandString(cmd - MIN_SHELL_MENU_ID, GCS_VERB, nullptr,
			reinterpret_cast<LPSTR>(verb), SIZEOF_ARRAY(verb));

		bool handled = false;

		// Pass the menu back to the caller to give it the chance to handle it.
		if (SUCCEEDED(hr))
		{
			handled = handler->HandleShellMenuItem(m_pidlParent.Raw(), m_pidlItems, verb);
		}

		if (!handled)
		{
			std::optional<std::string> parsingPathOpt = GetFilesystemDirectory();

			// Note that some menu items require the directory field to be set. For example, the
			// "Git Bash Here" item (which is added by Git for Windows) requires that.
			CMINVOKECOMMANDINFO commandInfo = {};
			commandInfo.cbSize = sizeof(commandInfo);
			commandInfo.fMask = 0;
			commandInfo.hwnd = m_hwnd;
			commandInfo.lpVerb = reinterpret_cast<LPCSTR>(MAKEINTRESOURCE(cmd - MIN_SHELL_MENU_ID));
			commandInfo.lpDirectory = parsingPathOpt ? parsingPathOpt->c_str() : nullptr;
			commandInfo.nShow = SW_SHOW;
			m_actualContextMenu->InvokeCommand(&commandInfo);
		}
	}
	else
	{
		handler->HandleCustomMenuItem(m_pidlParent.Raw(), m_pidlItems, cmd);
	}

	return S_OK;
}

// Returns the parsing path for the current directory, but only if it's a filesystem path.
std::optional<std::string> FileContextMenuManager::GetFilesystemDirectory()
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
		return wstrToStr(parsingPath.get());
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

	return wstrToStr(parsingPath.get());
}

LRESULT FileContextMenuManager::ParentWindowSubclass(HWND hwnd, UINT msg, WPARAM wParam,
	LPARAM lParam)
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

		if (m_contextMenu3)
		{
			LRESULT result;
			HRESULT hr = m_contextMenu3->HandleMenuMsg2(msg, wParam, lParam, &result);

			if (SUCCEEDED(hr))
			{
				return result;
			}
		}
		else if (m_contextMenu2)
		{
			m_contextMenu2->HandleMenuMsg(msg, wParam, lParam);
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

			int cmd = static_cast<int>(LOWORD(wParam));

			if (WI_IsFlagSet(HIWORD(wParam), MF_POPUP))
			{
				m_statusBar->SetPartText(0, L"");
			}
			else if (cmd >= MIN_SHELL_MENU_ID && cmd <= MAX_SHELL_MENU_ID)
			{
				TCHAR helpString[512];
				HRESULT hr =
					m_actualContextMenu->GetCommandString(cmd - MIN_SHELL_MENU_ID, GCS_HELPTEXT,
						nullptr, reinterpret_cast<LPSTR>(helpString), SIZEOF_ARRAY(helpString));

				if (FAILED(hr))
				{
					StringCchCopy(helpString, SIZEOF_ARRAY(helpString), L"");
				}

				m_statusBar->SetPartText(0, helpString);
			}
		}

		// Prevent the message from been passed onto the original window.
		return 0;
	}
	break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}
