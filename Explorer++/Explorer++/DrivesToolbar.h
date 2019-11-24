// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "HardwareChangeNotifier.h"
#include "Navigation.h"
#include "../Helper/BaseWindow.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/WindowSubclassWrapper.h"
#include <boost/serialization/strong_typedef.hpp>
#include <list>
#include <unordered_map>

class CDrivesToolbar : public CBaseWindow, public IFileContextMenuExternal, public NHardwareChangeNotifier::INotification
{
public:

	static CDrivesToolbar	*Create(HWND hParent, UINT uIDStart, UINT uIDEnd,
		HINSTANCE hInstance, IExplorerplusplus *pexpp, Navigation *navigation);

	/* IFileContextMenuExternal methods. */
	void	AddMenuEntries(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, HMENU hMenu);
	BOOL	HandleShellMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems, DWORD_PTR dwData, const TCHAR *szCmd);
	void	HandleCustomMenuItem(PCIDLIST_ABSOLUTE pidlParent, const std::vector<PITEMID_CHILD> &pidlItems, int iCmd);

protected:

	INT_PTR	OnMButtonUp(const POINTS *pts);

private:

	BOOST_STRONG_TYPEDEF(UINT,IDCounter);

	struct DriveInformation_t
	{
		int			Position;
		IDCounter	ID;
	};

	static const UINT_PTR PARENT_SUBCLASS_ID = 0;

	static const int MIN_SHELL_MENU_ID = 1;
	static const int MAX_SHELL_MENU_ID = 1000;

	static const int MENU_ID_OPEN_IN_NEW_TAB = (MAX_SHELL_MENU_ID + 1);

	static LRESULT CALLBACK DrivesToolbarParentProcStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	LRESULT CALLBACK DrivesToolbarParentProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	CDrivesToolbar(HWND hParent, UINT uIDStart, UINT uIDEnd, HINSTANCE hInstance,
		IExplorerplusplus *pexpp, Navigation *navigation);
	~CDrivesToolbar();

	static HWND	CreateDrivesToolbar(HWND hParent);

	void		Initialize(HWND hParent);

	void		InsertDrives();
	void		InsertDrive(const std::wstring &DrivePath);
	void		RemoveDrive(const std::wstring &DrivePath);

	int			GetSortedPosition(const std::wstring &DrivePath);
	DriveInformation_t GetDrivePosition(const std::wstring &DrivePath);
	std::wstring GetDrivePath(int iIndex);

	void		UpdateDriveIcon(const std::wstring &DrivePath);

	void		OnDeviceArrival(DEV_BROADCAST_HDR *dbh);
	void		OnDeviceRemoveComplete(DEV_BROADCAST_HDR *dbh);

	HINSTANCE m_hInstance;

	UINT m_uIDStart;
	UINT m_uIDEnd;

	IExplorerplusplus *m_pexpp;
	Navigation *m_navigation;

	struct IDCounterHasher
	{
		size_t operator()(const IDCounter& t) const
		{
			return (size_t)t;
		}
	};

	std::unordered_map<IDCounter,std::wstring,IDCounterHasher> m_mapID;

	IDCounter m_IDCounter;

	std::vector<WindowSubclassWrapper> m_windowSubclasses;
};