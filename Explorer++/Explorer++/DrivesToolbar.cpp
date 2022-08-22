// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DrivesToolbar.h"
#include "Config.h"
#include "CoreInterface.h"
#include "DarkModeHelper.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "TabContainer.h"
#include "../Helper/Controls.h"
#include "../Helper/DriveInfo.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"

DrivesToolbar *DrivesToolbar::Create(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance,
	IExplorerplusplus *pexpp, Navigation *navigation)
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
	m_navigation(navigation)
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
	return CreateToolbar(hParent,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS | TBSTYLE_LIST
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE,
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
}

void DrivesToolbar::Initialize(HWND hParent)
{
	SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	HIMAGELIST himlSmall;
	Shell_GetImageLists(nullptr, &himlSmall);

	int iconWidth;
	int iconHeight;
	ImageList_GetIconSize(himlSmall, &iconWidth, &iconHeight);
	SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(iconWidth, iconHeight));

	SendMessage(m_hwnd, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(himlSmall));

	m_windowSubclasses.push_back(std::make_unique<WindowSubclassWrapper>(hParent,
		DrivesToolbarParentProcStub, PARENT_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this)));

	InsertDrives();

	auto &darkModeHelper = DarkModeHelper::GetInstance();

	if (darkModeHelper.IsDarkModeEnabled())
	{
		darkModeHelper.SetDarkModeForToolbarTooltips(m_hwnd);
	}
}

INT_PTR DrivesToolbar::OnMButtonUp(const POINTS *pts, UINT keysDown)
{
	POINT pt;
	POINTSTOPOINT(pt, *pts);
	int index = static_cast<int>(SendMessage(m_hwnd, TB_HITTEST, 0, reinterpret_cast<LPARAM>(&pt)));

	if (index < 0)
	{
		return 0;
	}

	auto drive = MaybeGetDriveFromIndex(index);

	if (!drive)
	{
		return 0;
	}

	bool switchToNewTab = m_pexpp->GetConfig()->openTabsInForeground;

	if (WI_IsFlagSet(keysDown, MK_SHIFT))
	{
		switchToNewTab = !switchToNewTab;
	}

	m_pexpp->GetTabContainer()->CreateNewTab(drive->path.c_str(),
		TabSettings(_selected = switchToNewTab));

	return 0;
}

void DrivesToolbar::OnDeviceArrival(DEV_BROADCAST_HDR *dbh)
{
	if (dbh->dbch_devicetype != DBT_DEVTYP_VOLUME)
	{
		return;
	}

	auto *pdbv = reinterpret_cast<DEV_BROADCAST_VOLUME *>(dbh);

	/* Build a string that will form the drive name. */
	TCHAR szDrive[4];
	TCHAR chDrive = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
	StringCchPrintf(szDrive, SIZEOF_ARRAY(szDrive), _T("%c:\\"), chDrive);

	/* Is there a change in media, or a change
	in the physical device?
	If this is only a change in media, simply
	update any icons that may need updating;
	else if this is a change in the physical
	device, add the device in the required
	areas. */
	if (pdbv->dbcv_flags & DBTF_MEDIA)
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
	if (dbh->dbch_devicetype != DBT_DEVTYP_VOLUME)
	{
		return;
	}

	auto *pdbv = reinterpret_cast<DEV_BROADCAST_VOLUME *>(dbh);

	TCHAR szDrive[4];
	TCHAR chDrive = GetDriveLetterFromMask(pdbv->dbcv_unitmask);
	StringCchPrintf(szDrive, SIZEOF_ARRAY(szDrive), _T("%c:\\"), chDrive);

	/* Media changed or drive removed? */
	if (pdbv->dbcv_flags & DBTF_MEDIA)
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

LRESULT CALLBACK DrivesToolbar::DrivesToolbarParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	auto *pdt = reinterpret_cast<DrivesToolbar *>(dwRefData);

	return pdt->DrivesToolbarParentProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DrivesToolbar::DrivesToolbarParentProc(HWND hwnd, UINT uMsg, WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) >= m_uIDStart && LOWORD(wParam) <= m_uIDEnd)
		{
			int index = static_cast<int>(SendMessage(m_hwnd, TB_COMMANDTOINDEX, LOWORD(wParam), 0));

			if (index == -1)
			{
				break;
			}

			auto drive = MaybeGetDriveFromIndex(index);

			if (!drive)
			{
				break;
			}

			m_navigation->BrowseFolderInCurrentTab(drive->path.c_str());

			return 0;
		}
		break;

	case WM_NOTIFY:
		if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == m_hwnd)
		{
			switch (reinterpret_cast<LPNMHDR>(lParam)->code)
			{
			case NM_RCLICK:
			{
				auto *pnmm = reinterpret_cast<NMMOUSE *>(lParam);

				if (pnmm->dwItemSpec == -1)
				{
					break;
				}

				int index =
					static_cast<int>(SendMessage(m_hwnd, TB_COMMANDTOINDEX, pnmm->dwItemSpec, 0));

				if (index == -1)
				{
					break;
				}

				auto drive = MaybeGetDriveFromIndex(index);

				if (!drive)
				{
					break;
				}

				unique_pidl_absolute pidl;
				HRESULT hr = SHParseDisplayName(drive->path.c_str(), nullptr, wil::out_param(pidl),
					0, nullptr);

				if (FAILED(hr))
				{
					break;
				}

				ClientToScreen(m_hwnd, &pnmm->pt);

				unique_pidl_child child(ILCloneChild(ILFindLastID(pidl.get())));

				[[maybe_unused]] BOOL res = ILRemoveLastID(pidl.get());
				assert(res);

				FileContextMenuManager fcmm(m_hwnd, pidl.get(), { child.get() });

				fcmm.ShowMenu(this, MIN_SHELL_MENU_ID, MAX_SHELL_MENU_ID, &pnmm->pt,
					m_pexpp->GetStatusBar(), NULL, FALSE, IsKeyDown(VK_SHIFT));

				return TRUE;
			}
			break;

			case TBN_GETINFOTIP:
			{
				auto *pnmtbgit = reinterpret_cast<NMTBGETINFOTIP *>(lParam);

				int index =
					static_cast<int>(SendMessage(m_hwnd, TB_COMMANDTOINDEX, pnmtbgit->iItem, 0));

				if (index == -1)
				{
					break;
				}

				auto drive = MaybeGetDriveFromIndex(index);

				if (!drive)
				{
					break;
				}

				GetItemInfoTip(drive->path.c_str(), pnmtbgit->pszText, pnmtbgit->cchTextMax);

				return 0;
			}
			}
		}
		break;
	}

	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void DrivesToolbar::InsertDrives()
{
	DWORD dwSize = GetLogicalDriveStrings(0, nullptr);

	auto driveStrings = std::make_unique<TCHAR[]>(dwSize);
	dwSize = GetLogicalDriveStrings(dwSize, driveStrings.get());

	if (dwSize != 0)
	{
		TCHAR *pDrive = driveStrings.get();

		while (*pDrive != '\0')
		{
			InsertDrive(pDrive);

			pDrive += (lstrlen(pDrive) + 1);
		}
	}
}

void DrivesToolbar::InsertDrive(const std::wstring &drivePath)
{
	// Drives shouldn't be added multiple times, so if this drive already exists in the toolbar,
	// simply bail out.
	if (MaybeGetDriveIndex(drivePath))
	{
		return;
	}

	TCHAR szDisplayName[32];
	StringCchCopy(szDisplayName, SIZEOF_ARRAY(szDisplayName), drivePath.c_str());

	/* Drives will be shown without a closing backslash. */
	if (szDisplayName[lstrlen(szDisplayName) - 1] == '\\')
	{
		szDisplayName[lstrlen(szDisplayName) - 1] = '\0';
	}

	auto [itr, inserted] = m_drives.emplace(drivePath);
	assert(inserted);

	SHFILEINFO shfi;
	SHGetFileInfo(drivePath.c_str(), 0, &shfi, sizeof(shfi),
		SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);

	int index = static_cast<int>(std::distance(m_drives.begin(), itr));

	TBBUTTON tbButton = {};
	tbButton.iBitmap = shfi.iIcon;
	tbButton.idCommand = m_uIDStart + m_idCounter;
	tbButton.fsState = TBSTATE_ENABLED;
	tbButton.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_NOPREFIX;
	tbButton.iString = reinterpret_cast<INT_PTR>(szDisplayName);
	SendMessage(m_hwnd, TB_INSERTBUTTON, index, reinterpret_cast<LPARAM>(&tbButton));
	UpdateToolbarBandSizing(GetParent(m_hwnd), m_hwnd);

	++m_idCounter;
}

void DrivesToolbar::RemoveDrive(const std::wstring &drivePath)
{
	auto itr = std::find_if(m_drives.begin(), m_drives.end(),
		[&drivePath](const Drive &drive)
		{
			return drive.path.compare(drivePath) == 0;
		});

	if (itr == m_drives.end())
	{
		return;
	}

	int index = static_cast<int>(std::distance(m_drives.begin(), itr));

	SendMessage(m_hwnd, TB_DELETEBUTTON, index, 0);
	UpdateToolbarBandSizing(GetParent(m_hwnd), m_hwnd);
	m_drives.erase(itr);
}

/* Updates an items icon. This may be necessary,
for example, if a cd/dvd is inserted/removed. */
void DrivesToolbar::UpdateDriveIcon(const std::wstring &drivePath)
{
	auto index = MaybeGetDriveIndex(drivePath);

	if (!index)
	{
		return;
	}

	SHFILEINFO shfi;
	auto res = SHGetFileInfo(drivePath.c_str(), 0, &shfi, sizeof(shfi), SHGFI_SYSICONINDEX);

	if (res == 0)
	{
		return;
	}

	TBBUTTONINFO tbbi;
	tbbi.cbSize = sizeof(tbbi);
	tbbi.dwMask = TBIF_BYINDEX | TBIF_IMAGE;
	tbbi.iImage = shfi.iIcon;
	SendMessage(m_hwnd, TB_SETBUTTONINFO, *index, reinterpret_cast<LPARAM>(&tbbi));
}

const DrivesToolbar::Drive *DrivesToolbar::MaybeGetDriveFromIndex(int index) const
{
	if (index < 0 || index >= m_drives.size())
	{
		return nullptr;
	}

	auto itr = m_drives.begin();
	std::advance(itr, index);
	return &*itr;
}

std::optional<int> DrivesToolbar::MaybeGetDriveIndex(const std::wstring &drivePath)
{
	auto itr = std::find_if(m_drives.begin(), m_drives.end(),
		[&drivePath](const Drive &drive)
		{
			return drive.path.compare(drivePath) == 0;
		});

	if (itr == m_drives.end())
	{
		return std::nullopt;
	}

	return static_cast<int>(std::distance(m_drives.begin(), itr));
}

void DrivesToolbar::UpdateMenuEntries(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, IContextMenu *contextMenu,
	HMENU hMenu)
{
	UNREFERENCED_PARAMETER(pidlParent);
	UNREFERENCED_PARAMETER(pidlItems);
	UNREFERENCED_PARAMETER(dwData);
	UNREFERENCED_PARAMETER(contextMenu);

	std::wstring openInNewTabText =
		ResourceHelper::LoadString(m_hInstance, IDS_GENERAL_OPEN_IN_NEW_TAB);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING | MIIM_ID;
	mii.wID = MENU_ID_OPEN_IN_NEW_TAB;
	mii.dwTypeData = openInNewTabText.data();
	InsertMenuItem(hMenu, 1, TRUE, &mii);
}

BOOL DrivesToolbar::HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd)
{
	UNREFERENCED_PARAMETER(dwData);

	if (StrCmpI(szCmd, _T("open")) == 0)
	{
		assert(pidlItems.size() == 1);

		unique_pidl_absolute pidl(ILCombine(pidlParent, pidlItems[0]));
		m_pexpp->OpenItem(pidl.get());
		return TRUE;
	}

	return FALSE;
}

void DrivesToolbar::HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent,
	const std::vector<PITEMID_CHILD> &pidlItems, int iCmd)
{
	UNREFERENCED_PARAMETER(pidlItems);

	switch (iCmd)
	{
	case MENU_ID_OPEN_IN_NEW_TAB:
	{
		assert(pidlItems.size() == 1);

		unique_pidl_absolute pidl(ILCombine(pidlParent, pidlItems[0]));
		m_pexpp->GetTabContainer()->CreateNewTab(pidl.get(),
			TabSettings(_selected = m_pexpp->GetConfig()->openTabsInForeground));
	}
	break;
	}
}
