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
#include <vector>

LRESULT CALLBACK ShellMenuHookProcStub(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

FileContextMenuManager::FileContextMenuManager(HWND hwnd, PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PCITEMID_CHILD> &pidlItems) :
	m_hwnd(hwnd),
	m_pidlParent(ILCloneFull(pidlParent))
{
	wil::com_ptr_nothrow<IContextMenu> pContextMenu;
	HRESULT hr;

	for (auto pidl : pidlItems)
	{
		m_pidlItems.push_back(ILCloneChild(pidl));
	}

	m_pActualContext = nullptr;

	wil::com_ptr_nothrow<IShellFolder> shellFolder;
	hr = BindToIdl(m_pidlParent.get(), IID_PPV_ARGS(&shellFolder));

	if (FAILED(hr))
	{
		return;
	}

	if (pidlItems.empty())
	{
		wil::com_ptr_nothrow<IShellView> shellView;
		hr = shellFolder->CreateViewObject(m_hwnd, IID_PPV_ARGS(&shellView));

		if (SUCCEEDED(hr))
		{
			hr = shellView->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&pContextMenu));
		}
	}
	else
	{
		std::vector<PCITEMID_CHILD> pidlItemsTemp(pidlItems);
		hr = GetUIObjectOf(shellFolder.get(), hwnd, static_cast<UINT>(pidlItems.size()),
			pidlItemsTemp.data(), IID_PPV_ARGS(&pContextMenu));
	}

	if (SUCCEEDED(hr))
	{
		/* First, try to get IContextMenu3, then IContextMenu2, and if neither of these
		are available, IContextMenu. */
		hr = pContextMenu->QueryInterface(IID_PPV_ARGS(&m_pShellContext3));
		m_pActualContext = m_pShellContext3.get();

		if (FAILED(hr))
		{
			hr = pContextMenu->QueryInterface(IID_PPV_ARGS(&m_pShellContext2));
			m_pActualContext = m_pShellContext2.get();

			if (FAILED(hr))
			{
				hr = pContextMenu->QueryInterface(IID_PPV_ARGS(&m_pShellContext));
				m_pActualContext = m_pShellContext.get();
			}
		}
	}
}

FileContextMenuManager::~FileContextMenuManager()
{
	for (auto pidl : m_pidlItems)
	{
		CoTaskMemFree(pidl);
	}
}

HRESULT FileContextMenuManager::ShowMenu(FileContextMenuHandler *handler, int iMinID, int iMaxID,
	const POINT *ppt, StatusBar *pStatusBar, IUnknown *site, BOOL bRename, BOOL bExtended)
{
	if (m_pActualContext == nullptr)
	{
		return E_FAIL;
	}

	if (handler == nullptr || iMaxID <= iMinID || ppt == nullptr)
	{
		return E_FAIL;
	}

	m_pStatusBar = pStatusBar;

	m_iMinID = iMinID;
	m_iMaxID = iMaxID;

	HMENU hMenu = CreatePopupMenu();

	if (hMenu == nullptr)
	{
		return E_FAIL;
	}

	UINT uFlags = CMF_NORMAL;

	if (bExtended)
	{
		uFlags |= CMF_EXTENDEDVERBS;
	}

	if (bRename)
	{
		uFlags |= CMF_CANRENAME;
	}

	if (m_pidlItems.empty())
	{
		// The background context menu shouldn't have any default item set.
		uFlags |= CMF_NODEFAULT;
	}

	HRESULT hr;

	if (site)
	{
		wil::com_ptr_nothrow<IObjectWithSite> objectWithSite;
		hr = m_pActualContext->QueryInterface(IID_PPV_ARGS(&objectWithSite));

		if (SUCCEEDED(hr))
		{
			objectWithSite->SetSite(site);
		}
	}

	hr = m_pActualContext->QueryContextMenu(hMenu, 0, iMinID, iMaxID, uFlags);

	if (FAILED(hr))
	{
		return hr;
	}

	handler->UpdateMenuEntries(m_pidlParent.get(), m_pidlItems, m_pActualContext, hMenu);

	MenuHelper::RemoveTrailingSeparators(hMenu);
	MenuHelper::RemoveDuplicateSeperators(hMenu);

	BOOL bWindowSubclassed = FALSE;

	if (m_pShellContext3 != nullptr || m_pShellContext2 != nullptr)
	{
		/* Subclass the owner window, so that the shell can handle menu messages. */
		bWindowSubclassed = SetWindowSubclass(m_hwnd, ShellMenuHookProcStub,
			CONTEXT_MENU_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));
	}

	int iCmd =
		TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RETURNCMD, ppt->x, ppt->y, 0, m_hwnd, nullptr);

	if (bWindowSubclassed)
	{
		/* Restore previous window procedure. */
		RemoveWindowSubclass(m_hwnd, ShellMenuHookProcStub, CONTEXT_MENU_SUBCLASS_ID);
	}

	/* Was a shell menu item selected, or one of the
	custom entries? */
	if (iCmd >= iMinID && iCmd <= iMaxID)
	{
		TCHAR verb[64] = _T("");
		hr = m_pActualContext->GetCommandString(iCmd - iMinID, GCS_VERB, nullptr,
			reinterpret_cast<LPSTR>(verb), SIZEOF_ARRAY(verb));

		BOOL bHandled = FALSE;

		// Pass the menu back to the caller to give it the chance to handle it.
		if (SUCCEEDED(hr))
		{
			bHandled = handler->HandleShellMenuItem(m_pidlParent.get(), m_pidlItems, verb);
		}

		if (!bHandled)
		{
			std::optional<std::string> parsingPathOpt = GetFilesystemDirectory();

			// Note that some menu items require the directory field to be set. For example, the
			// "Git Bash Here" item (which is added by Git for Windows) requires that.
			CMINVOKECOMMANDINFO commandInfo = {};
			commandInfo.cbSize = sizeof(commandInfo);
			commandInfo.fMask = 0;
			commandInfo.hwnd = m_hwnd;
			commandInfo.lpVerb = reinterpret_cast<LPCSTR>(MAKEINTRESOURCE(iCmd - iMinID));
			commandInfo.lpDirectory = parsingPathOpt ? parsingPathOpt->c_str() : nullptr;
			commandInfo.nShow = SW_SHOW;
			m_pActualContext->InvokeCommand(&commandInfo);
		}
	}
	else
	{
		/* Custom menu entry, so pass back
		to caller. */
		handler->HandleCustomMenuItem(m_pidlParent.get(), m_pidlItems, iCmd);
	}

	/* Do NOT destroy the menu until AFTER
	the command has been executed. Items
	on the "Send to" submenu may not work,
	for example, if this item is destroyed
	earlier. */
	DestroyMenu(hMenu);

	return S_OK;
}

// Returns the parsing path for the current directory, but only if it's a filesystem path.
std::optional<std::string> FileContextMenuManager::GetFilesystemDirectory()
{
	wil::com_ptr_nothrow<IShellItem> shellItem;
	HRESULT hr = SHCreateItemFromIDList(m_pidlParent.get(), IID_PPV_ARGS(&shellItem));

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

LRESULT CALLBACK ShellMenuHookProcStub(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pfcmm = reinterpret_cast<FileContextMenuManager *>(dwRefData);

	return pfcmm->ShellMenuHookProc(hwnd, Msg, wParam, lParam);
}

LRESULT CALLBACK FileContextMenuManager::ShellMenuHookProc(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MEASUREITEM:
	case WM_DRAWITEM:
	case WM_INITMENUPOPUP:
	case WM_MENUCHAR:
		// wParam is 0 if this item was sent by a menu.
		if ((uMsg == WM_MEASUREITEM || uMsg == WM_DRAWITEM) && wParam != 0)
		{
			break;
		}

		if (m_pShellContext3 != nullptr)
		{
			LRESULT result;
			HRESULT hr = m_pShellContext3->HandleMenuMsg2(uMsg, wParam, lParam, &result);

			if (SUCCEEDED(hr))
			{
				return result;
			}
		}
		else if (m_pShellContext2 != nullptr)
		{
			m_pShellContext2->HandleMenuMsg(uMsg, wParam, lParam);
		}
		break;

	case WM_MENUSELECT:
	{
		if (m_pStatusBar != nullptr)
		{
			if (HIWORD(wParam) == 0xFFFF && lParam == 0)
			{
				m_pStatusBar->HandleStatusBarMenuClose();
			}
			else
			{
				m_pStatusBar->HandleStatusBarMenuOpen();

				int iCmd = static_cast<int>(LOWORD(wParam));

				if (WI_IsFlagSet(HIWORD(wParam), MF_POPUP))
				{
					m_pStatusBar->SetPartText(0, L"");
				}
				else if (iCmd >= m_iMinID && iCmd <= m_iMaxID)
				{
					/* Ask for the help string for the currently selected menu item. */
					TCHAR szHelpString[512];
					HRESULT hr = m_pActualContext->GetCommandString(iCmd - m_iMinID, GCS_HELPTEXT,
						nullptr, reinterpret_cast<LPSTR>(szHelpString), SIZEOF_ARRAY(szHelpString));

					if (FAILED(hr))
					{
						StringCchCopy(szHelpString, SIZEOF_ARRAY(szHelpString), L"");
					}

					m_pStatusBar->SetPartText(0, szHelpString);
				}
			}

			/* Prevent the message from been passed onto the original window. */
			return 0;
		}
	}
	break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}
