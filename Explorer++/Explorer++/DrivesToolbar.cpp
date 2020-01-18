// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DrivesToolbar.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"

DrivesToolbar *DrivesToolbar::Create(HWND hParent, UINT uIDStart, UINT uIDEnd,
	HINSTANCE hInstance, IExplorerplusplus *pexpp, Navigation *navigation)
{
	return new DrivesToolbar(hParent, uIDStart, uIDEnd, hInstance, pexpp, navigation);
}

DrivesToolbar::DrivesToolbar(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance,
	IExplorerplusplus *pexpp, Navigation *navigation) :
	BaseWindow(CreateDrivesToolbar(hParent)),
	m_hInstance(hInstance),
	m_uIDStart(uIDStart),
	m_uIDEnd(uIDEnd),
	m_pexpp(pexpp),
	m_navigation(navigation),
	m_IDCounter(0)
{
	Initialize(hParent);

	HardwareChangeNotifier::GetInstance().AddObserver(this);
}

DrivesToolbar::~DrivesToolbar()
{
	HardwareChangeNotifier::GetInstance().RemoveObserver(this);
}

HWND DrivesToolbar::CreateDrivesToolbar(HWND hParent)
{
	return CreateToolbar(hParent, WS_CHILD | WS_VISIBLE |
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS |
		TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_FLAT |
		CCS_NODIVIDER | CCS_NORESIZE, TBSTYLE_EX_DOUBLEBUFFER |
		TBSTYLE_EX_HIDECLIPPEDBUTTONS);
}

void DrivesToolbar::Initialize(HWND hParent)
{
	SendMessage(m_hwnd,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(NULL,&himlSmall);

	int iconWidth;
	int iconHeight;
	ImageList_GetIconSize(himlSmall, &iconWidth, &iconHeight);
	SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));

	SendMessage(m_hwnd,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(himlSmall));

	m_windowSubclasses.push_back(WindowSubclassWrapper(hParent, DrivesToolbarParentProcStub,
		PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	InsertDrives();
}

INT_PTR DrivesToolbar::OnMButtonUp(const POINTS *pts)
{
	POINT pt;
	POINTSTOPOINT(pt, *pts);
	int iIndex = static_cast<int>(SendMessage(m_hwnd, TB_HITTEST, 0, reinterpret_cast<LPARAM>(&pt)));

	if(iIndex >= 0)
	{
		TBBUTTON tbButton;
		SendMessage(m_hwnd, TB_GETBUTTON, iIndex, reinterpret_cast<LPARAM>(&tbButton));

		auto itr = m_mapID.find(static_cast<IDCounter>(static_cast<UINT>(tbButton.dwData)));
		assert(itr != m_mapID.end());

		m_pexpp->GetTabContainer()->CreateNewTab(itr->second.c_str(), TabSettings(_selected = true));
	}

	return 0;
}

void DrivesToolbar::OnDeviceArrival(DEV_BROADCAST_HDR *dbh)
{
	if(dbh->dbch_devicetype != DBT_DEVTYP_VOLUME)
	{
		return;
	}

	DEV_BROADCAST_VOLUME *pdbv = reinterpret_cast<DEV_BROADCAST_VOLUME *>(dbh);

	/* Build a string that will form the drive name. */
	TCHAR szDrive[4];
	TCHAR chDrive = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
	StringCchPrintf(szDrive,SIZEOF_ARRAY(szDrive),_T("%c:\\"),chDrive);

	/* Is there a change in media, or a change
	in the physical device?
	If this is only a change in media, simply
	update any icons that may need updating;
	else if this is a change in the physical
	device, add the device in the required
	areas. */
	if(pdbv->dbcv_flags & DBTF_MEDIA)
	{
		UpdateDriveIcon(szDrive);
	}
	else
	{
		/* A drive was added to the system,
		so add it to the toolbar. */
		InsertDrive(szDrive);
	}
}

void DrivesToolbar::OnDeviceRemoveComplete(DEV_BROADCAST_HDR *dbh)
{
	if(dbh->dbch_devicetype != DBT_DEVTYP_VOLUME)
	{
		return;
	}

	DEV_BROADCAST_VOLUME *pdbv = reinterpret_cast<DEV_BROADCAST_VOLUME *>(dbh);

	TCHAR szDrive[4];
	TCHAR chDrive = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
	StringCchPrintf(szDrive,SIZEOF_ARRAY(szDrive),_T("%c:\\"),chDrive);

	/* Media changed or drive removed? */
	if(pdbv->dbcv_flags & DBTF_MEDIA)
	{
		UpdateDriveIcon(szDrive);
	}
	else
	{
		/* The device was removed from the system.
		Remove its button from the toolbar. */
		RemoveDrive(szDrive);
	}
}

LRESULT CALLBACK DrivesToolbar::DrivesToolbarParentProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	DrivesToolbar *pdt = reinterpret_cast<DrivesToolbar *>(dwRefData);

	return pdt->DrivesToolbarParentProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK DrivesToolbar::DrivesToolbarParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		if(LOWORD(wParam) >= m_uIDStart &&
			LOWORD(wParam) <= m_uIDEnd)
		{
			int iIndex = static_cast<int>(SendMessage(m_hwnd,TB_COMMANDTOINDEX,LOWORD(wParam),0));

			if(iIndex != -1)
			{
				std::wstring Path = GetDrivePath(iIndex);
				m_navigation->BrowseFolderInCurrentTab(Path.c_str());
			}

			return 0;
		}
		break;

	case WM_NOTIFY:
		if(reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch(reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
				{
					NMMOUSE *pnmm = reinterpret_cast<NMMOUSE *>(lParam);

					if(pnmm->dwItemSpec != -1)
					{
						int iIndex = static_cast<int>(SendMessage(m_hwnd,TB_COMMANDTOINDEX,pnmm->dwItemSpec,0));

						if(iIndex != -1)
						{
							std::wstring Path = GetDrivePath(iIndex);

							unique_pidl_absolute pidlItem;
							HRESULT hr = SHParseDisplayName(Path.c_str(), nullptr, wil::out_param(pidlItem), 0, nullptr);

							if(SUCCEEDED(hr))
							{
								ClientToScreen(m_hwnd,&pnmm->pt);

								std::vector<PCITEMID_CHILD> pidlItems;
								FileContextMenuManager fcmm(m_hwnd,pidlItem.get(),pidlItems);

								fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,&pnmm->pt,m_pexpp->GetStatusBar(),
									NULL,FALSE,IsKeyDown(VK_SHIFT));
							}

							return TRUE;
						}
					}
				}
				break;

			case TBN_GETINFOTIP:
				{
					NMTBGETINFOTIP *pnmtbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

					int iIndex = static_cast<int>(SendMessage(m_hwnd,TB_COMMANDTOINDEX,pnmtbgit->iItem,0));

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

void DrivesToolbar::InsertDrives()
{
	DWORD dwSize = GetLogicalDriveStrings(0,NULL);

	auto driveStrings = std::make_unique<TCHAR[]>(dwSize);
	dwSize = GetLogicalDriveStrings(dwSize,driveStrings.get());

	if(dwSize != 0)
	{
		TCHAR *pDrive = driveStrings.get();

		while(*pDrive != '\0')
		{
			InsertDrive(pDrive);

			pDrive += (lstrlen(pDrive) + 1);
		}
	}
}

void DrivesToolbar::InsertDrive(const std::wstring &DrivePath)
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
	SendMessage(m_hwnd,TB_INSERTBUTTON,Position,reinterpret_cast<LPARAM>(&tbButton));
	UpdateToolbarBandSizing(GetParent(m_hwnd),m_hwnd);

	m_mapID.insert(std::make_pair(m_IDCounter,DrivePath));
	++m_IDCounter;
}

void DrivesToolbar::RemoveDrive(const std::wstring &DrivePath)
{
	DriveInformation_t di = GetDrivePosition(DrivePath);

	if(di.Position != -1)
	{
		SendMessage(m_hwnd,TB_DELETEBUTTON,di.Position,0);
		UpdateToolbarBandSizing(GetParent(m_hwnd),m_hwnd);
		m_mapID.erase(di.ID);
	}
}

/* Updates an items icon. This may be necessary,
for example, if a cd/dvd is inserted/removed. */
void DrivesToolbar::UpdateDriveIcon(const std::wstring &DrivePath)
{
	DriveInformation_t di = GetDrivePosition(DrivePath);

	if(di.Position != -1)
	{
		SHFILEINFO shfi;
		SHGetFileInfo(DrivePath.c_str(),0,&shfi,sizeof(shfi),SHGFI_SYSICONINDEX);
		SendMessage(m_hwnd,TB_CHANGEBITMAP,di.ID,shfi.iIcon);
	}
}

int DrivesToolbar::GetSortedPosition(const std::wstring &DrivePath)
{
	int Position = 0;

	int nButtons = static_cast<int>(SendMessage(m_hwnd,TB_BUTTONCOUNT,0,0));

	for(int i = 0;i < nButtons;i++)
	{
		TBBUTTON tbButton;
		SendMessage(m_hwnd,TB_GETBUTTON,i,reinterpret_cast<LPARAM>(&tbButton));

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

DrivesToolbar::DriveInformation_t DrivesToolbar::GetDrivePosition(const std::wstring &DrivePath)
{
	DriveInformation_t di;
	di.Position = -1;

	int nButtons = static_cast<int>(SendMessage(m_hwnd,TB_BUTTONCOUNT,0,0));

	for(int i = 0;i < nButtons;i++)
	{
		TBBUTTON tbButton;
		SendMessage(m_hwnd,TB_GETBUTTON,i,reinterpret_cast<LPARAM>(&tbButton));

		auto itr = m_mapID.find(static_cast<IDCounter>(static_cast<UINT>(tbButton.dwData)));
		assert(itr != m_mapID.end());

		if(DrivePath.compare(itr->second) == 0)
		{
			di.Position = i;
			di.ID = static_cast<IDCounter>(static_cast<UINT>(tbButton.dwData));
			break;
		}
	}

	return di;
}

std::wstring DrivesToolbar::GetDrivePath(int iIndex)
{
	TBBUTTON tbButton;
	SendMessage(m_hwnd,TB_GETBUTTON,iIndex,reinterpret_cast<LPARAM>(&tbButton));

	auto itr = m_mapID.find(static_cast<IDCounter>(static_cast<UINT>(tbButton.dwData)));
	assert(itr != m_mapID.end());

	return itr->second;
}

void DrivesToolbar::AddMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, HMENU hMenu)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(dwData);

	std::wstring openInNewTabText = ResourceHelper::LoadString(m_hInstance,IDS_GENERAL_OPEN_IN_NEW_TAB);

	MENUITEMINFO mii;
	mii.cbSize		= sizeof(mii);
	mii.fMask		= MIIM_STRING|MIIM_ID;
	mii.wID			= MENU_ID_OPEN_IN_NEW_TAB;
	mii.dwTypeData	= openInNewTabText.data();
	InsertMenuItem(hMenu,1,TRUE,&mii);
}

BOOL DrivesToolbar::HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd)
{
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(dwData);

	if(StrCmpI(szCmd,_T("open")) == 0)
	{
		m_pexpp->OpenItem(pidlParent,FALSE,FALSE);
		return TRUE;
	}

	return FALSE;
}

void DrivesToolbar::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, int iCmd)
{
	UNREFERENCED_PARAMETER(pidlItems);

	switch(iCmd)
	{
	case MENU_ID_OPEN_IN_NEW_TAB:
		m_pexpp->GetTabContainer()->CreateNewTab(pidlParent, TabSettings(_selected = true));
		break;
	}
}