// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "IDropFilesCallback.h"
#include "iServiceProvider.h"
#include "ListViewEdit.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "MassRenameDialog.h"
#include "MenuRanges.h"
#include "Navigation.h"
#include "ResourceHelper.h"
#include "SetFileAttributesDialog.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/ViewModes.h"
#include "ViewModeHelper.h"
#include "../Helper/BulkClipboardWriter.h"
#include "../Helper/ContextMenuManager.h"
#include "../Helper/Controls.h"
#include "../Helper/DropHandler.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Helper.h"
#include "../Helper/iDataObject.h"
#include "../Helper/iDropSource.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>

LRESULT CALLBACK Explorerplusplus::ListViewProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	Explorerplusplus *pexpp = reinterpret_cast<Explorerplusplus *>(dwRefData);

	return pexpp->ListViewSubclassProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::ListViewSubclassProc(HWND ListView, UINT msg,
	WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_MENUSELECT:
			SendMessage(m_hContainer,WM_MENUSELECT,wParam,lParam);
			break;

		case WM_SETFOCUS:
			m_hLastActiveWindow = ListView;
			m_mainToolbar->UpdateToolbarButtonStates();
			break;

		case WM_LBUTTONDBLCLK:
			{
				LV_HITTESTINFO	ht;
				DWORD			dwPos;
				POINT			MousePos;

				dwPos = GetMessagePos();
				MousePos.x = GET_X_LPARAM(dwPos);
				MousePos.y = GET_Y_LPARAM(dwPos);
				ScreenToClient(m_hActiveListView,&MousePos);

				ht.pt = MousePos;
				ListView_HitTest(ListView,&ht);

				/* NM_DBLCLK for the listview is sent both on double clicks
				(by default), as well as in the situation when LVS_EX_ONECLICKACTIVATE
				is active (in which case it is sent on a single mouse click).
				Therefore, because we only want to navigate up one folder on
				a DOUBLE click, we'll handle the event here. */
				if(ht.flags == LVHT_NOWHERE)
				{
					/* The user has double clicked in the whitespace
					area for this tab, so go up one folder... */
					m_navigation->OnNavigateUp();
					return 0;
				}
			}
			break;

		case WM_RBUTTONDOWN:
			if((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON)
				&& !(wParam & MK_MBUTTON))
			{
				LV_HITTESTINFO lvhti;

				lvhti.pt.x	= LOWORD(lParam);
				lvhti.pt.y	= HIWORD(lParam);

				/* Test to see if the mouse click was
				on an item or not. */
				ListView_HitTest(m_hActiveListView,&lvhti);

				HDC hdc;
				TCHAR szText[MAX_PATH];
				RECT rc;
				SIZE sz;

				UINT uViewMode = m_pActiveShellBrowser->GetViewMode();

				if(uViewMode == ViewMode::List)
				{
					if(!(lvhti.flags & LVHT_NOWHERE) && lvhti.iItem != -1)
					{
						ListView_GetItemRect(m_hActiveListView,lvhti.iItem,&rc,LVIR_LABEL);
						ListView_GetItemText(m_hActiveListView,lvhti.iItem,0,szText,SIZEOF_ARRAY(szText));

						hdc = GetDC(m_hActiveListView);
						GetTextExtentPoint32(hdc,szText,lstrlen(szText),&sz);
						ReleaseDC(m_hActiveListView,hdc);

						rc.right = rc.left + sz.cx;

						if(!PtInRect(&rc,lvhti.pt))
						{
							m_blockNextListViewSelection = TRUE;
						}
					}
				}

				if(!(lvhti.flags & LVHT_NOWHERE))
				{
					m_bDragAllowed = TRUE;
				}
			}
			break;

		case WM_RBUTTONUP:
			m_bDragCancelled = FALSE;
			m_bDragAllowed = FALSE;

			m_blockNextListViewSelection = FALSE;
			break;

		/* If no item is currently been dragged, and the last drag
		has not just finished (i.e. item was dragged, but was cancelled
		with escape, but mouse button is still down), and when the right
		mouse button was clicked, it was over an item, start dragging. */
		case WM_MOUSEMOVE:
			{
				m_blockNextListViewSelection = FALSE;
				if(!m_bDragging && !m_bDragCancelled && m_bDragAllowed)
				{
					if((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON)
						&& !(wParam & MK_MBUTTON))
					{
						NMLISTVIEW nmlv;
						POINT pt;
						DWORD dwPos;
						HRESULT hr;

						dwPos = GetMessagePos();
						pt.x = GET_X_LPARAM(dwPos);
						pt.y = GET_Y_LPARAM(dwPos);
						MapWindowPoints(HWND_DESKTOP,m_hActiveListView,&pt,1);

						LV_HITTESTINFO lvhti;

						lvhti.pt = pt;

						/* Test to see if the mouse click was
						on an item or not. */
						ListView_HitTest(m_hActiveListView,&lvhti);

						if(!(lvhti.flags & LVHT_NOWHERE) && ListView_GetSelectedCount(m_hActiveListView) > 0)
						{
							nmlv.iItem = 0;
							nmlv.ptAction = pt;

							hr = OnListViewBeginDrag((LPARAM)&nmlv,DragType::RightClick);

							if(hr == DRAGDROP_S_CANCEL)
							{
								m_bDragCancelled = TRUE;
							}
						}
					}
				}
			}
			break;

		case WM_MOUSEWHEEL:
			if(OnMouseWheel(MOUSEWHEEL_SOURCE_LISTVIEW,wParam,lParam))
			{
				return 0;
			}
			break;

		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->code)
			{
			case HDN_BEGINDRAG:
				return FALSE;
				break;

			case HDN_ENDDRAG:
				{
					/* When the drag ends, the dragged item
					is shifted into position, and all later
					items are shifted down one. Therefore,
					take the item out of its position in the
					list, and move it into its new position. */
					NMHEADER *pnmHeader = NULL;
					Column_t Column;
					int i = 0;

					pnmHeader = (NMHEADER *)lParam;

					auto currentColumns = m_pActiveShellBrowser->ExportCurrentColumns();

					i = 0;
					auto itr = currentColumns.begin();
					while(i < (pnmHeader->iItem + 1) && itr != currentColumns.end())
					{
						if(itr->bChecked)
						{
							i++;
						}

						itr++;
					}

					if(itr != currentColumns.begin())
						itr--;

					Column = *itr;
					currentColumns.erase(itr);

					i = 0;
					itr = currentColumns.begin();
					while(i < (pnmHeader->pitem->iOrder + 1) && itr != currentColumns.end())
					{
						if(itr->bChecked)
						{
							i++;
						}

						itr++;
					}

					if(itr != currentColumns.begin())
						itr--;

					currentColumns.insert(itr,Column);

					m_pActiveShellBrowser->ImportColumns(currentColumns);

					Tab &tab = m_tabContainer->GetSelectedTab();
					tab.GetShellBrowser()->GetNavigationController()->Refresh();

					return TRUE;
				}
				break;
			}
			break;
	}

	return DefSubclassProc(ListView,msg,wParam,lParam);
}

LRESULT Explorerplusplus::OnListViewKeyDown(LPARAM lParam)
{
	LV_KEYDOWN	*lv_key = NULL;

	lv_key = (LV_KEYDOWN *)lParam;

	switch(lv_key->wVKey)
	{
		case VK_RETURN:
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				/* Key press: Ctrl+Enter
				Action: Open item in background tab. */
				OpenAllSelectedItems(TRUE);
			}
			else
			{
				OpenAllSelectedItems(FALSE);
			}
			break;

		case VK_DELETE:
			if(IsKeyDown(VK_SHIFT))
			{
				OnListViewFileDelete(true);
			}
			else
			{
				OnListViewFileDelete(false);
			}
			break;

		case 'C':
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				OnListViewCopy(TRUE);
			}
			break;

		case 'V':
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				OnListViewPaste();
			}
			break;

		case 'X':
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				OnListViewCopy(FALSE);
			}
			break;
	}

	return 0;
}

BOOL Explorerplusplus::OnListViewItemChanging(const NMLISTVIEW *changeData)
{
	if (changeData->uChanged != LVIF_STATE)
	{
		return FALSE;
	}

	int tabId = DetermineListViewObjectIndex(changeData->hdr.hwndFrom);

	if (tabId == -1)
	{
		return FALSE;
	}

	Tab &tab = m_tabContainer->GetTab(tabId);
	ViewMode viewMode = tab.GetShellBrowser()->GetViewMode();

	bool previouslySelected = WI_IsFlagSet(changeData->uOldState, LVIS_SELECTED);
	bool currentlySelected = WI_IsFlagSet(changeData->uNewState, LVIS_SELECTED);

	if (viewMode == +ViewMode::List && !previouslySelected && currentlySelected)
	{
		if (m_blockNextListViewSelection)
		{
			m_blockNextListViewSelection = FALSE;
			return TRUE;
		}
	}

	return FALSE;
}

int Explorerplusplus::DetermineListViewObjectIndex(HWND hListView)
{
	for (auto &item : m_tabContainer->GetAllTabs())
	{
		if (item.second->GetShellBrowser()->GetListView() == hListView)
		{
			return item.first;
		}
	}

	return -1;
}

BOOL Explorerplusplus::OnListViewBeginLabelEdit(LPARAM lParam)
{
	if(!CanRename())
	{
		return TRUE;
	}

	/* Subclass the edit window. The only reason this is
	done is so that when the listview item is put into
	edit mode, any extension the file has will not be
	selected along with the rest of the text. Although
	selection works directly from here in Windows Vista,
	it does not work in Windows XP. */
	HWND hEdit = ListView_GetEditControl(m_hActiveListView);

	if(hEdit == NULL)
	{
		return TRUE;
	}

	CListViewEdit::CreateNew(hEdit,reinterpret_cast<NMLVDISPINFO *>(lParam)->item.iItem,this);

	m_bListViewRenaming = TRUE;

	return FALSE;
}

BOOL Explorerplusplus::OnListViewEndLabelEdit(LPARAM lParam)
{
	NMLVDISPINFO	*pdi = NULL;
	LVITEM			*pItem = NULL;
	TCHAR			NewFileName[MAX_PATH + 1];
	TCHAR			OldFileName[MAX_PATH + 1];
	TCHAR			OldName[MAX_PATH];
	TCHAR			szTemp[128];
	TCHAR			szError[256];
	TCHAR			szTitle[256];
	DWORD			dwAttributes;
	int				ret;

	pdi = (NMLVDISPINFO *) lParam;
	pItem = &pdi->item;

	m_bListViewRenaming = FALSE;

	/* Did the user cancel the editing? */
	if(pItem->pszText == NULL)
		return FALSE;

	/* Is the new filename empty? */
	if(lstrcmp(pItem->pszText,EMPTY_STRING) == 0)
		return FALSE;

	/*
	Deny file names ending with a dot, as they are just
	synonyms for the same file without any dot(s).
	For example:
	C:\Hello.txt
	C:\Hello.txt....
	refer to exactly the same file.
	
	Taken from the web site referenced below:
	"Do not end a file or directory name with a trailing
	space or a period. Although the underlying file system
	may support such names, the operating system does not.
	However, it is acceptable to start a name with a period."	
	*/
	if(pItem->pszText[lstrlen(pItem->pszText) - 1] == '.')
		return FALSE;

	/*
	The following characters are NOT allowed
	within a file name:
	\/:*?"<>|

	See: http://msdn.microsoft.com/en-us/library/aa365247.aspx
	*/
	if(StrChr(pItem->pszText,'\\') != NULL ||
		StrChr(pItem->pszText,'/') != NULL ||
		StrChr(pItem->pszText,':') != NULL ||
		StrChr(pItem->pszText,'*') != NULL ||
		StrChr(pItem->pszText,'?') != NULL ||
		StrChr(pItem->pszText,'"') != NULL ||
		StrChr(pItem->pszText,'<') != NULL ||
		StrChr(pItem->pszText,'>') != NULL ||
		StrChr(pItem->pszText,'|') != NULL)
	{
		LoadString(m_hLanguageModule,IDS_ERR_FILENAMEINVALID,
			szError,SIZEOF_ARRAY(szError));
		LoadString(m_hLanguageModule,IDS_ERR_FILENAMEINVALID_MSGTITLE,
			szTitle,SIZEOF_ARRAY(szTitle));

		MessageBox(m_hContainer,szError,szTitle,MB_ICONERROR);

		return 0;
	}

	std::wstring currentDirectory = m_pActiveShellBrowser->GetDirectory();
	StringCchCopy(NewFileName, SIZEOF_ARRAY(NewFileName), currentDirectory.c_str());
	StringCchCopy(OldFileName, SIZEOF_ARRAY(OldFileName), currentDirectory.c_str());

	m_pActiveShellBrowser->GetItemDisplayName(pItem->iItem,SIZEOF_ARRAY(OldName),OldName);
	PathAppend(OldFileName,OldName);

	BOOL bRes = PathAppend(NewFileName,pItem->pszText);

	if(!bRes)
	{
		return 0;
	}

	dwAttributes = m_pActiveShellBrowser->GetItemFileFindData(pItem->iItem).dwFileAttributes;

	if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
	{
		BOOL bExtensionHidden = FALSE;

		bExtensionHidden = (!m_config->globalFolderSettings.showExtensions) ||
			(m_config->globalFolderSettings.hideLinkExtension && lstrcmpi(PathFindExtension(OldName),_T(".lnk")) == 0);

		/* If file extensions are turned off, the new filename
		will be incorrect (i.e. it will be missing the extension).
		Therefore, append the extension manually if it is turned
		off. */
		if(bExtensionHidden)
		{
			TCHAR	*szExt = NULL;

			szExt = PathFindExtension(OldName);

			if(*szExt == '.')
				StringCchCat(NewFileName,SIZEOF_ARRAY(NewFileName),szExt);
		}
	}

	if (lstrcmp(OldFileName, NewFileName) == 0)
		return FALSE;

	CFileActionHandler::RenamedItem_t RenamedItem;
	RenamedItem.strOldFilename = OldFileName;
	RenamedItem.strNewFilename = NewFileName;

	TrimStringRight(RenamedItem.strNewFilename,_T(" "));

	std::list<CFileActionHandler::RenamedItem_t> RenamedItemList;
	RenamedItemList.push_back(RenamedItem);
	ret = m_FileActionHandler.RenameFiles(RenamedItemList);

	/* If the file was not renamed, show an error message. */
	if(!ret)
	{
		LoadString(m_hLanguageModule,IDS_FILERENAMEERROR,szTemp,
		SIZEOF_ARRAY(szTemp));

		MessageBox(m_hContainer,szTemp,NExplorerplusplus::APP_NAME,
			MB_ICONWARNING|MB_OK);
	}

	return ret;
}

void Explorerplusplus::OnListViewRClick(POINT *pCursorPos)
{
	/* It may be possible for the active tab/folder
	to change while the menu is been shown (e.g. if
	not handled correctly, the back/forward buttons
	on a mouse will change the directory without
	destroying the popup menu).
	If this happens, the shell browser used will not
	stay constant throughout this function, and will
	cause various problems (e.g. changing from a
	computer where new items can be created to one
	where they cannot while the popup menu is still
	active will cause the 'new' entry to stay on the
	menu incorrectly).
	Due to the possibility of unforseen problems,
	this function should NOT access the current
	shell browser once the menu has been destroyed. */

	SetForegroundWindow(m_hContainer);

	if(IsKeyDown(VK_SHIFT) &&
		!IsKeyDown(VK_CONTROL) &&
		!IsKeyDown(VK_MENU))
	{
		LVHITTESTINFO lvhti;

		lvhti.pt = *pCursorPos;
		ScreenToClient(m_hActiveListView,&lvhti.pt);
		ListView_HitTest(m_hActiveListView,&lvhti);

		if(!(lvhti.flags & LVHT_NOWHERE) && lvhti.iItem != -1)
		{
			if(ListView_GetItemState(m_hActiveListView,lvhti.iItem,LVIS_SELECTED) !=
				LVIS_SELECTED)
			{
				NListView::ListView_SelectAllItems(m_hActiveListView,FALSE);
				NListView::ListView_SelectItem(m_hActiveListView,lvhti.iItem,TRUE);
			}
		}
	}

	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		OnListViewBackgroundRClick(pCursorPos);
	}
	else
	{
		OnListViewItemRClick(pCursorPos);
	}
}

void Explorerplusplus::OnListViewBackgroundRClick(POINT *pCursorPos)
{
	HMENU hMenu = InitializeRightClickMenu();
	auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

	unique_pidl_absolute pidlParent(ILCloneFull(pidlDirectory.get()));
	ILRemoveLastID(pidlParent.get());

	PCUITEMID_CHILD pidlChildFolder = ILFindLastID(pidlDirectory.get());

	wil::com_ptr<IShellFolder> pShellFolder;
	HRESULT hr = BindToIdl(pidlParent.get(), IID_PPV_ARGS(&pShellFolder));

	if(SUCCEEDED(hr))
	{
		wil::com_ptr<IDataObject> pDataObject;
		hr = GetUIObjectOf(pShellFolder.get(), NULL, 1, &pidlChildFolder, IID_PPV_ARGS(&pDataObject));

		if(SUCCEEDED(hr))
		{
			CServiceProvider ServiceProvider(this);
			CContextMenuManager cmm(CContextMenuManager::CONTEXT_MENU_TYPE_BACKGROUND, pidlDirectory.get(),
				pDataObject.get(), &ServiceProvider, BLACKLISTED_BACKGROUND_MENU_CLSID_ENTRIES);

			cmm.ShowMenu(m_hContainer,hMenu,IDM_FILE_COPYFOLDERPATH,MIN_SHELL_MENU_ID,
				MAX_SHELL_MENU_ID,*pCursorPos,*m_pStatusBar);
		}
	}

	DestroyMenu(hMenu);
}

HMENU Explorerplusplus::InitializeRightClickMenu(void)
{
	HMENU hMenu = GetSubMenu(LoadMenu(m_hLanguageModule,
		MAKEINTRESOURCE(IDR_MAINMENU_RCLICK)),0);

	MENUITEMINFO mii;

	for(auto ViewMode : VIEW_MODES)
	{
		TCHAR szTemp[64];
		LoadString(m_hLanguageModule,GetViewModeMenuStringId(ViewMode),
			szTemp,SIZEOF_ARRAY(szTemp));

		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_ID|MIIM_STRING;
		mii.wID			= GetViewModeMenuId(ViewMode);
		mii.dwTypeData	= szTemp;
		InsertMenuItem(hMenu,IDM_RCLICK_VIEW_PLACEHOLDER,FALSE,&mii);
	}

	DeleteMenu(hMenu,IDM_RCLICK_VIEW_PLACEHOLDER,MF_BYCOMMAND);

	mii.cbSize		= sizeof(mii);
	mii.fMask		= MIIM_SUBMENU;
	mii.hSubMenu	= m_hSortSubMenu;
	SetMenuItemInfo(hMenu,IDM_POPUP_SORTBY,FALSE,&mii);

	mii.cbSize		= sizeof(mii);
	mii.fMask		= MIIM_SUBMENU;
	mii.hSubMenu	= m_hGroupBySubMenu;
	SetMenuItemInfo(hMenu,IDM_POPUP_GROUPBY,FALSE,&mii);

	UINT uViewMode = m_pActiveShellBrowser->GetViewMode();

	if(uViewMode == ViewMode::List)
	{
		lEnableMenuItem(hMenu,IDM_POPUP_GROUPBY,FALSE);
	}
	else
	{
		lEnableMenuItem(hMenu,IDM_POPUP_GROUPBY,TRUE);
	}

	return hMenu;
}

void Explorerplusplus::OnListViewItemRClick(POINT *pCursorPos)
{
	int nSelected = ListView_GetSelectedCount(m_hActiveListView);

	if(nSelected > 0)
	{
		std::vector<unique_pidl_child> pidlPtrs;
		std::vector<PCITEMID_CHILD> pidlItems;
		int iItem = -1;

		while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
		{
			auto pidlPtr = m_pActiveShellBrowser->GetItemChildIdl(iItem);

			pidlItems.push_back(pidlPtr.get());
			pidlPtrs.push_back(std::move(pidlPtr));
		}

		auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

		CFileContextMenuManager fcmm(m_hActiveListView, pidlDirectory.get(), pidlItems);

		FileContextMenuInfo_t fcmi;
		fcmi.uFrom = FROM_LISTVIEW;

		CStatusBar StatusBar(m_hStatusBar);

		fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,pCursorPos,&StatusBar,
			reinterpret_cast<DWORD_PTR>(&fcmi),TRUE,IsKeyDown(VK_SHIFT));
	}
}

HRESULT Explorerplusplus::OnListViewBeginDrag(LPARAM lParam,DragType dragType)
{
	IDropSource			*pDropSource = NULL;
	IDragSourceHelper	*pDragSourceHelper = NULL;
	NMLISTVIEW			*pnmlv = NULL;
	POINT				pt = {0,0};
	HRESULT				hr;
	int					iDragStartObjectIndex;

	pnmlv = reinterpret_cast<NMLISTVIEW *>(lParam);

	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return E_FAIL;
	}

	std::vector<unique_pidl_child> pidls;
	std::vector<PCITEMID_CHILD> rawPidls;
	std::list<std::wstring> FilenameList;

	int item = -1;

	/* Store the pidl of the current folder, as well as the relative
	pidl's of the dragged items. */
	auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();

	while((item = ListView_GetNextItem(m_hActiveListView,item,LVNI_SELECTED)) != -1)
	{
		auto pidl = m_pActiveShellBrowser->GetItemChildIdl(item);

		rawPidls.push_back(pidl.get());
		pidls.push_back(std::move(pidl));

		TCHAR szFullFilename[MAX_PATH];

		m_pActiveShellBrowser->GetItemFullName(item,szFullFilename,SIZEOF_ARRAY(szFullFilename));

		std::wstring stringFilename(szFullFilename);

		FilenameList.push_back(stringFilename);
	}

	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_ALL,
		IID_PPV_ARGS(&pDragSourceHelper));

	if(SUCCEEDED(hr))
	{
		hr = CreateDropSource(&pDropSource, dragType);

		if(SUCCEEDED(hr))
		{
			FORMATETC ftc[2];
			STGMEDIUM stg[2];

			/* We'll export two formats:
			CF_HDROP
			CFSTR_SHELLIDLIST */
			BuildHDropList(&ftc[0],&stg[0],FilenameList);
			BuildShellIDList(&ftc[1],&stg[1],pidlDirectory.get(),rawPidls);

			IDataObject *pDataObject = NULL;
			IDataObjectAsyncCapability *pAsyncCapability = NULL;
			
			hr = CreateDataObject(ftc,stg,&pDataObject,2);
			pDataObject->QueryInterface(IID_PPV_ARGS(&pAsyncCapability));

			assert(pAsyncCapability != NULL);

			/* Docs mention setting the argument to VARIANT_TRUE/VARIANT_FALSE.
			But the argument is a BOOL, so we'll go with regular TRUE/FALSE. */
			pAsyncCapability->SetAsyncMode(TRUE);

			hr = pDragSourceHelper->InitializeFromWindow(m_hActiveListView,&pt,pDataObject);

			m_pActiveShellBrowser->DragStarted(pnmlv->iItem,&pnmlv->ptAction);
			m_bDragging = TRUE;

			/* Need to remember which tab started the drag (as
			it may be different from the tab in which the drag
			finishes). */
			iDragStartObjectIndex = m_tabContainer->GetSelectedTab().GetId();

			DWORD dwEffect;

			hr = DoDragDrop(pDataObject,pDropSource,DROPEFFECT_COPY|DROPEFFECT_MOVE|
				DROPEFFECT_LINK,&dwEffect);

			m_bDragging = FALSE;

			/* The object that starts any drag may NOT be the
			object that stops it (i.e. when a file is dragged
			between tabs). Therefore, need to tell the object
			that STARTED dragging that dragging has stopped. */
			m_tabContainer->GetTab(iDragStartObjectIndex).GetShellBrowser()->DragStopped();

			BOOL bInAsyncOp;

			hr = pAsyncCapability->InOperation(&bInAsyncOp);

			pAsyncCapability->Release();
			pDataObject->Release();
			pDropSource->Release();
		}

		pDragSourceHelper->Release();
	}

	return hr;
}

void Explorerplusplus::OnListViewFileDelete(bool permanent)
{
	int nSelected = ListView_GetSelectedCount(m_hActiveListView);

	if(nSelected == 0)
	{
		return;
	}

	// PIDLPointer is analogous to a unique_ptr. This vector exists only
	// so that the underlying PIDLs will be freed on scope exit (i.e.
	// when this function returns).
	std::vector<unique_pidl_absolute> pidlPtrs;

	std::vector<PCIDLIST_ABSOLUTE> pidls;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		auto pidlPtr = m_pActiveShellBrowser->GetItemCompleteIdl(iItem);

		if (!pidlPtr)
		{
			continue;
		}

		pidls.push_back(pidlPtr.get());
		pidlPtrs.push_back(std::move(pidlPtr));
	}

	if (pidls.empty())
	{
		return;
	}

	m_FileActionHandler.DeleteFiles(m_hContainer,pidls,permanent,false);
}

void Explorerplusplus::OnListViewDoubleClick(NMHDR *nmhdr)
{
	if(nmhdr->hwndFrom == m_hActiveListView)
	{
		LV_HITTESTINFO	ht;
		DWORD			dwPos;
		POINT			MousePos;

		dwPos = GetMessagePos();
		MousePos.x = GET_X_LPARAM(dwPos);
		MousePos.y = GET_Y_LPARAM(dwPos);
		ScreenToClient(m_hActiveListView,&MousePos);

		ht.pt = MousePos;
		ListView_HitTest(m_hActiveListView,&ht);

		if(ht.flags != LVHT_NOWHERE && ht.iItem != -1)
		{
			if(IsKeyDown(VK_MENU))
			{
				auto pidlDirectory = m_pActiveShellBrowser->GetDirectoryIdl();
				auto pidl = m_pActiveShellBrowser->GetItemChildIdl(ht.iItem);
				std::vector<PCITEMID_CHILD> items = { pidl.get() };

				ShowMultipleFileProperties(pidlDirectory.get(), items.data(), m_hContainer, 1);
			}
			else if(IsKeyDown(VK_CONTROL))
			{
				/* Open the item in a new tab. */
				OpenListViewItem(ht.iItem,TRUE,FALSE);
			}
			else if(IsKeyDown(VK_SHIFT))
			{
				/* Open the item in a new window. */
				OpenListViewItem(ht.iItem,FALSE,TRUE);
			}
			else
			{
				OpenListViewItem(ht.iItem,FALSE,FALSE);
			}
		}
	}
}

void Explorerplusplus::OnListViewFileRename()
{
	int nSelected = ListView_GetSelectedCount(m_hActiveListView);

	/* If there is only item selected, start editing
	it in-place. If multiple items are selected,
	show the mass rename dialog. */
	if(nSelected == 1)
	{
		OnListViewFileRenameSingle();
	}
	else if(nSelected > 1)
	{
		OnListViewFileRenameMultiple();
	}
}

void Explorerplusplus::OnListViewFileRenameSingle()
{
	int iSelected = ListView_GetNextItem(m_hActiveListView,
		-1, LVNI_SELECTED | LVNI_FOCUSED);

	if (iSelected == -1)
	{
		return;
	}

	BOOL canRename = TestListViewItemAttributes(iSelected, SFGAO_CANRENAME);

	if (!canRename)
	{
		return;
	}

	ListView_EditLabel(m_hActiveListView, iSelected);
}

void Explorerplusplus::OnListViewFileRenameMultiple()
{
	std::list<std::wstring>	FullFilenameList;
	TCHAR szFullFilename[MAX_PATH];
	int iIndex = -1;

	while ((iIndex = ListView_GetNextItem(m_hActiveListView,
		iIndex, LVNI_SELECTED)) != -1)
	{
		BOOL canRename = TestListViewItemAttributes(iIndex, SFGAO_CANRENAME);

		if (!canRename)
		{
			continue;
		}

		m_pActiveShellBrowser->GetItemFullName(iIndex, szFullFilename, SIZEOF_ARRAY(szFullFilename));
		FullFilenameList.push_back(szFullFilename);
	}

	if (FullFilenameList.empty())
	{
		return;
	}

	CMassRenameDialog CMassRenameDialog(m_hLanguageModule, m_hContainer, this,
		FullFilenameList, &m_FileActionHandler);
	CMassRenameDialog.ShowModalDialog();
}

void Explorerplusplus::OnListViewCopyItemPath(void) const
{
	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return;
	}

	std::wstring strItemPaths;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->GetItemFullName(iItem,szFullFilename,SIZEOF_ARRAY(szFullFilename));

		strItemPaths += szFullFilename + std::wstring(_T("\r\n"));
	}

	strItemPaths = strItemPaths.substr(0,strItemPaths.size() - 2);

	BulkClipboardWriter clipboardWriter;
	clipboardWriter.WriteText(strItemPaths);
}

void Explorerplusplus::OnListViewCopyUniversalPaths(void) const
{
	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return;
	}

	std::wstring strUniversalPaths;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->GetItemFullName(iItem,szFullFilename,SIZEOF_ARRAY(szFullFilename));

		TCHAR szBuffer[1024];

		DWORD dwBufferSize = SIZEOF_ARRAY(szBuffer);
		UNIVERSAL_NAME_INFO *puni = reinterpret_cast<UNIVERSAL_NAME_INFO *>(&szBuffer);
		DWORD dwRet = WNetGetUniversalName(szFullFilename,UNIVERSAL_NAME_INFO_LEVEL,
			reinterpret_cast<LPVOID>(puni),&dwBufferSize);

		if(dwRet == NO_ERROR)
		{
			strUniversalPaths += puni->lpUniversalName + std::wstring(_T("\r\n"));
		}
		else
		{
			strUniversalPaths += szFullFilename + std::wstring(_T("\r\n"));
		}
	}

	strUniversalPaths = strUniversalPaths.substr(0,strUniversalPaths.size() - 2);

	BulkClipboardWriter clipboardWriter;
	clipboardWriter.WriteText(strUniversalPaths);
}

HRESULT Explorerplusplus::OnListViewCopy(BOOL bCopy)
{
	IDataObject		*pClipboardDataObject = NULL;
	int				iItem = -1;
	HRESULT			hr;

	if(!CanCopy())
		return E_FAIL;

	SetCursor(LoadCursor(NULL,IDC_WAIT));

	std::list<std::wstring> FileNameList;

	BuildListViewFileSelectionList(m_hActiveListView,&FileNameList);

	if(bCopy)
	{
		hr = CopyFiles(FileNameList,&pClipboardDataObject);

		if(SUCCEEDED(hr))
		{
			m_pClipboardDataObject = pClipboardDataObject;
		}
	}
	else
	{
		hr = CutFiles(FileNameList,&pClipboardDataObject);

		if(SUCCEEDED(hr))
		{
			m_pClipboardDataObject = pClipboardDataObject;
			m_iCutTabInternal = m_tabContainer->GetSelectedTab().GetId();

			TCHAR szFilename[MAX_PATH];

			/* 'Ghost' each of the cut items. */
			while((iItem = ListView_GetNextItem(m_hActiveListView,
				iItem,LVNI_SELECTED)) != -1)
			{
				m_pActiveShellBrowser->GetItemDisplayName(iItem,SIZEOF_ARRAY(szFilename),
					szFilename);
				m_CutFileNameList.push_back(szFilename);

				m_pActiveShellBrowser->GhostItem(iItem);
			}
		}
	}

	SetCursor(LoadCursor(NULL,IDC_ARROW));

	return hr;
}

void Explorerplusplus::OnListViewSetFileAttributes() const
{
	const Tab &selectedTab = m_tabContainer->GetSelectedTab();
	selectedTab.GetShellBrowser()->SetFileAttributesForSelection();
}

void Explorerplusplus::OnListViewPaste(void)
{
	IDataObject *pClipboardObject = NULL;
	HRESULT hr;

	hr = OleGetClipboard(&pClipboardObject);

	if(hr == S_OK)
	{
		TCHAR szDestination[MAX_PATH + 1];

		/* DO NOT use the internal current directory string.
		Files are copied asynchronously, so a change of directory
		will cause the destination directory to change in the
		middle of the copy operation. */
		StringCchCopy(szDestination,SIZEOF_ARRAY(szDestination),
			m_CurrentDirectory.c_str());

		/* Also, the string must be double NULL terminated. */
		szDestination[lstrlen(szDestination) + 1] = '\0';

		CDropHandler *pDropHandler = CDropHandler::CreateNew();
		CDropFilesCallback *DropFilesCallback = new CDropFilesCallback(this);
		pDropHandler->CopyClipboardData(pClipboardObject,m_hContainer,szDestination,
			DropFilesCallback,!m_config->overwriteExistingFilesConfirmation);
		pDropHandler->Release();

		pClipboardObject->Release();
	}
}

void Explorerplusplus::BuildListViewFileSelectionList(HWND hListView,
	std::list<std::wstring> *pFileSelectionList)
{
	if(pFileSelectionList == NULL)
	{
		return;
	}

	std::list<std::wstring> FileSelectionList;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(hListView,
		iItem,LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFileName[MAX_PATH];

		m_pActiveShellBrowser->GetItemFullName(iItem,
			szFullFileName,SIZEOF_ARRAY(szFullFileName));

		std::wstring stringFileName(szFullFileName);
		FileSelectionList.push_back(stringFileName);
	}

	pFileSelectionList->assign(FileSelectionList.begin(),
		FileSelectionList.end());
}

int Explorerplusplus::HighlightSimilarFiles(HWND ListView) const
{
	TCHAR	FullFileName[MAX_PATH];
	TCHAR	TestFile[MAX_PATH];
	HRESULT	hr;
	BOOL	bSimilarTypes;
	int		iSelected;
	int		nItems;
	int		nSimilar = 0;
	int		i = 0;

	iSelected = ListView_GetNextItem(ListView,
	-1,LVNI_SELECTED);

	if(iSelected == -1)
		return -1;

	hr = m_pActiveShellBrowser->GetItemFullName(iSelected,TestFile,SIZEOF_ARRAY(TestFile));

	if(SUCCEEDED(hr))
	{
		nItems = ListView_GetItemCount(ListView);

		for(i = 0;i < nItems;i++)
		{
			m_pActiveShellBrowser->GetItemFullName(i,FullFileName,SIZEOF_ARRAY(FullFileName));

			bSimilarTypes = CompareFileTypes(FullFileName,TestFile);

			if(bSimilarTypes)
			{
				NListView::ListView_SelectItem(ListView,i,TRUE);
				nSimilar++;
			}
			else
			{
				NListView::ListView_SelectItem(ListView,i,FALSE);
			}
		}
	}

	return nSimilar;
}

void Explorerplusplus::OpenAllSelectedItems(BOOL bOpenInNewTab)
{
	BOOL	bSeenDirectory = FALSE;
	DWORD	dwAttributes;
	int		iItem = -1;
	int		iFolderItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVIS_SELECTED)) != -1)
	{
		dwAttributes = m_pActiveShellBrowser->GetItemFileFindData(iItem).dwFileAttributes;

		if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			bSeenDirectory = TRUE;
			iFolderItem = iItem;
		}
		else
		{
			OpenListViewItem(iItem,FALSE,FALSE);
		}
	}

	if(bSeenDirectory)
		OpenListViewItem(iFolderItem,bOpenInNewTab,FALSE);
}

void Explorerplusplus::OpenListViewItem(int iItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow)
{
	auto pidl = m_pActiveShellBrowser->GetDirectoryIdl();
	auto ridl = m_pActiveShellBrowser->GetItemChildIdl(iItem);

	if(ridl != NULL)
	{
		unique_pidl_absolute pidlComplete(ILCombine(pidl.get(), ridl.get()));
		OpenItem(pidlComplete.get(), bOpenInNewTab, bOpenInNewWindow);
	}
}