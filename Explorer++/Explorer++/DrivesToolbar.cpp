/******************************************************************
 *
 * Project: Explorer++
 * File: DrivesToolbarHandler.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles all messages associated with the drives toolbar.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++_internal.h"
#include "DrivesToolbar.h"
#include "MainResource.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/Macros.h"


CDrivesToolbar::CDrivesToolbar(HWND hToolbar,UINT uIDStart,UINT uIDEnd,HINSTANCE hInstance,IExplorerplusplus *pexpp) :
m_hToolbar(hToolbar),
m_uIDStart(uIDStart),
m_uIDEnd(uIDEnd),
m_hInstance(hInstance),
m_pexpp(pexpp),
m_IDCounter(0)
{
	InitializeToolbar();

	/* TODO: Register for hardware change notifications. */
}

CDrivesToolbar::~CDrivesToolbar()
{
	RemoveWindowSubclass(m_hToolbar,DrivesToolbarProcStub,SUBCLASS_ID);
	RemoveWindowSubclass(GetParent(m_hToolbar),DrivesToolbarParentProcStub,PARENT_SUBCLASS_ID);
}

void CDrivesToolbar::InitializeToolbar()
{
	SendMessage(m_hToolbar,TB_SETBITMAPSIZE,0,MAKELONG(16,16));
	SendMessage(m_hToolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);
	SendMessage(m_hToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(himlSmall));

	SetWindowSubclass(m_hToolbar,DrivesToolbarProcStub,SUBCLASS_ID,reinterpret_cast<DWORD_PTR>(this));

	SetWindowSubclass(GetParent(m_hToolbar),DrivesToolbarParentProcStub,PARENT_SUBCLASS_ID,
		reinterpret_cast<DWORD_PTR>(this));

	InsertDrives();
}

LRESULT CALLBACK DrivesToolbarProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CDrivesToolbar *pdt = reinterpret_cast<CDrivesToolbar *>(dwRefData);

	return pdt->DrivesToolbarProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CDrivesToolbar::DrivesToolbarProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_MBUTTONUP:
		{
			POINT ptCursor;
			ptCursor.x = GET_X_LPARAM(lParam);
			ptCursor.y = GET_Y_LPARAM(lParam);

			int iIndex = static_cast<int>(SendMessage(m_hToolbar,TB_HITTEST,0,reinterpret_cast<LPARAM>(&ptCursor)));

			if(iIndex >= 0)
			{
				TBBUTTON tbButton;
				SendMessage(m_hToolbar,TB_GETBUTTON,iIndex,reinterpret_cast<LPARAM>(&tbButton));

				auto itr = m_mapID.find(static_cast<IDCounter>(static_cast<UINT>(tbButton.dwData)));
				assert(itr != m_mapID.end());

				m_pexpp->BrowseFolder(itr->second.c_str(),SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK DrivesToolbarParentProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	CDrivesToolbar *pdt = reinterpret_cast<CDrivesToolbar *>(dwRefData);

	return pdt->DrivesToolbarParentProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK CDrivesToolbar::DrivesToolbarParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		if(LOWORD(wParam) >= m_uIDStart &&
			LOWORD(wParam) <= m_uIDEnd)
		{
			int iIndex = static_cast<int>(SendMessage(m_hToolbar,TB_COMMANDTOINDEX,LOWORD(wParam),0));

			if(iIndex != -1)
			{
				std::wstring Path = GetDrivePath(iIndex);
				m_pexpp->BrowseFolder(Path.c_str(),SBSP_ABSOLUTE,FALSE,FALSE,FALSE);
			}

			return 0;
		}
		break;

	case WM_NOTIFY:
		if(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hToolbar)
		{
			switch(reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
				{
					NMMOUSE *pnmm = reinterpret_cast<NMMOUSE *>(lParam);

					if(pnmm->dwItemSpec != -1)
					{
						int iIndex = static_cast<int>(SendMessage(m_hToolbar,TB_COMMANDTOINDEX,pnmm->dwItemSpec,0));

						if(iIndex != -1)
						{
							std::wstring Path = GetDrivePath(iIndex);

							LPITEMIDLIST pidlItem = NULL;
							HRESULT hr = GetIdlFromParsingName(Path.c_str(),&pidlItem);

							if(SUCCEEDED(hr))
							{
								ClientToScreen(m_hToolbar,&pnmm->pt);

								std::list<LPITEMIDLIST> pidlItemList;
								CFileContextMenuManager fcmm(m_hToolbar,pidlItem,pidlItemList);

								fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,&pnmm->pt,m_pexpp->GetStatusBar(),
									NULL,FALSE,GetKeyState(VK_SHIFT) & 0x80);

								CoTaskMemFree(pidlItem);
							}

							return TRUE;
						}
					}
				}
				break;

			case TBN_GETINFOTIP:
				{
					NMTBGETINFOTIP *pnmtbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

					int iIndex = static_cast<int>(SendMessage(m_hToolbar,TB_COMMANDTOINDEX,pnmtbgit->iItem,0));

					if(iIndex != -1)
					{
						std::wstring Path = GetDrivePath(iIndex);
						GetItemInfoTip(Path.c_str(),pnmtbgit->pszText,pnmtbgit->cchTextMax);
					}

					return 0;
				}
				break;
			}
		}
		break;
	}

	return DefSubclassProc(hwnd,uMsg,wParam,lParam);
}

void CDrivesToolbar::InsertDrives()
{
	DWORD dwSize = GetLogicalDriveStrings(0,NULL);

	TCHAR *pDriveStrings = new TCHAR[dwSize];
	dwSize = GetLogicalDriveStrings(dwSize,pDriveStrings);

	if(dwSize != 0)
	{
		TCHAR *pDrive = pDriveStrings;

		while(*pDrive != '\0')
		{
			InsertDrive(pDrive);

			pDrive += (lstrlen(pDrive) + 1);
		}
	}

	delete[] pDriveStrings;
}

void CDrivesToolbar::InsertDrive(const std::wstring &DrivePath)
{
	TCHAR szDisplayName[32];
	StringCchCopy(szDisplayName,SIZEOF_ARRAY(szDisplayName),DrivePath.c_str());

	/* Drives will be shown without a closing backslash. */
	if(szDisplayName[lstrlen(szDisplayName) - 1] == '\\')
	{
		szDisplayName[lstrlen(szDisplayName) - 1] = '\0';
	}

	SHFILEINFO shfi;
	SHGetFileInfo(DrivePath.c_str(),0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX|SHGFI_USEFILEATTRIBUTES);

	int Position = GetSortedPosition(DrivePath);

	TBBUTTON tbButton;
	tbButton.iBitmap	= shfi.iIcon;
	tbButton.idCommand	= m_uIDStart + m_IDCounter;
	tbButton.fsState	= TBSTATE_ENABLED;
	tbButton.fsStyle	= BTNS_BUTTON|BTNS_AUTOSIZE|BTNS_SHOWTEXT|BTNS_NOPREFIX;
	tbButton.dwData		= m_IDCounter;
	tbButton.iString	= reinterpret_cast<INT_PTR>(szDisplayName);
	SendMessage(m_hToolbar,TB_INSERTBUTTON,Position,reinterpret_cast<LPARAM>(&tbButton));

	m_mapID.insert(std::make_pair(m_IDCounter,DrivePath));
	++m_IDCounter;
}

void CDrivesToolbar::RemoveDrive(const std::wstring &DrivePath)
{
	DriveInformation_t di = GetDrivePosition(DrivePath);

	SendMessage(m_hToolbar,TB_DELETEBUTTON,di.Position,0);
	m_mapID.erase(di.ID);
}

/* Updates an items icon. This may be necessary,
for example, if a cd/dvd is inserted/removed. */
void CDrivesToolbar::UpdateDriveIcon(const std::wstring &DrivePath)
{
	DriveInformation_t di = GetDrivePosition(DrivePath);

	SHFILEINFO shfi;
	SHGetFileInfo(DrivePath.c_str(),0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);
	SendMessage(m_hToolbar,TB_CHANGEBITMAP,di.ID,shfi.iIcon);
}

int CDrivesToolbar::GetSortedPosition(const std::wstring &DrivePath)
{
	int Position = 0;

	int nButtons = static_cast<int>(SendMessage(m_hToolbar,TB_BUTTONCOUNT,0,0));

	for(int i = 0;i < nButtons;i++)
	{
		TBBUTTON tbButton;
		SendMessage(m_hToolbar,TB_GETBUTTON,i,reinterpret_cast<LPARAM>(&tbButton));

		auto itr = m_mapID.find(static_cast<IDCounter>(static_cast<UINT>(tbButton.dwData)));
		assert(itr != m_mapID.end());

		if(DrivePath.compare(itr->second) < 0)
		{
			break;
		}

		Position++;
	}

	return Position;
}

CDrivesToolbar::DriveInformation_t CDrivesToolbar::GetDrivePosition(const std::wstring &DrivePath)
{
	DriveInformation_t di;
	di.Position = -1;

	int nButtons = static_cast<int>(SendMessage(m_hToolbar,TB_BUTTONCOUNT,0,0));

	for(int i = 0;i < nButtons;i++)
	{
		TBBUTTON tbButton;
		SendMessage(m_hToolbar,TB_GETBUTTON,i,reinterpret_cast<LPARAM>(&tbButton));

		auto itr = m_mapID.find(static_cast<IDCounter>(static_cast<UINT>(tbButton.dwData)));
		assert(itr != m_mapID.end());

		if(DrivePath.compare(itr->second) == 0)
		{
			di.Position = i;
			di.ID = static_cast<IDCounter>(static_cast<UINT>(tbButton.dwData));
			break;
		}
	}

	assert(di.Position != -1);

	return di;
}

std::wstring CDrivesToolbar::GetDrivePath(int iIndex)
{
	TBBUTTON tbButton;
	SendMessage(m_hToolbar,TB_GETBUTTON,iIndex,reinterpret_cast<LPARAM>(&tbButton));

	auto itr = m_mapID.find(static_cast<IDCounter>(static_cast<UINT>(tbButton.dwData)));
	assert(itr != m_mapID.end());

	return itr->second;
}

void CDrivesToolbar::AddMenuEntries(LPITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,HMENU hMenu)
{
	TCHAR szTemp[64];
	LoadString(m_hInstance,IDS_GENERAL_OPEN_IN_NEW_TAB,szTemp,SIZEOF_ARRAY(szTemp));

	MENUITEMINFO mii;
	mii.cbSize		= sizeof(mii);
	mii.fMask		= MIIM_STRING|MIIM_ID;
	mii.wID			= MENU_ID_OPEN_IN_NEW_TAB;
	mii.dwTypeData	= szTemp;
	InsertMenuItem(hMenu,1,TRUE,&mii);
}

BOOL CDrivesToolbar::HandleShellMenuItem(LPITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,DWORD_PTR dwData,TCHAR *szCmd)
{
	if(StrCmpI(szCmd,_T("open")) == 0)
	{
		m_pexpp->OpenItem(pidlParent,FALSE,FALSE);
		return TRUE;
	}

	return FALSE;
}

void CDrivesToolbar::HandleCustomMenuItem(LPITEMIDLIST pidlParent,
	const std::list<LPITEMIDLIST> &pidlItemList,int iCmd)
{
	switch(iCmd)
	{
	case MENU_ID_OPEN_IN_NEW_TAB:
		m_pexpp->BrowseFolder(pidlParent,SBSP_ABSOLUTE,TRUE,TRUE,FALSE);
		break;
	}
}