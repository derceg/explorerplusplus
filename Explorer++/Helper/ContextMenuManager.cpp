// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * This class is designed to show an existing menu, with
 * has been modified to also show menu items inserted
 * by shell extensions.
 */

#include "stdafx.h"
#include "ContextMenuManager.h"
#include "Helper.h"
#include "Macros.h"
#include "MenuHelper.h"
#include "ShellHelper.h"
#include <vector>

const TCHAR ContextMenuManager::CMH_FOLDER_DRAG_AND_DROP[] =
	_T("Folder\\ShellEx\\DragDropHandlers");
const TCHAR ContextMenuManager::CMH_DIRECTORY_BACKGROUND[] =
	_T("Directory\\Background\\shellex\\ContextMenuHandlers");
const TCHAR ContextMenuManager::CMH_DIRECTORY_DRAG_AND_DROP[] =
	_T("Directory\\shellex\\DragDropHandlers");

/* The following steps are taken when showing shell
extensions on an existing menu:
1. Load shell extensions.
2. Build and show menu.
3. Pass selection to shell extension (if necessary).
4. Release shell extensions (also free the DLL's they reside in). */
ContextMenuManager::ContextMenuManager(ContextMenuType contextMenuType,
	PCIDLIST_ABSOLUTE pidlDirectory, IDataObject *pDataObject, IUnknown *pUnkSite,
	const std::vector<std::wstring> &blacklistedCLSIDEntries)
{
	ItemType itemType = GetItemType(pidlDirectory);

	const TCHAR *pszRegContext = nullptr;

	switch (itemType)
	{
	case ItemType::Folder:
		if (contextMenuType == ContextMenuType::DragAndDrop)
		{
			pszRegContext = CMH_FOLDER_DRAG_AND_DROP;
		}
		break;

	case ItemType::Directory:
		if (contextMenuType == ContextMenuType::Background)
		{
			pszRegContext = CMH_DIRECTORY_BACKGROUND;
		}
		else if (contextMenuType == ContextMenuType::DragAndDrop)
		{
			pszRegContext = CMH_DIRECTORY_DRAG_AND_DROP;
		}
		break;
	}

	if (pszRegContext == nullptr)
	{
		return;
	}

	BOOL bRet =
		LoadContextMenuHandlers(pszRegContext, m_ContextMenuHandlers, blacklistedCLSIDEntries);

	if (!bRet)
	{
		return;
	}

	/* Initialize the shell extensions, and extract
	an IContextMenu interface. */
	for (const auto &contextMenuHandler : m_ContextMenuHandlers)
	{
		IShellExtInit *pShellExtInit = nullptr;
		HRESULT hr;

		IUnknown *pUnknown = contextMenuHandler.pUnknown;

		hr = pUnknown->QueryInterface(IID_PPV_ARGS(&pShellExtInit));

		if (SUCCEEDED(hr))
		{
			MenuHandler menuHandler;
			IContextMenu *pContextMenu = nullptr;
			IContextMenu2 *pContextMenu2 = nullptr;
			IContextMenu3 *pContextMenu3 = nullptr;

			pShellExtInit->Initialize(pidlDirectory, pDataObject, nullptr);
			pShellExtInit->Release();

			if (pUnkSite != nullptr)
			{
				IObjectWithSite *pObjectSite = nullptr;

				hr = pUnknown->QueryInterface(IID_PPV_ARGS(&pObjectSite));

				if (SUCCEEDED(hr))
				{
					pObjectSite->SetSite(pUnkSite);
					pObjectSite->Release();
				}
			}

			hr = pUnknown->QueryInterface(IID_PPV_ARGS(&pContextMenu3));
			menuHandler.pContextMenuActual = pContextMenu3;

			if (FAILED(hr) || pContextMenu3 == nullptr)
			{
				hr = pUnknown->QueryInterface(IID_PPV_ARGS(&pContextMenu2));
				menuHandler.pContextMenuActual = pContextMenu2;

				if (FAILED(hr) || pContextMenu2 == nullptr)
				{
					hr = pUnknown->QueryInterface(IID_PPV_ARGS(&pContextMenu));
					menuHandler.pContextMenuActual = pContextMenu;
				}
			}

			menuHandler.pContextMenu = pContextMenu;
			menuHandler.pContextMenu2 = pContextMenu2;
			menuHandler.pContextMenu3 = pContextMenu3;

			menuHandler.uStartID = 0;
			menuHandler.uEndID = 0;

			m_MenuHandlers.push_back(menuHandler);
		}
	}
}

ContextMenuManager::~ContextMenuManager()
{
	/* Release the IContextMenu interfaces. */
	for (auto menuHandler : m_MenuHandlers)
	{
		if (menuHandler.pContextMenuActual != nullptr)
		{
			menuHandler.pContextMenuActual->Release();
		}
	}

	/* ...and free the necessary DLL's. */
	for (auto contextMenuHandler : m_ContextMenuHandlers)
	{
		contextMenuHandler.pUnknown->Release();

		if (contextMenuHandler.hDLL != nullptr)
		{
			FreeLibrary(contextMenuHandler.hDLL);
		}
	}
}

bool ContextMenuManager::ShowMenu(HWND hwnd, HMENU hMenu, UINT uIDPrevious, UINT uMinID,
	UINT uMaxID, const POINT &pt, StatusBar &statusBar)
{
	if (hwnd == nullptr || hMenu == nullptr || uIDPrevious == 0 || uMaxID <= uMinID)
	{
		return false;
	}

	m_uMinID = uMinID;
	m_uMaxID = uMaxID;
	m_pStatusBar = &statusBar;

	AddMenuEntries(hMenu, uIDPrevious, uMinID, uMaxID);

	BOOL bRet = SetWindowSubclass(hwnd, ContextMenuHookProc, CONTEXT_MENU_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this));

	if (!bRet)
	{
		return false;
	}

	UINT uCmd =
		TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_VERTICAL | TPM_RETURNCMD, pt.x,
			pt.y, 0, hwnd, nullptr);

	RemoveWindowSubclass(hwnd, ContextMenuHookProc, CONTEXT_MENU_SUBCLASS_ID);

	if (uCmd != 0)
	{
		if (uCmd >= uMinID && uCmd <= uMaxID)
		{
			InvokeMenuEntry(hwnd, uCmd);
		}
		else
		{
			SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(uCmd, 0), NULL);
		}
	}

	return true;
}

void ContextMenuManager::AddMenuEntries(HMENU hMenu, UINT uIDPrevious, int iMinID, int iMaxID)
{
	/* Shell extension menu items will be inserted
	directly *after* this item. This is used rather
	than a direct position, as some shell extensions
	(such as the "Catalyst Context Menu") insert themselves
	at positions other than the one specified. */
	int iStartPos = GetMenuItemPos(hMenu, uIDPrevious) + 1;

	/* First, add two separators to the menu; these
	will "bracket" the shell extensions. */
	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_FTYPE;
	mii.fType = MFT_SEPARATOR;
	InsertMenuItem(hMenu, iStartPos, TRUE, &mii);
	InsertMenuItem(hMenu, iStartPos + 1, TRUE, &mii);

	iStartPos++;

	int iOffset = 0;

	/* Loop through each of the acquired IContextMenu interfaces and add
	the required menu items. */
	for (auto itr = m_MenuHandlers.begin(); itr != m_MenuHandlers.end(); itr++)
	{
		if (itr->pContextMenuActual != nullptr)
		{
			HRESULT hr = itr->pContextMenuActual->QueryContextMenu(hMenu, iStartPos,
				iMinID + iOffset, iMaxID, CMF_NORMAL | CMF_EXPLORE);

			if (HRESULT_SEVERITY(hr) == SEVERITY_SUCCESS)
			{
				int iCurrentOffset = HRESULT_CODE(hr);

				if (iCurrentOffset > 0)
				{
					/* Need to save ID offsets for this menu handler. */
					itr->uStartID = iMinID + iOffset;
					itr->uEndID = iMinID + iOffset + iCurrentOffset;

					iOffset += iCurrentOffset;

					iStartPos = GetMenuItemPos(hMenu, uIDPrevious) + 2;
				}
			}
		}
	}

	MenuHelper::RemoveDuplicateSeperators(hMenu);
}

LRESULT CALLBACK ContextMenuManager::ContextMenuHookProc(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pcmm = reinterpret_cast<ContextMenuManager *>(dwRefData);

	switch (uMsg)
	{
	case WM_INITMENUPOPUP:
	case WM_MEASUREITEM:
	case WM_DRAWITEM:
	case WM_MENUCHAR:
	{
		LRESULT lRes;
		HRESULT hr = pcmm->HandleMenuMessage(uMsg, wParam, lParam, lRes);

		if (SUCCEEDED(hr))
		{
			if (uMsg == WM_INITMENUPOPUP)
			{
				return 0;
			}
			else if (uMsg == WM_MEASUREITEM || uMsg == WM_DRAWITEM)
			{
				return TRUE;
			}
			else
			{
				return lRes;
			}
		}
	}
	break;

	case WM_MENUSELECT:
	{
		if (HIWORD(wParam) == 0xFFFF && lParam == 0)
		{
			pcmm->m_pStatusBar->HandleStatusBarMenuClose();
		}
		else
		{
			pcmm->m_pStatusBar->HandleStatusBarMenuOpen();

			UINT uCmd = LOWORD(wParam);

			if (uCmd >= pcmm->m_uMinID && uCmd <= pcmm->m_uMaxID)
			{
				TCHAR szHelperText[512];

				HRESULT hr = pcmm->GetMenuHelperText(LOWORD(wParam), szHelperText,
					SIZEOF_ARRAY(szHelperText));

				if (hr == S_OK)
				{
					pcmm->m_pStatusBar->SetPartText(0, szHelperText);
					return 0;
				}
			}
		}
	}
	break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

HRESULT ContextMenuManager::HandleMenuMessage(UINT uMsg, WPARAM wParam, LPARAM lParam,
	LRESULT &lRes)
{
	UINT uItemID = static_cast<UINT>(-1);
	bool bContextMenu3Required = false;

	switch (uMsg)
	{
	case WM_INITMENUPOPUP:
		uItemID = GetMenuItemID(reinterpret_cast<HMENU>(wParam), 0);
		break;

	case WM_MEASUREITEM:
		uItemID = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam)->itemID;
		break;

	case WM_DRAWITEM:
		uItemID = reinterpret_cast<DRAWITEMSTRUCT *>(lParam)->itemID;
		break;

	case WM_MENUCHAR:
		uItemID = GetMenuItemID(reinterpret_cast<HMENU>(lParam), 0);
		bContextMenu3Required = true;
		break;
	}

	HRESULT hr = E_FAIL;

	if (uItemID != -1)
	{
		for (auto menuHandler : m_MenuHandlers)
		{
			if (uItemID >= menuHandler.uStartID && uItemID < menuHandler.uEndID)
			{
				if (menuHandler.pContextMenu3 != nullptr)
				{
					hr = menuHandler.pContextMenu3->HandleMenuMsg2(uMsg, wParam, lParam, &lRes);
				}
				else if (menuHandler.pContextMenu2 != nullptr && !bContextMenu3Required)
				{
					hr = menuHandler.pContextMenu2->HandleMenuMsg(uMsg, wParam, lParam);
				}

				break;
			}
		}
	}

	return hr;
}

/* See http://blogs.msdn.com/b/oldnewthing/archive/2004/09/28/235242.aspx for potential
issues with this. */
HRESULT ContextMenuManager::GetMenuHelperText(UINT uID, TCHAR *szText, UINT cchMax)
{
	HRESULT hr = E_FAIL;

	for (auto menuHandler : m_MenuHandlers)
	{
		if (uID >= menuHandler.uStartID && uID < menuHandler.uEndID)
		{
			if (menuHandler.pContextMenuActual != nullptr)
			{
				hr = menuHandler.pContextMenuActual->GetCommandString(uID - menuHandler.uStartID,
					GCS_HELPTEXT, nullptr, reinterpret_cast<LPSTR>(szText), cchMax);
			}

			break;
		}
	}

	return hr;
}

int ContextMenuManager::GetMenuItemPos(HMENU hMenu, UINT uID)
{
	int iPos = -1;

	for (int i = 0; i < GetMenuItemCount(hMenu); i++)
	{
		if (GetMenuItemID(hMenu, i) == uID)
		{
			iPos = i;
			break;
		}
	}

	return iPos;
}

void ContextMenuManager::InvokeMenuEntry(HWND hwnd, UINT uCmd)
{
	for (auto menuHandler : m_MenuHandlers)
	{
		if (uCmd >= menuHandler.uStartID && uCmd < menuHandler.uEndID)
		{
			CMINVOKECOMMANDINFO cmici;
			cmici.cbSize = sizeof(CMINVOKECOMMANDINFO);
			cmici.fMask = 0;
			cmici.hwnd = hwnd;
			cmici.lpVerb = reinterpret_cast<LPCSTR>(MAKEWORD(uCmd - menuHandler.uStartID, 0));
			cmici.lpParameters = nullptr;
			cmici.lpDirectory = nullptr;
			cmici.nShow = SW_SHOWNORMAL;
			menuHandler.pContextMenuActual->InvokeCommand(&cmici);
			break;
		}
	}
}

ContextMenuManager::ItemType ContextMenuManager::GetItemType(PCIDLIST_ABSOLUTE pidl)
{
	SFGAOF attributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
	GetItemAttributes(pidl, &attributes);

	if ((attributes & SFGAO_FOLDER) && (attributes & SFGAO_FILESYSTEM))
	{
		return ItemType::Directory;
	}
	else if ((attributes & SFGAO_FOLDER) && !(attributes & SFGAO_FILESYSTEM))
	{
		return ItemType::Folder;
	}
	else
	{
		return ItemType::File;
	}
}
